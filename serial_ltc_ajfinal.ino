#include "SPI.h"
#include "EEPROM.h"

const int numdacs=4;
int dacs[numdacs]={10,9,8,7};

const int numadc_per_dac=16;

uint8_t chipSelect;
uint8_t channel;

const byte numChars = 128;
char receivedChars[numChars];
char tempChars[numChars];
char messageFromPC[numChars] = {0};
int channelFromPC = 0;
int chipSelectFromPC = 0;
float floatFromPC = 0.0;
boolean newData = false;

const int numchan=64;
bool goodtransfer=true;

struct eep {
  float voltage[numchan];
  float slope[numchan];
  float offset[numchan];
};

eep eep;

void setup() {
  Serial.begin(115200);
  Serial.println("Enter data in this style <SET chipSelect channel voltage>");
  if (sizeof(eep)>EEPROM.length()) {
    Serial.println("Warning size of needed memory exceeds EEPROM");
  }
  for (int i=0;i<numdacs;i++) {
    pinMode(dacs[i],OUTPUT); // set this arduino pin to output
    digitalWrite(dacs[i],HIGH); // set CSbar=high i.e. not selected
  }

  // assign other pins as outputs as well.
  // Can be useful if other pins are blown out.
  pinMode(6,OUTPUT);
  pinMode(5,OUTPUT);
  pinMode(4,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(2,OUTPUT);
  digitalWrite(6,HIGH);
  digitalWrite(5,HIGH);
  digitalWrite(4,HIGH);
  digitalWrite(3,HIGH);
  digitalWrite(2,HIGH);
  
  SPI.begin();

  for (int i=0;i<numdacs;i++) {
    set_span(dacs[i]); // set the software span for each DAC
  }
  
  read_eep();
}

void set_span(int dac) {  
  digitalWrite(dac,LOW);
  SPI.transfer16(0x00e0);  // command code: write span to all
  SPI.transfer16(0x0003);  // span +/- 10 V
  digitalWrite(dac,HIGH);
}

void read_eep () {
  EEPROM.get(0,eep);
}

void write_eep () {
  EEPROM.put(0,eep);
}

void reset_eep_default () {
  for (int i=0;i<numchan;i++) {
    eep.voltage[i]=0;
    eep.slope[i]=0.04/10;
    eep.offset[i]=0;
  }
}

void loop() {
  int cs;
  int ch;
  int voltage_index;
  recvWithStartEndMarkers();
  if (newData == true) {
    strcpy(tempChars, receivedChars);

    // The routine parseData() interprets a command
    // and sets any relevant variables we would need
    // in order to complete the command
    
    parseData();
    // showParsedData();


    // Once parseData() has been completed, the relevant
    // variables needed to complete the command are now
    // set.  So, we just need to translate them into the
    // appropriate commands for the Arduino and DAC chip, and
    // then issue those commands.

    // Summary:
    // - parseData() gets the relevant data that is needed
    // - loop() actually issues the commands to the arduino/DAC chip

   
    if (!strncmp(messageFromPC, "SET", 3)) {
      chipSelect = chipSelectFromPC;
      channel = channelFromPC;

      Serial.print("Setting CSbar ");
      Serial.print(chipSelect);
      Serial.print(" channel ");
      Serial.print(channel);
      Serial.print(" to ");
      Serial.print(floatFromPC,7);
      Serial.println(" V");

      digitalWrite(chipSelect,LOW);
      SPI.transfer16(0x0030|(channel&0xF)); // & channel with 0xF so that only 0-15 can appear -- prevents erroneous commands being sent.
      SPI.transfer16(dac_value(floatFromPC));
      digitalWrite(chipSelect, HIGH);
    } else if (!strncmp(messageFromPC, "MUX", 3)) {
      Serial.println("Changing MUX");
      chipSelect = chipSelectFromPC;
      channel = channelFromPC;

      digitalWrite(chipSelect, LOW);
      SPI.transfer16(0x00b0);
      SPI.transfer16(0x0010 | channel);
      digitalWrite(chipSelect, HIGH);

    } else if (!strncmp(messageFromPC,"ZERO",4)) {
      Serial.println("Zeroing all channels");
      for (int i=0;i<numdacs;i++) {
        for (int c=0;c<numadc_per_dac;c++) {    
          Serial.print("Zeroing CSbar ");
          Serial.print(dacs[i]);
          Serial.print(" channel ");
          Serial.println(c);
          digitalWrite(dacs[i],LOW);
          SPI.transfer16(0x0030|(c & 0xF));
          SPI.transfer16(dac_value(0.));
          digitalWrite(chipSelect, HIGH);
        }
      }
      Serial.println("Done zeroing.");
    } else if (!strncmp(messageFromPC,"STV",3)) { // STV = set voltage number i
      Serial.print("Voltage ");
      Serial.print(channelFromPC);
      Serial.print(" set to ");
      Serial.print(floatFromPC);
      Serial.println(" V");
      eep.voltage[channelFromPC]=floatFromPC;
    } else if (!strncmp(messageFromPC,"STC",3)) { // STC = set current number i
      Serial.print("Current ");
      Serial.print(channelFromPC);
      Serial.print(" set to ");
      Serial.print(floatFromPC);
      Serial.println(" A");
      eep.voltage[channelFromPC]=(floatFromPC-eep.offset[channelFromPC])/eep.slope[channelFromPC]; // c=mV+b
    } else if (!strncmp(messageFromPC,"PRI",3)) { // PRI = print
      for (int i=0;i<numchan;i++) {
        Serial.print(i);
        Serial.print(" ");
        Serial.print(eep.voltage[i],10); // print voltage
        Serial.print(" ");
        Serial.print(eep.slope[i]*eep.voltage[i]+eep.offset[i],10); // print current
        Serial.print(" ");
        Serial.print(eep.slope[i],10); // m from c=mV+b
        Serial.print(" ");
        Serial.print(eep.offset[i],10); // b from c=mV+b
        Serial.print(" ");
        get_cs_ch(i,cs,ch);
        Serial.print(cs);
        Serial.print(" ");
        Serial.print(ch);
        Serial.println();
      }
    } else if (!strncmp(messageFromPC,"ONA",3)) { // ONA = on all
      for (int i=0;i<numchan;i++) {
        on_voltage_i(i,eep.voltage[i]);
      }
      Serial.println("All channels on.");
    } else if (!strncmp(messageFromPC,"OFA",3)) { // OFA = off all (set everything to zero)
      for (int i=0;i<numchan;i++) {
        on_voltage_i(i,0);
      }
      Serial.println("All channels off.");
    } else if (!strncmp(messageFromPC,"ONN",3)) { // ONN = on neg
      for (int i=0;i<numchan;i++) {
        on_voltage_i(i,-eep.voltage[i]);
      }
      Serial.println("All channels set to negative.");
    } else if (!strncmp(messageFromPC,"RES",3)) { // RES = reset eeprom to default
      reset_eep_default();
      write_eep();
      Serial.println("EEPROM reset to default values.");
    } else if (!strncmp(messageFromPC,"REA",3)) { // REA = read from eeprom
      read_eep();
      Serial.println("read voltages and calibration constants from EEPROM.");
    } else if (!strncmp(messageFromPC,"WRI",3)) { // WRI = write to eeprom
      write_eep();
      Serial.println("voltages and calibration constants written to EEPROM.");
    }
    newData = false;
  }
}

void on_voltage_i(int i,float v) { // set voltage on channel i
  int cs;
  int ch;
  get_cs_ch(i,cs,ch);
  on_voltage_cs_ch(cs,ch,v);
}

void on_voltage_cs_ch(int cs,int ch,float v) { // set voltage on csbar cs, DAC channel ch
  digitalWrite(cs,LOW);
  SPI.transfer16(0x0030|(ch&0xF)); // & channel with 0xF so that only 0-15 can appear -- prevents erroneous commands being sent.
  SPI.transfer16(dac_value(v));
  digitalWrite(cs,HIGH);
}

void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();

    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      } else {
        receivedChars[ndx] = '\0';
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    } else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

void parseData() {
  char *strtokIndx;
  char delimiter[4] = ", ";

  strtokIndx = strtok(tempChars, delimiter);
  strcpy(messageFromPC, strtokIndx);

  Serial.print("The message I received is ");
  Serial.println(messageFromPC);

  goodtransfer=true;

  if (!strncmp(messageFromPC,"SET",3)) {
    strtokIndx = strtok(NULL, delimiter);
    chipSelectFromPC = atoi(strtokIndx);
    strtokIndx = strtok(NULL, delimiter);
    channelFromPC = atoi(strtokIndx);
    strtokIndx = strtok(NULL, delimiter);
    floatFromPC = atof(strtokIndx);
  } else if (!strncmp(messageFromPC, "MUX", 3)) {
    strtokIndx = strtok(NULL, delimiter);
    chipSelectFromPC = atoi(strtokIndx);
    strtokIndx = strtok(NULL, delimiter);
    channelFromPC = atoi(strtokIndx);
  } else if ((!strncmp(messageFromPC,"STV",3))||(!strncmp(messageFromPC,"STC",3))) {
    strtokIndx = strtok(NULL, delimiter);
    channelFromPC = atoi(strtokIndx);
    strtokIndx = strtok(NULL, delimiter);
    floatFromPC = atof(strtokIndx);
  }
}

void showParsedData() {
  Serial.print("Message ");
  Serial.println(messageFromPC);
  Serial.print("CS_Integer ");
  Serial.println(chipSelectFromPC);
  Serial.print("Integer ");
  Serial.println(channelFromPC);
  Serial.print("Float ");
  Serial.println(floatFromPC);
}

uint16_t dac_value(float volts) {
  float minv = -10.0;
  float maxv = 10.0;
  return (uint16_t)((volts - minv) / (maxv - minv) * 65535);
}

void get_cs_ch(int i,int &cs,int &ch) {
  if ((i>=0)&(i<=15)) {
    cs=10;
    ch=i;
  } else if ((i>=16)&&(i<=31)) {
    cs=9;
    ch=i-16;
  } else if ((i>=32)&&(i<=47)) {
    cs=8;
    ch=i-32;
  } else if ((i>=48)&(i<=63)) {
    cs=7;
    ch=i-48;
  }
}

#include "SPI.h"
#include "EEPROM.h"

// convert from index to chip select and channel number
const int NDAC=4;              // number of chips
const int NADC_PER_DAC=16;     // output terminal indexes
const int CS_IDX2ID[NDAC]={7, 8, 9, 10};  // chip select indexes

uint8_t chipSelect;
uint8_t channel;

// variables for parsing serial commands
const byte NCHAR=60;
char receivedChars[NCHAR];
char tempChars[NCHAR];
char messageFromPC[NCHAR]={0};
int channelFromPC=0;
int chipSelectFromPC=0;
float floatFromPC=0.0;
boolean newData=false;

// this structure is used for storing things in the EEPROM
// save voltages in 2D array
struct eep {
  float voltage[NDAC][NADC_PER_DAC];
};

// initialize the struct
eep eep;

/// UTILITIES ===============================================================

// setup
void setup() {
    Serial.begin(115200);
    // Serial.println("Enter data in this style <SET chipSelect channel voltage>#");
    if (sizeof(eep)>EEPROM.length()) {
        Serial.println("Warning size of needed memory exceeds EEPROM#");
    }
    for (int i=0;i<NDAC;i++) {
        pinMode(CS_IDX2ID[i], OUTPUT); // set this arduino pin to output
        digitalWrite(CS_IDX2ID[i], HIGH); // set CSbar=high i.e. not selected
    }

    // assign other pins as outputs as well.
    // Can be useful if other pins are blown out.
    pinMode(6, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(2, OUTPUT);
    digitalWrite(6, HIGH);
    digitalWrite(5, HIGH);
    digitalWrite(4, HIGH);
    digitalWrite(3, HIGH);
    digitalWrite(2, HIGH);

    SPI.begin();

    // set the software span for each DAC
    for (int i=0;i<NDAC;i++) {
        digitalWrite(CS_IDX2ID[i], LOW);
        SPI.transfer16(0x00e0);  // command code: write span to all
        SPI.transfer16(0x0003);  // span +/- 10 V
        digitalWrite(CS_IDX2ID[i], HIGH);
    }
    Serial.println("Setup complete#");
}

// get the dac setpoint int, converted from volts
uint16_t volt2dac(float volts) {
    float minv = -10.0;
    float maxv = 10.0;
    return (uint16_t)((volts-minv)/(maxv-minv)*65535);
}

// receive messages from arduino
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
            if (ndx >= NCHAR) {
            ndx = NCHAR - 1;
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

// parse messages from serial communication into global variables. These are then
// passed to the run loop for the board to execute
void parseData() {
    char *strtokIndx;
    char delimiter[4] = ", ";

    strtokIndx = strtok(tempChars, delimiter);
    strcpy(messageFromPC, strtokIndx);

    /*// Debugging
    Serial.print("The message I received is '");
    Serial.print(messageFromPC);
    Serial.println("'");
    */

    // for a SET command, we expect a CSbar, a channel, and a voltage
    if (!strncmp(messageFromPC, "SET", 3)) {
        strtokIndx=strtok(NULL, delimiter);
        chipSelectFromPC=atoi(strtokIndx);
        strtokIndx=strtok(NULL, delimiter);
        channelFromPC=atoi(strtokIndx);
        strtokIndx=strtok(NULL, delimiter);
        floatFromPC=atof(strtokIndx);
    }

    // for a ESET command, we expect a CSbar and a channel
    else if (!strncmp(messageFromPC, "ESET", 4)) {
        strtokIndx=strtok(NULL, delimiter);
        chipSelectFromPC=atoi(strtokIndx);
        strtokIndx=strtok(NULL, delimiter);
        channelFromPC=atoi(strtokIndx);
    }

    // for a MUX command, we expect a CSbar and a channel
    else if (!strncmp(messageFromPC, "MUX", 3)) {
        strtokIndx=strtok(NULL, delimiter);
        chipSelectFromPC=atoi(strtokIndx);
        strtokIndx=strtok(NULL, delimiter);
        channelFromPC=atoi(strtokIndx);
    }

    // for PWR, we expect a CSbar
    else if (!strncmp(messageFromPC, "PWR", 3)) {
        strtokIndx = strtok(NULL, delimiter);
        chipSelectFromPC = atoi(strtokIndx);
    }

    // for any other command, we don't expect anything, just the command itself
}

// print parsed data (debugging only)
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

/// FUNCTIONS FOR BASE OPERATION =============================================

// set voltage on csbar cs, DAC channel ch, and turn on that channel
void set_voltage(int cs, int ch, float v, bool save) {
    // write
    digitalWrite(cs, LOW);
    SPI.transfer16(0x0030|(ch&0xF)); // & channel with 0xF so that only 0-15 can appear -- prevents erroneous commands being sent.
    SPI.transfer16(volt2dac(v));
    digitalWrite(cs, HIGH);

    // save voltage into eep
    if(save){
        for(int i=0; i<NDAC; i++){
            if(CS_IDX2ID[i] == cs){
                eep.voltage[i][ch] = v;
                break;
            }
        }
    }
}

/// MAIN COMMAND FUNCTIONS ===================================================

// set a voltage
void cmd_SET(){
    chipSelect=chipSelectFromPC;
    channel=channelFromPC;
    Serial.print("Setting CSbar ");
    Serial.print(chipSelect);
    Serial.print(" channel ");
    Serial.print(channel);
    Serial.print(" to ");
    Serial.print(floatFromPC, 6);
    Serial.println(" V#");
    set_voltage(chipSelect, channel, floatFromPC, true);
}

// set mux output
void cmd_MUX(){
    Serial.print("Changing MUX ");
    Serial.print(chipSelectFromPC);
    Serial.print(" to ");
    Serial.print(channelFromPC);
    Serial.println("#");
    chipSelect = chipSelectFromPC;
    channel = channelFromPC;
    digitalWrite(chipSelect, LOW);
    SPI.transfer16(0x00b0);
    SPI.transfer16(0x0010|channel);
    digitalWrite(chipSelect, HIGH);
}

// power down single channel
void cmd_PWR(){
    Serial.print("Powering down ");
    Serial.print(chipSelectFromPC);
    Serial.println("#");
    chipSelect=chipSelectFromPC;

    // do power down
    digitalWrite(chipSelect, LOW);
    SPI.transfer16(0x0050); // power down all channels
    SPI.transfer16(0x0000);
    digitalWrite(chipSelect, HIGH);
}

// zero all values, don't change eep
void cmd_ZERO(){
    for (int i=0;i<NDAC;i++) {
        for (int c=0;c<NADC_PER_DAC;c++) {
        Serial.print("Zeroing CSbar ");
        Serial.print(CS_IDX2ID[i]);
        Serial.print(" channel ");
        Serial.println(c);
        digitalWrite(CS_IDX2ID[i], LOW);
        SPI.transfer16(0x0030|(c&0xF));
        SPI.transfer16(volt2dac(0.));
        digitalWrite(chipSelect, HIGH);
        }
    }
    Serial.println("All channels zeroed#");
}

// reset onboard storage to defaults
void cmd_ERST(){
    for (int i=0; i<NDAC; i++) {
        for (int j=0; j<NADC_PER_DAC; j++){
        eep.voltage[i][j]=0;
        }
    }
    cmd_EWR();
    Serial.println("EEPROM reset to default values#");
}

// eeprom read
void cmd_ERD(){
    EEPROM.get(0, eep);
    Serial.println("read voltages and calibration constants from EEPROM#");
}

// write onboard storage from eep struct
void cmd_EWR(){
    EEPROM.put(0, eep);
    Serial.println("voltages and calibration constants written to EEPROM#");
}

// set single voltage from eep
void cmd_ESET(){
    chipSelect=chipSelectFromPC;
    channel=channelFromPC;

    // search for the cs id
    for(int i=0; i<NDAC; i++){
        if(CS_IDX2ID[i] == chipSelect){
                // print
                Serial.print("Setting CSbar ");
                Serial.print(chipSelect);
                Serial.print(" channel ");
                Serial.print(channel);
                Serial.print(" to ");
                Serial.print(eep.voltage[i][channel]);
                Serial.println(" V#");
            set_voltage(chipSelect, channel, eep.voltage[i][channel], false);
            break;
        }
    }
}

// set all voltages from eep
void cmd_ESTA(){
    Serial.println("Setting all voltages to eep saved values#");
    for(int i=0; i<NDAC; i++){
        for(int ch=0; ch<NADC_PER_DAC; ch++){
            set_voltage(CS_IDX2ID[i], ch, eep.voltage[i][ch], false);
        }
    }
}

// set negative of all eep values
void cmd_ENEG(){
    Serial.println("Setting all voltages to negative of eep saved values#");
    for(int i=0; i<NDAC; i++){
        for(int ch=0; ch<NADC_PER_DAC; ch++){
            set_voltage(CS_IDX2ID[i], ch, -1*eep.voltage[i][ch], true);
        }
    }
}

/// MAIN RUN LOOP ============================================================
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

        // Low level commands
        if      (!strncmp(messageFromPC, "SET", 3)) cmd_SET();  // base level set voltage
        else if (!strncmp(messageFromPC, "MUX", 3)) cmd_MUX();  // change the chipselect board to mux
        else if (!strncmp(messageFromPC, "PWR", 3)) cmd_PWR();  // power down chipselect cs
        else if (!strncmp(messageFromPC, "ZERO", 4))cmd_ZERO(); // zero all channels

        // EEPROM commands
        else if (!strncmp(messageFromPC, "ERST", 4))cmd_ERST(); // ERST = reset eeprom to default
        else if (!strncmp(messageFromPC, "ERD", 3)) cmd_ERD();  // ERD = read from eeprom
        else if (!strncmp(messageFromPC, "EWR", 3)) cmd_EWR();  // EWR = write to eeprom
        else if (!strncmp(messageFromPC, "ESET", 4)) cmd_ESET();  // eeprom set single value
        else if (!strncmp(messageFromPC, "ESTA", 4)) cmd_ESTA();  // eeprom set all values
        else if (!strncmp(messageFromPC, "ENEG", 4)) cmd_ENEG();  // eeprom invert all values

        // ensure new data is taken
        newData = false;
    }
}


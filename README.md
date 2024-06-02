# serial_ltc_ajfinal

The following serial commands are implemented:

<SET cs ch v>
     Sets channel ch on CSbar cs to v volts.

<MUX cs ch>
     Sets the MUX on CSbar cs to ch.

<ZERO>
     Sets all 64 voltages to zero.

"<STV i v>" "SeT Voltage"
     Sets voltage i to v volts in volatile memory.

<STC i c> "SeT Current"
     Sets current i to c amperes in volatile memory.  c=m*V+b

<SSL i m> "Set SLope"
     Sets slope i to sl amperes/volt in volatile memory.  c=m*V+b

<SOF i b> "Set OFfset"
     Sets offset i to b amperes in volatile memory. c=m*V+b

<PRI> "PRInt"
     Prints all the voltages, currents, and calibration constants in
     volatile memory.

<ONA> "ON All"
     Turns on all currents to the values stored in volatile memory.

<OFA> "OFf All"
     Turns on all currents to zero, but does not delete the values stored
     in volatile memory.

<ONN> "ON Negative"
     Turns on all currents to the negative of the values stored in
     volatile memory.

<RES> "RESet"
     Resets all voltages to zero, all calibration constants to default,
     and writes them all to the EEPROM.  Obviously this means that
     everything that was in the EEPROM is lost.

<REA> "REAd eeprom"
     Reads all voltages and calibration constants from EEPROM into
     volatile memory.

<WRI> "WRIte eeprom"
     Writes all voltages and calibration constants from volatile memory
     to EEPROM.  The values stored to EEPROM will automatically be read
     into volatile memory on next reboot or by connection made to arduino
     by serial port.

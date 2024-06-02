# serial_ltc_ajfinal

The following serial commands are implemented:

&lt;SET cs ch v&gt;
     Sets channel ch on CSbar cs to v volts.

&lt;MUX cs ch&gt;
     Sets the MUX on CSbar cs to ch.

&lt;ZERO&gt;
     Sets all 64 voltages to zero.

&lt;STV i v&gt; "SeT Voltage"
     Sets voltage i to v volts in volatile memory.

&lt;STC i c&gt; "SeT Current"
     Sets current i to c amperes in volatile memory.  c=m*V+b

&lt;SSL i m&gt; "Set SLope"
     Sets slope i to sl amperes/volt in volatile memory.  c=m*V+b

&lt;SOF i b&gt; "Set OFfset"
     Sets offset i to b amperes in volatile memory. c=m*V+b

&lt;PRI&gt; "PRInt"
     Prints all the voltages, currents, and calibration constants in
     volatile memory.

&lt;ONA&gt; "ON All"
     Turns on all currents to the values stored in volatile memory.

&lt;OFA&gt; "OFf All"
     Turns on all currents to zero, but does not delete the values stored
     in volatile memory.

&lt;ONN&gt; "ON Negative"
     Turns on all currents to the negative of the values stored in
     volatile memory.

&lt;RES&gt; "RESet"
     Resets all voltages to zero, all calibration constants to default,
     and writes them all to the EEPROM.  Obviously this means that
     everything that was in the EEPROM is lost.

&lt;REA&gt; "REAd eeprom"
     Reads all voltages and calibration constants from EEPROM into
     volatile memory.

&lt;WRI&gt; "WRIte eeprom"
     Writes all voltages and calibration constants from volatile memory
     to EEPROM.  The values stored to EEPROM will automatically be read
     into volatile memory on next reboot or by connection made to arduino
     by serial port.

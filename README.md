# DS3232_Testing

This arduino program can be used to test the DS3231/DS3232 RTC modules commonly found on EBay.

# Requirements
* Requires an Arduino Uno
  * Uses the SDA/SCL pins of the UNO
  * Uses interrupts specific to the UNO
* Need a DS3231/DS3232 RTC module connected to the UNO
* To be able to send text to the serial bus of the Arduino, ie use the Arduino IDE Serial Monitor

# Libraries
* [Arduino DS3232RTC Library by Jack Christensen](https://github.com/JChristensen/DS3232RTC)
* Time and TimeAlarms by Michael Margolis
* [Streaming library by Mikal Hart](http://arduiniana.org/libraries/streaming/)

# Usage

To send commands the first character is the command key character.
	 Example: t,17,2,14,20,30,0 would set the time to Feb 14, 2017 20:30:00

To change time send: t,yy,m,d,h,m,s
	 where yy=year, m=month, d=day, h=hour, m=minutes, s=seconds

To set alarm send: a,n,d,h,m,s
	 where n = 1 or 2 depending on which alarm to set
	 d=daydate, h=hour, m=minute, s=seconds
	 Example: a,1,0,20,30,0

To change alarm type send: m,ALARM_TYPES
	 Where ALARM_TYPES can be ALM1_EVERY_SECOND, ALM1_MATCH_SECONDS, ALM1_MATCH_MINUTES,
	 ALM1_MATCH_HOURS (default), ALM1_MATCH_DATE, ALM1_MATCH_DAY
	 ALM2_EVERY_MINUTE, ALM2_MATCH_MINUTES, ALM2_MATCH_HOURS (default),
	 ALM2_MATCH_DATE, ALM2_MATCH_DAY
	 Example: m,ALM1_MATCH_SECONDS

To store a byte value (255 or less) into sram send: s,byte
	Example: s,99
To store a byte value (255 or less) into a specific sram address (20-255) send: S,byte,addr
	Example: S,99,20
To read the value in address (20-255) from sram send: r,addr
	Example: r,20

To store a byte value (255 or less) into external eeprom send: e,byte
	Example: e,99
To store a byte value (255 or less) into a specific external eeprom address send: E,byte,addr
	Example: E,99,0
To read the value in address (0-4096) from external eeprom send: q,addr
	Example: q,0

To print out all RTC registers send: d
	Example: d
To print out all RTC registers and SRAM send: D
	Example: D
To print out this message again send: h
	Example: h

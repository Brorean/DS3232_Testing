
#include <Streaming.h>
#include <Wire.h>
#include <DS3232RTC.h>
#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <avr/sleep.h>
#include "AT24C32.h"

const uint8_t ALARM_PIN = 2;
const uint8_t ALARM_INTERRUPT = 0;
boolean       g_alarm_int_flag = false;
uint8_t       g_sram_addr = SRAM_START_ADDR;
boolean       g_sram_present = false;
boolean       g_eeprom_present = false;
int           g_eeprom_addr = 0x00;
const int     EEPROM_MAX_ADDR = 1000;
ALARM_TYPES_t g_alarm_1_type = ALM1_MATCH_HOURS;
ALARM_TYPES_t g_alarm_2_type = ALM2_MATCH_HOURS;

boolean is_sram_present();
void check_serial_for_data();
void wake_up_from_alarm();
void put_to_sleep();
void handle_alarm_interrupt();
void init_rtc_time();
void print_rtc_sram_values(boolean print_all_sram);
void printDateTime(time_t t);
void printTime(time_t t);
void printDate(time_t t);
void printI00(int val, char delim);

void setup() {
  time_t t;
  Serial.begin(115200);
  
  print_help();
  init_rtc_time();

  if (is_sram_present()) {
    Serial.println(F("SRAM is available"));
    g_sram_present = true;
  } else {
    Serial.println(F("SRAM is not available"));
    g_sram_present = false;
  }
  
  uint8_t rv = ee.is_present();
  if (rv == 0) {
    Serial.println(F("EEPROM is available"));
    g_eeprom_present = true;
  } else {
    Serial << F("EEPROM is not available, status = ") << rv << endl; 
    g_eeprom_present = false;
  }
  
  pinMode(ALARM_PIN, INPUT);

  t = now();
  printDateTime(t);
  print_temp();
}

void loop() {
  if (g_alarm_int_flag) {
    g_alarm_int_flag = false;
    init_rtc_time();
    handle_alarm_interrupt();  
  } else if (Serial.available()) {
    delay(20);
    init_rtc_time();
    check_serial_for_data();
  }
  
  Serial.flush();
  put_to_sleep();
}

void print_temp() {
  float c = RTC.temperature() / 4.;
  float f = c * 9. / 5. + 32.;
  Serial << "  " << c << " C  " << f << " F" << endl;  
}

void check_serial_for_data() {
  time_t t;
  tmElements_t tm;
  byte retval = 0;
  uint8_t value = 0;
  uint8_t sram_byte = 0;
  uint8_t sram_addr = 0;
  uint16_t eprom_int = 0;
  uint16_t eprom_addr = 0;

  //check for input to set the RTC, minimum length is 12, i.e. t,yy,m,d,h,m,s
  char cmd_op = (char)Serial.read();
  if (Serial.available() > 0)
    char comma = (char)Serial.read();
  if (cmd_op == 't' || cmd_op == 'T') {
    if(Serial.available() >= 12) {
      //note that the tmElements_t Year member is an offset from 1970,
      //but the RTC wants the last two digits of the calendar year.
      //use the convenience macros from Time.h to do the conversions.
      int y = Serial.parseInt();
      if (y >= 100 && y < 1000)
        Serial << F("Error: Year must be two digits or four digits!") << endl;
      else {
        if (y >= 1000)
          tm.Year = CalendarYrToTm(y);
        else    //(y < 100)
          tm.Year = y2kYearToTm(y);
        tm.Month = Serial.parseInt();
        tm.Day = Serial.parseInt();
        tm.Hour = Serial.parseInt();
        tm.Minute = Serial.parseInt();
        tm.Second = Serial.parseInt();
        t = makeTime(tm);
        RTC.set(t);        //use the time_t value to ensure correct weekday is set
        setTime(t);        
        Serial << F("RTC set to: ");
        printDateTime(t);
        Serial << endl;
        //dump any extraneous input
        while (Serial.available() > 0) Serial.read();
      }      
    } else {
      Serial.println(F("The time set command appears to be invalid"));
    }
  } else if (cmd_op == 'a' || cmd_op == 'A') {
      if (Serial.available() >= 9) {
      //format a,n,d,hh,mm,ss
      //where n is alarm 1 or 2
      uint8_t alarm_num = Serial.parseInt();
      uint8_t alarm_daydate = Serial.parseInt();
      uint8_t alarm_hour = Serial.parseInt();
      uint8_t alarm_min = Serial.parseInt();
      uint8_t alarm_sec = Serial.parseInt();
      //dump any extraneous input
      while (Serial.available() > 0) Serial.read();
      if (alarm_num == 1) {
        RTC.setAlarm(g_alarm_1_type, alarm_sec, alarm_min, alarm_hour, alarm_daydate);
        RTC.alarmInterrupt(ALARM_1, true);
        Serial << F("Setting ALARM_1: ") << alarm_daydate << F("\t") << alarm_hour << F(":") << alarm_min << F(":") << alarm_sec << endl;
      } else {
        RTC.setAlarm(g_alarm_2_type, alarm_sec, alarm_min, alarm_hour, alarm_daydate);
        RTC.alarmInterrupt(ALARM_2, true);
        Serial << F("Setting ALARM_2: ") << alarm_daydate << F("\t") << alarm_hour << F(":") << alarm_min << F(":") << alarm_sec << endl;
      }
    } else {
      Serial.println(F("The alarm set command appears to be invalid"));
    }
  } else if (cmd_op == 'm' || cmd_op == 'M') {
    if (Serial.available() >= 14) {
      String alarm_type = Serial.readString();
      Serial << "Alarm type: " << alarm_type << endl;
      if (alarm_type == "ALM1_EVERY_SECOND") {
        g_alarm_1_type = ALM1_EVERY_SECOND;
      } else if (alarm_type == "ALM1_MATCH_SECONDS") {
        g_alarm_1_type = ALM1_MATCH_SECONDS;
      } else if (alarm_type == "ALM1_MATCH_MINUTES") {
        g_alarm_1_type = ALM1_MATCH_MINUTES;
      } else if (alarm_type == "ALM1_MATCH_HOURS") {
        g_alarm_1_type = ALM1_MATCH_HOURS;
      } else if (alarm_type == "ALM1_MATCH_DATE") {
        g_alarm_1_type = ALM1_MATCH_DATE;
      } else if (alarm_type == "ALM1_MATCH_DAY") {
        g_alarm_1_type = ALM1_MATCH_DAY;
      } else if (alarm_type == "ALM2_EVERY_MINUTE") {
        g_alarm_2_type = ALM2_EVERY_MINUTE;
      } else if (alarm_type == "ALM2_MATCH_MINUTES") {
        g_alarm_2_type = ALM2_MATCH_MINUTES;
      } else if (alarm_type == "ALM2_MATCH_HOURS") {
        g_alarm_2_type = ALM2_MATCH_HOURS;
      } else if (alarm_type == "ALM2_MATCH_DATE") {
        g_alarm_2_type = ALM2_MATCH_DATE;
      } else if (alarm_type == "ALM2_MATCH_DAY") {
        g_alarm_2_type = ALM2_MATCH_DAY;
      }
      Serial << F("ALARM_1_TYPE: ") << _HEX(g_alarm_1_type) << endl;
      Serial << F("ALARM_2_TYPE: ") << _HEX(g_alarm_2_type) << endl;
    }
  } else if  (cmd_op == 's') {
    if (g_sram_present) {
      sram_byte = Serial.parseInt();
      retval = RTC.writeRTC(g_sram_addr, &sram_byte, 1);
      Serial << F("Writing ") << sram_byte << F(" to address ") << g_sram_addr << F(" Status: ") << retval << endl;
      if (g_sram_addr == 0xFF) {
        g_sram_addr = SRAM_START_ADDR;
      } else {
        g_sram_addr++;
      }
    } else {
      Serial.println(F("SRAM is not available"));
    }
  } else if (cmd_op == 'S') {
    if (g_sram_present) {
      sram_byte = Serial.parseInt();
      sram_addr = Serial.parseInt();
      if (sram_addr >= SRAM_START_ADDR && sram_addr <= 0xFF) {
        retval = RTC.writeRTC(g_sram_addr, &sram_byte, 1);
        Serial << F("Writing ") << sram_byte << F(" to address ") << g_sram_addr << F(" Status: ") << retval << endl;
      } else {
        Serial << F("Invalid sram address: ") << sram_addr << endl;
      }
    } else {
      Serial.println(F("SRAM is not available"));
    }
  } else if (cmd_op == 'r' || cmd_op == 'R') {
    if (g_sram_present) {
      sram_byte = Serial.parseInt();
      if ((sram_byte >= SRAM_START_ADDR) && (sram_byte <= 0xFF)) {
        value = RTC.readRTC(sram_byte);
        Serial << F("Value ") << value << F(" was in address ") << sram_byte << endl; 
      } else {
        Serial << F("Invalid address: ") << sram_byte << endl;
      }
    } else {
      Serial.println(F("SRAM is not available"));
    }
  } else if (cmd_op == 'e') {
    if (g_eeprom_present) {
      eprom_int = Serial.parseInt();
      retval = ee.write_mem(g_eeprom_addr, eprom_int);
      Serial << F("Writing ") << eprom_int << F(" to address ") << g_eeprom_addr << F(" Status: ") << retval << endl;
      if (g_eeprom_addr == EEPROM_MAX_ADDR) {
        g_eeprom_addr = 0x00;
      } else {
        g_eeprom_addr++;
      }
    } else {
      Serial.println(F("EEPROM is not available"));
    }
  } else if (cmd_op == 'E') {
    if (g_eeprom_present) {
      eprom_int = Serial.parseInt();
      eprom_addr = Serial.parseInt();
      if (eprom_addr >= 0x00 && eprom_addr <= EEPROM_MAX_ADDR) {
        retval == ee.write_mem(eprom_addr, eprom_int);
        Serial << F("Writing ") << eprom_int << F(" to address ") << eprom_addr << F(" Status: ") << retval << endl;
      } else {
        Serial << F("Invalid address: ") << eprom_addr << endl;
      }
    } else {
      Serial.println(F("EEPROM is not available"));
    }
  } else if (cmd_op == 'q') {
    if (g_eeprom_present) {
      eprom_int = Serial.parseInt();
      int num_bytes = ee.read_mem(eprom_int, &value, 1);
      Serial << F("Value ") << value << F(" was in address ") << eprom_int << endl;
    } else {
      Serial.println(F("EEPROM is not available"));
    }
  } else if (cmd_op == 'd') {
    print_rtc_sram_values(false);
  } else if (cmd_op == 'D') {
    print_rtc_sram_values(true);
  } else if (cmd_op == 'h' || cmd_op == 'H') {
    print_help();
  }
  
  //dump any extraneous input
  while (Serial.available() > 0) Serial.read();
}

boolean is_sram_present() {
  uint8_t sram_byte = 0x63;
  uint8_t retval = 0;
  retval = RTC.writeRTC(SRAM_START_ADDR, &sram_byte, 1);
  if (retval == 0)
    return true;
  else
    return false;    
}

/* Function for the alarm interrupt */
void wake_up_from_alarm() {
  sleep_disable();
  detachInterrupt(ALARM_INTERRUPT);
  g_alarm_int_flag = true;
}

void handle_alarm_interrupt() {
  time_t t = now();
  
  if (RTC.alarm(ALARM_1)) {
    Serial.print(F("ALARM_1 triggered\t"));
    printDateTime(t);
    Serial.println("");
  } else if (RTC.alarm(ALARM_2)) {
    Serial.print(F("ALARM_2 triggered\t"));
    printDateTime(t);
    Serial.println("");
  } else {
    Serial << F("Unknown") << endl;
  }
}

void put_to_sleep() {
  PRR = PRR | 0b00100000; //disable timer
  set_sleep_mode(SLEEP_MODE_IDLE);  //used for debugging, don't want to shutdown all functions
  cli();
  sleep_enable();
  sei();
  attachInterrupt(ALARM_INTERRUPT, wake_up_from_alarm, LOW);
  sleep_cpu();

  //wakeup starts here
  sleep_disable();
  PRR = PRR & 0b00000000; //enable timer
}

/* used for debugging the RTC */
void print_rtc_sram_values(boolean print_all_sram) {
  uint16_t addr = 0;
  uint8_t values[SRAM_START_ADDR + SRAM_SIZE]; //could use malloc
  uint16_t range = 0;
  if (print_all_sram) {
    range = SRAM_START_ADDR + SRAM_SIZE;  
  } else {
    range = SRAM_START_ADDR - 1;
  }
  
  Serial.println("Addr: DEC, HEX, BIN");
  RTC.readRTC(addr, values, range);
  for (addr = 0; addr < range; addr++) {
    Serial.print(addr, HEX);
    Serial.print(": ");
    Serial.print(values[addr], DEC);
    Serial.print(", ");
    Serial.print(values[addr], HEX);
    Serial.print(", ");
    Serial.println(values[addr], BIN);
  }
}

void init_rtc_time() {
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet) {
    Serial.println(F("Time set failed"));
  }  
}

//print date and time to Serial
void printDateTime(time_t t) {
    printDate(t);
    Serial << ' ';
    printTime(t);
}

//print time to Serial
void printTime(time_t t) {
    printI00(hour(t), ':');
    printI00(minute(t), ':');
    printI00(second(t), ' ');
}

//print date to Serial
void printDate(time_t t) {
    printI00(day(t), 0);
    Serial << monthShortStr(month(t)) << _DEC(year(t));
}

//Print an integer in "00" format (with leading zero),
//followed by a delimiter character to Serial.
//Input value assumed to be between 0 and 99.
void printI00(int val, char delim) {
    if (val < 10) Serial << '0';
    Serial << _DEC(val);
    if (delim > 0) Serial << delim;
    return;
}

void print_help() {
  Serial.println(F("To send commands the first character is the command key character."));
  Serial.println(F("\t Example: t,17,2,14,20,30,0 would set the time to Feb 14, 2017 20:30:00\n"));
  
  Serial.println(F("To change time send: t,yy,m,d,h,m,s"));
  Serial.println(F("\t where yy=year, m=month, d=day, h=hour, m=minutes, s=seconds\n"));
  
  Serial.println(F("To set alarm send: a,n,d,h,m,s"));
  Serial.println(F("\t where n = 1 or 2 depending on which alarm to set"));
  Serial.println(F("\t d=daydate, h=hour, m=minute, s=seconds"));
  Serial.println(F("\t Example: a,1,0,20,30,0\n"));
  
  Serial.println(F("To change alarm type send: m,ALARM_TYPES"));
  Serial.println(F("\t Where ALARM_TYPES can be ALM1_EVERY_SECOND, ALM1_MATCH_SECONDS, ALM1_MATCH_MINUTES,"));
  Serial.println(F("\t ALM1_MATCH_HOURS (default), ALM1_MATCH_DATE, ALM1_MATCH_DAY"));
  Serial.println(F("\t ALM2_EVERY_MINUTE, ALM2_MATCH_MINUTES, ALM2_MATCH_HOURS (default),"));
  Serial.println(F("\t ALM2_MATCH_DATE, ALM2_MATCH_DAY"));
  Serial.println(F("\t Example: m,ALM1_MATCH_SECONDS\n"));
  
  Serial.println(F("To store a byte value (255 or less) into sram send: s,byte"));
  Serial.println(F("\tExample: s,99"));
  Serial.println(F("To store a byte value (255 or less) into a specific sram address (20-255) send: S,byte,addr"));
  Serial.println(F("\tExample: S,99,20"));
  Serial.println(F("To read the value in address (20-255) from sram send: r,addr"));
  Serial.println(F("\tExample: r,20\n"));
  
  Serial.println(F("To store a byte value (255 or less) into external eeprom send: e,byte"));
  Serial.println(F("\tExample: e,99"));
  Serial.println(F("To store a byte value (255 or less) into a specific external eeprom address send: E,byte,addr"));
  Serial.println(F("\tExample: E,99,0"));
  Serial.println(F("To read the value in address (0-4096) from external eeprom send: q,addr"));
  Serial.println(F("\tExample: q,0\n"));
  
  Serial.println(F("To print out all RTC registers send: d"));
  Serial.println(F("\tExample: d"));
  Serial.println(F("To print out all RTC registers and SRAM send: D"));
  Serial.println(F("\tExample: D"));
  Serial.println(F("To print out this message again send: h"));
  Serial.println(F("\tExample: h\n"));
}


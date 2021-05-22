/* -*- tab-width: 2; mode: c; -*-
 * 
 * Alternative firmware for OpenLog loggers for recording
 * NMEA messages from GPSs.
 * 
 * Configures the GPS on startup.
 *
 * Copyright (c) 2021, Steve Jack.
 *
 * MIT licence.
 *
 * Notes
 *
 * OpenLog: Tested on a SparkFun unit.
 *          Don't let the free RAM go below 600.
 *
 * STM32F1: Won't compile with the standard SD library.
 *          Works with SdFat 2.0.6.
 *          Turn on USB Serial if using SdFat as it wants a Serial.
 *          Set U(S)ART to no generic serial.
 *
 * SAMD21:  Works with the standard SD library and SdFat 2.0.6. 
 *
 * ESP32:   Works with the standard library.
 *          SdFat 2.0.6 doesn't seem to work.
 *
 * ESP8266: WeMos D1 Mini.
 *          Seems to reboot after most writes to the SD card.
 *          Tried both the standard and SdFat libraries.
 *          Needs a reset on powerup to get the comms to the 
 *          GPS going.
 *
 */

#pragma GCC diagnostic warning "-Wunused-variable"

#include <Arduino.h>

//

#define CONFIG_GPS            1
#define RX_BUFFER_SIZE      100

// Processor specific configuration.

#if defined(ARDUINO_ARCH_ESP32)
#define USE_SDFAT             0
#define FILE_MODE   FILE_APPEND
#else
#define USE_SDFAT             1
#define FILE_MODE    FILE_WRITE
#endif

#if defined(ARDUINO_ARCH_SAMD)

#define SD_BUFFER_SIZE     4096
#define GPS_SERIAL      Serial1
#define SD_CS                 7
#define BAUD_RATE         38400
#define GPS_RATE            500 // ms
// #define DEBUG_SERIAL  SerialUSB
#define PASSTHROUGH   SerialUSB
#define PASSTHROUGH_BAUD 115200
#define FIX_LED               5

const int status_LED = 6, LED_sense = 0;

#elif defined(ARDUINO_AVR_PRO) || defined(ARDUINO_AVR_UNO)

#define SD_BUFFER_SIZE      256
#define GPS_SERIAL       Serial
#define SD_CS                10
#define BAUD_RATE         19200
#define GPS_RATE            500 // ms
#define FIX_LED               5

const int status_LED = 6, LED_sense = 0;

#elif defined(ARDUINO_ESP32_DEV)

#define SD_BUFFER_SIZE     4096
#define GPS_SERIAL      Serial2
#define BAUD_RATE         38400
#define GPS_RATE            500 // ms
#define DEBUG_SERIAL     Serial
// #define PASSTHROUGH      Serial
#define PASSTHROUGH_BAUD 115200

const int status_LED = 2, LED_sense = 0;

#elif defined(ARDUINO_ESP8266_WEMOS_D1MINI)

#warning "Doesn't work properly on the ESP8266 WeMos D1 Mini."

#define SD_BUFFER_SIZE     4096
#define GPS_SERIAL       Serial
#define SD_CS                D8
#define BAUD_RATE         19200
#define GPS_RATE            500 // ms

// Don't use D4 for the status LED.
const int status_LED = -1, LED_sense = 0;

#elif defined(ARDUINO_BLUEPILL_F103C8) || defined(ARDUINO_BLUEPILL_F103CB)

// Works well on the Blue Pill. 

#define SD_BUFFER_SIZE     4096
#define GPS_SERIAL      Serial1
#define SD_CS               PA4
#define BAUD_RATE         57600
#define GPS_RATE            200 // ms
#define DEBUG_SERIAL    Serial2
#define PASSTHROUGH     Serial3
#define PASSTHROUGH_BAUD 115200

const int status_LED = PC13, LED_sense = 1;

HardwareSerial Serial1(PA10,PA9);
HardwareSerial Serial2(PA3,PA2);
HardwareSerial Serial3(PB11,PB10);

#else

#error "No configuration for this processor."

#endif

//

#include <SPI.h>

#if USE_SDFAT
#include <SdFat.h>
#else
#include <SD.h>
#endif

//

void config_gps(void);
void ublox_checksum(uint8_t *,uint8_t *,uint8_t *);
void set_status_LED(int);

void dateTimeCallback(uint16_t*,uint16_t*,uint8_t*);

//

static int      file_index = 0, SD_enabled = 0, est_write_rate = 0;
static char     sd_buffer[SD_BUFFER_SIZE];
static uint8_t  rx_buffer[RX_BUFFER_SIZE];
static uint16_t hours = 0, minutes = 0, seconds = 0, years = 0, months = 0, days = 0;

#if USE_SDFAT
SdFat           SD;
#endif

/*
 *
 */

void setup() {

  int         i;
  char        text[128], text2[32];
  File        root, file;

  text[0]  = i = 0;
  text2[0] = text2[sizeof(text2) - 1] = 0;
    
  //

  est_write_rate = (int) ((1000l * (long int) SD_BUFFER_SIZE) / (200l * 1000l / GPS_RATE)); // ms between writes.

  if (status_LED >= 0) {

    pinMode(status_LED,OUTPUT);
    digitalWrite(status_LED,1 ^ LED_sense);
  }

#if defined(FIX_LED)
  pinMode(FIX_LED,OUTPUT);
  digitalWrite(FIX_LED,0);
#endif

#if defined(PASSTHROUGH)
  PASSTHROUGH.begin(PASSTHROUGH_BAUD);
#endif

#if defined(DEBUG_SERIAL)

  DEBUG_SERIAL.begin(115200);

  for (i = 0; i < 10; ++i) {

    if (DEBUG_SERIAL) {
      break;
    }

    delay(100);
  }

  delay(500);

  DEBUG_SERIAL.print("\r\nGPS NMEA Logger\r\n");
  DEBUG_SERIAL.print(__DATE__);
  sprintf(text,"\r\n\nEst. %d ms between writes\r\n\n",est_write_rate);
  DEBUG_SERIAL.print(text);

#endif

  SPI.begin();

  memset(sd_buffer,0,SD_BUFFER_SIZE);
  memset(rx_buffer,0,RX_BUFFER_SIZE);

#if CONFIG_GPS

  config_gps();

#else

  GPS_SERIAL.begin(BAUD_RATE);
  
#endif

#if USE_SDFAT
  FsDateTime::setCallback(dateTimeCallback);
  // pinMode(SD_CS,OUTPUT);
#endif

#if defined(SD_CS)
  if (SD.begin(SD_CS)) {
#else
  if (SD.begin()) {
#endif

    SD_enabled = 1;

#if defined(DEBUG_SERIAL)

    delay(100);

    if (root = SD.open("/")) {

      while (file = root.openNextFile()) {

#if USE_SDFAT
        file.getName(text2,sizeof(text2) - 1);
        sprintf(text,"%-30s %12u\r\n",text2,file.size());
#else
        sprintf(text,"%-30s %12u\r\n",file.name(),file.size());
#endif
        DEBUG_SERIAL.print(text);
        file.close();
      }

      root.close();
    }

  } else {
  
    DEBUG_SERIAL.print("SD.begin() failed\r\n");
#endif
  }

  set_status_LED(0);

  return;
}

/*
 *
 */

void loop() {

  int              i, len;
  char             text[96], *rx_buffer_s;
  uint8_t          u8;
  uint16_t         u16;
  static int       ublox_length = 0, commas = 0;
  static char      filename[16] = {0}, nmea_csum_s[4]= {0}, *section, *check;
  static int16_t   rx_index = 0, sd_index = 0, read_mode = 0, 
                   satellites = 0, fix = 0;
  static uint16_t  nmea_csum = 0;
  static uint32_t  msecs, last_write = 0;
  static File      output;
#if defined (FIX_LED)
  static uint8_t   led_phase = 0;
  static uint32_t  last_led_msecs = 0;
#endif

  u16         = 0;
  rx_buffer_s = (char *) rx_buffer;
  
  //

  for (i = 0; (i < 128)&&(GPS_SERIAL.available()); ++i) {

    u8 = rx_buffer[rx_index] = GPS_SERIAL.read();

#if defined(PASSTHROUGH)
    PASSTHROUGH.write(u8);
#endif

    switch (read_mode) {

    case 0:

      // NMEA starts with a $
      if (u8 == '$') {

        read_mode = 1;
        commas    = 0;
        nmea_csum = 0;
        section   = 
        check     = rx_buffer_s; 

      // and Ublox starts with 0xb5. 
      } else if (u8 == 0xb5) {

        read_mode    = 2;
        ublox_length = 8;
      }

      break;

    case 1: // NMEA

      switch (u8) {

      case ',':

        ++commas;

        if ((rx_buffer_s[3] == 'R')&&(rx_buffer_s[4] == 'M')&&(rx_buffer_s[5] == 'C')) {

          switch (commas) {

          case  2:
            strncpy(text,section,8);

            text[6] = 0; seconds = atoi(&text[4]);
            text[4] = 0; minutes = atoi(&text[2]);
            text[2] = 0; hours   = atoi(text);

            break;

          case 10:
            strncpy(text,section,6);

            text[6] = 0; years  = atoi(&text[4]) + 2000;
            text[4] = 0; months = atoi(&text[2]);
            text[2] = 0; days   = atoi(text);

            break;

          default:
            break;
          }
        }
        
        if ((rx_buffer_s[3] == 'G')&&(rx_buffer_s[4] == 'G')&&(rx_buffer_s[5] == 'A')) {

          switch (commas) {

          case  7:
            fix = atoi(section);
            break;

          case  8:
            satellites = atoi(section);
            break;

          default:
            break;
          }
        }
        
        section    = &rx_buffer_s[rx_index + 1];
        nmea_csum ^= u8;
        break;

      case 10: // LF

#if defined(DEBUG_SERIAL) && 0
        sprintf(text,"\r %1d %3d %3d %1d %02d %2s %c%c %c \r",
                SD_enabled,rx_index,sd_index,fix,satellites,
                nmea_csum_s,check[0],check[1],filename[0]);
        DEBUG_SERIAL.print(text);
#endif
        rx_buffer[len = (rx_index + 1)] = 0;

#if 1
        // Only log records with a valid checksum.

        if ((nmea_csum_s[0] == check[0])&&
            (nmea_csum_s[1] == check[1])) {
#else
        if (1) {
#endif
          if ((SD_enabled)&&(fix)&&(filename[0])) {

            // Is there room in the SD buffer?

            if (len > (SD_BUFFER_SIZE - 4 - sd_index)) {

              set_status_LED(1);

              if (!output) {

                output = SD.open(filename,FILE_MODE);
              }

              if (output) {

                output.write((uint8_t *) sd_buffer,sd_index);

                if (est_write_rate > 2000) {
                  
                  output.close();

                } else {

                  output.flush();
                }
                
                last_write = millis();
              }

              set_status_LED(0);

              sd_index = 0;
            }

            strcpy((char *) &sd_buffer[sd_index],rx_buffer_s);

            sd_index += len;
          }
        }
 
        rx_index = read_mode = 0;
        break;

      case '*':

        check = &rx_buffer_s[rx_index + 1];
        sprintf(nmea_csum_s,"%02X",nmea_csum);
        break;

      default:

        nmea_csum ^= u8;
        break;
      }
      
      break;

    case 2: // Ublox

      if (rx_index == 4) {

        ublox_length = u8 + 8;
        
      } else if (rx_index >= (ublox_length - 1)) {

        rx_index = read_mode = 0;
      }
  
      break;
    }

    if (read_mode) {

      if (++rx_index > (RX_BUFFER_SIZE - 4)) {

        rx_index = read_mode = 0;
      }
    }
  }

#if defined(PASSTHROUGH)

  for (i = 0; (i < 128)&&(PASSTHROUGH.available()); ++i) {

    GPS_SERIAL.write(PASSTHROUGH.read());
  }

#endif

  // This is probably redundant.

  if ((output)&&(((msecs = millis()) - last_write) > 10000)) {

    output.close();
  }

  // Once we have the date, sort out the output file name.

  if ((!filename[0])&&(years > 2000)&&(satellites > 3)) {

    File file;

    for (i = 0; i < 1000; ++i, ++file_index) {

      sprintf(filename,"/%04d%02d%02d.%03d",years,months,days,file_index % 1000);

      if (file = SD.open(filename,FILE_READ)) {

        file.close();

      } else {

        break;
      }
    }
    
#if defined(DEBUG_SERIAL)
    sprintf(text,"\r\nLog file: \'%s\'\r\n",filename);
    DEBUG_SERIAL.print(text);
#endif

#if defined(PASSTHROUGH)
    
    sprintf(text,"$GNTXT,01,01,02,%s",filename);

    for (i = 1, u16 = 0; text[i]; ++i) {

      u16 ^= text[i];
    }

    sprintf(&text[i],"*%02X\r\n",u16);

    PASSTHROUGH.print(text);

#endif
  }

#if defined(FIX_LED)

  if (((msecs = millis()) - last_led_msecs) > 100) {

    last_led_msecs = msecs;

    switch (led_phase) {

    case 1:
    case 3:
    case 5:
    case 7:

      if ((satellites - 4) > led_phase) {
        digitalWrite(FIX_LED,1);
      }
      break;

    default:

      digitalWrite(FIX_LED,0);  
      break;
    }

    if (++led_phase > 9) {

      led_phase = 0;
    }
  }

#endif

  return;
}

/*
 * 
 */

void set_status_LED(int state) {

  if (status_LED >= 0) {

    digitalWrite(status_LED,state ^ LED_sense);
  }

  return;
}

/*
 * Send commands to the GPS to configure it for 3D airborne fixes and
 * the required baud and update rates.
 * (Factory settings are auto 2D/3D, 9600 and 1Hz.)
 * 
 */

#if CONFIG_GPS

void config_gps() {

  int             i, j;
  uint8_t        *tx_buffer;
  const uint8_t  
    ubx_cfg_rate[12] = "\x06\x08\x06\x00\xc8\x00\x01\x00\x01\x00",
    ubx_nav5_cfg[12] = "\x06\x24\x24\x00\x05\x00\x07\x02\x00\x00",
    ubx_cfg_prt[26]  = "\x06\x00\x14\x00\x01\x00\x00\x00\xd0\x08\x00\x00\x00\x4b\x00\x00\x03\x00\x03\x00\x00\x00\x00\x00",
    ubx_cfg_msg[14]  = "\x06\x01\x08\x00\xf0\x02\x00\x00\x00\x00\x00\x01";
  struct messages {
    uint8_t msg_class; 
    uint8_t msg_id; 
    uint8_t msg_rate;};
  const struct messages 
    gps_msgs[16] = {{0xf0, 0x0a,  0}, {0xf0, 0x09,  0}, {0xf0, 0x00,  1}, {0xf0, 0x01,  0}, // NMEA
                    {0xf0, 0x40,  0}, {0xf0, 0x06,  0}, {0xf0, 0x02, 10}, {0xf0, 0x07,  0}, 
                    {0xf0, 0x03,  0}, {0xf0, 0x04,  1}, {0xf0, 0x0e,  0}, {0xf0, 0x41, 10}, 
                    {0xf0, 0x05,  0}, {0xf0, 0x08,  0},
                    {0x00, 0x00,  0}};   

  //

  tx_buffer    = rx_buffer;
  tx_buffer[0] = 0xb5;
  tx_buffer[1] = 0x62;

  //
  
  GPS_SERIAL.begin(9600);

  delay(50);

#if (BAUD_RATE != 9600)

  // If we are not using the default baud rate, change it.

  memset(&tx_buffer[2],0,64);
  memcpy(&tx_buffer[2],ubx_cfg_prt,sizeof(ubx_cfg_prt));

  tx_buffer[20] = 2;
  tx_buffer[14] = 0xff &             BAUD_RATE;
  tx_buffer[15] = 0xff & ((uint32_t) BAUD_RATE >>  8);
  tx_buffer[16] = 0xff & ((uint32_t) BAUD_RATE >> 16);

  ublox_command(tx_buffer);

  GPS_SERIAL.end();

  GPS_SERIAL.begin(BAUD_RATE);

#else

  ubx_cfg_prt;

#endif

  delay(50);

  // 3D airborne

  memset(&tx_buffer[2],0,64);
  memcpy(&tx_buffer[2],ubx_nav5_cfg,sizeof(ubx_nav5_cfg));

  ublox_command(tx_buffer);

  delay(50);

  // Update rate.

  memset(&tx_buffer[2],0,64);
  memcpy(&tx_buffer[2],ubx_cfg_rate,sizeof(ubx_cfg_rate));

  tx_buffer[6] = GPS_RATE & 0xff;
  tx_buffer[7] = GPS_RATE >> 8;

  ublox_command(tx_buffer);

  // Turn off unwanted messages.

  memset(&tx_buffer[2],0,32);
  memcpy(&tx_buffer[2],ubx_cfg_msg,sizeof(ubx_cfg_msg));

  for (i = 0; gps_msgs[i].msg_class; ++i) {

    delay(50);

    tx_buffer[6] = gps_msgs[i].msg_class;
    tx_buffer[7] = gps_msgs[i].msg_id;

    for (j = 0; j < 6; ++j) {

      tx_buffer[8 + j] = gps_msgs[i].msg_rate;
    }

    ublox_command(tx_buffer);
  }

  //
  
  return;
}

//

void ublox_command(uint8_t *command) {

  int     l;
  uint8_t ck_a, ck_b;

  l = command[4] + 4;

  ublox_checksum(command,&ck_a,&ck_b);

  command[l + 2] = ck_a;
  command[l + 3] = ck_b;

  GPS_SERIAL.write(command,l + 4);
  GPS_SERIAL.flush();

  return;
}

//

void ublox_checksum(uint8_t *buffer,uint8_t *ck_a,uint8_t *ck_b) {

  int i, l;

  *ck_a = *ck_b = 0;
  
  l =  4 + buffer[4];

  for (i = 0; i < l; ++i) {

    *ck_a += buffer[2 + i];
    *ck_b += *ck_a;
  }

  return;
} 

#endif

/*
 *
 */

#if USE_SDFAT

void dateTimeCallback(uint16_t* date,uint16_t* time,uint8_t* ms10) {

  *date = FS_DATE(years,months,days);
  *time = FS_TIME(hours,minutes,seconds);
  *ms10 = 0;

  return;
}

#endif

/*
 *
 */

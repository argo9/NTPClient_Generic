/****************************************************************************************************************************
  RTC_Ethernet_NTPClient.ino

  Self-adjusting clock for one time zone using an external DS3231 RTC
  
  For AVR, ESP8266/ESP32, SAMD21/SAMD51, nRF52, STM32, SAM DUE, WT32_ETH01, RTL8720DN boards using 
  a) Ethernet W5x00, ENC28J60, LAN8742A
  b) WiFiNINA
  c) ESP8266/ESP32 WiFi
  d) ESP8266/ESP32-AT-command WiFi
  e) WT32_ETH01 (ESP32 + LAN8720)
  f) RTL8720DN

  Based on and modified from Arduino NTPClient Library (https://github.com/arduino-libraries/NTPClient)
  to support other boards such as ESP8266/ESP32, SAMD21, SAMD51, Adafruit's nRF52 boards, SAM DUE, RTL8720DN, etc.
  and Ethernet/WiFi/WiFiNINA shields
  
  Copyright (C) 2015 by Fabrice Weinberg and licensed under MIT License (MIT)

  Built by Khoi Hoang https://github.com/khoih-prog/NTPClient_Generic
  Licensed under MIT license
 *****************************************************************************************************************************/

#include "defines.h"

// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
#include <Timezone_Generic.h>             // https://github.com/khoih-prog/Timezone_Generic

#include <DS323x_Generic.h>               // https://github.com/khoih-prog/DS323x_Generic

DS323x rtc;

//////////////////////////////////////////

// US Eastern Time Zone (New York, Detroit)
TimeChangeRule myDST = {"EDT", Second, Sun, Mar, 2, -240};    //Daylight time = UTC - 4 hours
TimeChangeRule mySTD = {"EST", First, Sun, Nov, 2, -300};     //Standard time = UTC - 5 hours
Timezone *myTZ;

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev

//////////////////////////////////////////

// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
#include <NTPClient_Generic.h>          // https://github.com/khoih-prog/NTPClient_Generic

// A UDP instance to let us send and receive packets over UDP
EthernetUDP ntpUDP;

// NTP server
//World
//char timeServer[] = "time.nist.gov";
// Canada
char timeServer[] = "0.ca.pool.ntp.org";
//char timeServer[] = "1.ca.pool.ntp.org";
//char timeServer[] = "2.ca.pool.ntp.org";
//char timeServer[] = "3.ca.pool.ntp.org";
// Europe
//char timeServer[] = ""europe.pool.ntp.org";

#define TIME_ZONE_OFFSET_HRS            (-4)
#define NTP_UPDATE_INTERVAL_MS          60000L

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, timeServer, (3600 * TIME_ZONE_OFFSET_HRS), NTP_UPDATE_INTERVAL_MS);

static bool gotCurrentTime = false;

// format and print a time_t value, with a time zone appended.
void printDateTime(time_t t, const char *tz)
{
  char buf[32];
  char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)
  strcpy(m, monthShortStr(month(t)));
  sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s",
          hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t), tz);
  Serial.println(buf);
}

void update_RTC(void)
{
  // Just get the correct time once
  if (timeClient.updated())
  {
    Serial.println("********UPDATED********");
    rtc.now( DateTime(timeClient.getUTCEpochTime()) );
    gotCurrentTime = true;
  }
}

void displayRTC()
{
  // Display time from RTC
  DateTime now = rtc.now();

  Serial.println("============================");

  time_t utc = now.get_time_t();
  time_t local = myTZ->toLocal(utc, &tcr);
  
  printDateTime(utc, "UTC");
  printDateTime(local, tcr -> abbrev);
}

void check_status(void)
{
  // Update first time after 5s
  static ulong checkstatus_timeout  = 5000L;
  static ulong RTCDisplay_timeout   = 0;

  static ulong current_millis;

// Display every 10s
#define RTC_DISPLAY_INTERVAL        (10000L)

// Update RTC every hour if got correct time from NTP
#define RTC_UPDATE_INTERVAL         (3600 * 1000L)

// Retry updating RTC every 5s if haven't got correct time from NTP
#define RTC_RETRY_INTERVAL          (3 * 1000L)

  current_millis = millis();

  if ((current_millis > RTCDisplay_timeout) || (RTCDisplay_timeout == 0))
  {
    if (gotCurrentTime)
      displayRTC();
      
    RTCDisplay_timeout = current_millis + RTC_DISPLAY_INTERVAL;
  }
  
  // Update RTC every RTC_UPDATE_INTERVAL (3600) seconds.
  if ((current_millis > checkstatus_timeout))
  {
    update_RTC();
    
    if (gotCurrentTime)
    {
      Serial.println("RTC updated. Next update in seconds : " + String(RTC_UPDATE_INTERVAL/1000, DEC));
      checkstatus_timeout = current_millis + RTC_UPDATE_INTERVAL;
    }
    else
    {
      Serial.println("Retry RTC update in seconds : " + String(RTC_RETRY_INTERVAL/1000, DEC));
      checkstatus_timeout = current_millis + RTC_RETRY_INTERVAL;
    }   
  }
}

//////////////////////////////////////////

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  Serial.print("\nStart RTC_Ethernet_NTPClient on " + String(BOARD_NAME));
  Serial.println(" with " + String(SHIELD_TYPE));
  Serial.println(NTPCLIENT_GENERIC_VERSION);

  Wire.begin();

#if USE_ETHERNET_WRAPPER

  EthernetInit();

#else

#if USE_ETHERNET
  ET_LOGWARN(F("=========== USE_ETHERNET ==========="));
#elif USE_ETHERNET2
  ET_LOGWARN(F("=========== USE_ETHERNET2 ==========="));
#elif USE_ETHERNET3
  ET_LOGWARN(F("=========== USE_ETHERNET3 ==========="));
#elif USE_ETHERNET_LARGE
  ET_LOGWARN(F("=========== USE_ETHERNET_LARGE ==========="));
#elif USE_ETHERNET_ESP8266
  ET_LOGWARN(F("=========== USE_ETHERNET_ESP8266 ==========="));
#else
  ET_LOGWARN(F("========================="));
#endif

  ET_LOGWARN(F("Default SPI pinout:"));
  ET_LOGWARN1(F("MOSI:"), MOSI);
  ET_LOGWARN1(F("MISO:"), MISO);
  ET_LOGWARN1(F("SCK:"),  SCK);
  ET_LOGWARN1(F("SS:"),   SS);
  ET_LOGWARN(F("========================="));

#if defined(ESP8266)
  // For ESP8266, change for other boards if necessary
  #ifndef USE_THIS_SS_PIN
    #define USE_THIS_SS_PIN   D2    // For ESP8266
  #endif

  ET_LOGWARN1(F("ESP8266 setCsPin:"), USE_THIS_SS_PIN);

  #if ( USE_ETHERNET || USE_ETHERNET_LARGE || USE_ETHERNET2 || USE_ETHERNET_ENC )
    // For ESP8266
    // Pin                D0(GPIO16)    D1(GPIO5)    D2(GPIO4)    D3(GPIO0)    D4(GPIO2)    D8
    // Ethernet           0                 X            X            X            X        0
    // Ethernet2          X                 X            X            X            X        0
    // Ethernet3          X                 X            X            X            X        0
    // EthernetLarge      X                 X            X            X            X        0
    // Ethernet_ESP8266   0                 0            0            0            0        0
    // D2 is safe to used for Ethernet, Ethernet2, Ethernet3, EthernetLarge libs
    // Must use library patch for Ethernet, EthernetLarge libraries
    Ethernet.init (USE_THIS_SS_PIN);

  #elif USE_ETHERNET3
    // Use  MAX_SOCK_NUM = 4 for 4K, 2 for 8K, 1 for 16K RX/TX buffer
    #ifndef ETHERNET3_MAX_SOCK_NUM
      #define ETHERNET3_MAX_SOCK_NUM      4
    #endif
  
    Ethernet.setCsPin (USE_THIS_SS_PIN);
    Ethernet.init (ETHERNET3_MAX_SOCK_NUM);

  #elif USE_CUSTOM_ETHERNET
  
    // You have to add initialization for your Custom Ethernet here
    // This is just an example to setCSPin to USE_THIS_SS_PIN, and can be not correct and enough
    Ethernet.init(USE_THIS_SS_PIN);
  
  #endif  //( USE_ETHERNET || USE_ETHERNET2 || USE_ETHERNET3 || USE_ETHERNET_LARGE )

#elif defined(ESP32)

  // You can use Ethernet.init(pin) to configure the CS pin
  //Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit Featherwing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit Featherwing Ethernet

  #ifndef USE_THIS_SS_PIN
    #define USE_THIS_SS_PIN   22    // For ESP32
  #endif

  ET_LOGWARN1(F("ESP32 setCsPin:"), USE_THIS_SS_PIN);

  // For other boards, to change if necessary
  #if ( USE_ETHERNET || USE_ETHERNET_LARGE || USE_ETHERNET2 || USE_ETHERNET_ENC )
    // Must use library patch for Ethernet, EthernetLarge libraries
    // ESP32 => GPIO2,4,5,13,15,21,22 OK with Ethernet, Ethernet2, EthernetLarge
    // ESP32 => GPIO2,4,5,15,21,22 OK with Ethernet3
  
    //Ethernet.setCsPin (USE_THIS_SS_PIN);
    Ethernet.init (USE_THIS_SS_PIN);
  
  #elif USE_ETHERNET3
    // Use  MAX_SOCK_NUM = 4 for 4K, 2 for 8K, 1 for 16K RX/TX buffer
    #ifndef ETHERNET3_MAX_SOCK_NUM
      #define ETHERNET3_MAX_SOCK_NUM      4
    #endif
  
    Ethernet.setCsPin (USE_THIS_SS_PIN);
    Ethernet.init (ETHERNET3_MAX_SOCK_NUM);

  #elif USE_CUSTOM_ETHERNET
  
    // You have to add initialization for your Custom Ethernet here
    // This is just an example to setCSPin to USE_THIS_SS_PIN, and can be not correct and enough
    Ethernet.init(USE_THIS_SS_PIN); 
  
  #endif  //( USE_ETHERNET || USE_ETHERNET2 || USE_ETHERNET3 || USE_ETHERNET_LARGE )

#elif ETHERNET_USE_RPIPICO

  pinMode(USE_THIS_SS_PIN, OUTPUT);
  digitalWrite(USE_THIS_SS_PIN, HIGH);
  
  // ETHERNET_USE_RPIPICO, use default SS = 5 or 17
  #ifndef USE_THIS_SS_PIN
    #if defined(ARDUINO_ARCH_MBED)
      #define USE_THIS_SS_PIN   5     // For Arduino Mbed core
    #else  
      #define USE_THIS_SS_PIN   17    // For E.Philhower core
    #endif
  #endif

  ET_LOGWARN1(F("RPIPICO setCsPin:"), USE_THIS_SS_PIN);

  // For other boards, to change if necessary
  #if ( USE_ETHERNET || USE_ETHERNET_LARGE || USE_ETHERNET2 || USE_ETHERNET_ENC )
    // Must use library patch for Ethernet, EthernetLarge libraries
    // For RPI Pico using Arduino Mbed RP2040 core
    // SCK: GPIO2,  MOSI: GPIO3, MISO: GPIO4, SS/CS: GPIO5
    // For RPI Pico using E. Philhower RP2040 core
    // SCK: GPIO18,  MOSI: GPIO19, MISO: GPIO16, SS/CS: GPIO17
    // Default pin 5/17 to SS/CS
  
    //Ethernet.setCsPin (USE_THIS_SS_PIN);
    Ethernet.init (USE_THIS_SS_PIN);
  
  #elif USE_ETHERNET3
    // Use  MAX_SOCK_NUM = 4 for 4K, 2 for 8K, 1 for 16K RX/TX buffer
    #ifndef ETHERNET3_MAX_SOCK_NUM
      #define ETHERNET3_MAX_SOCK_NUM      4
    #endif
  
    Ethernet.setCsPin (USE_THIS_SS_PIN);
    Ethernet.init (ETHERNET3_MAX_SOCK_NUM);
    
  #endif    //( USE_ETHERNET || USE_ETHERNET2 || USE_ETHERNET3 || USE_ETHERNET_LARGE )
  
#else   //defined(ESP8266)
  // unknown board, do nothing, use default SS = 10
  #ifndef USE_THIS_SS_PIN
    #define USE_THIS_SS_PIN   10    // For other boards
  #endif

  ET_LOGWARN3(F("Board :"), BOARD_NAME, F(", setCsPin:"), USE_THIS_SS_PIN);

  // For other boards, to change if necessary
  #if ( USE_ETHERNET || USE_ETHERNET_LARGE || USE_ETHERNET2  || USE_ETHERNET_ENC )
    // Must use library patch for Ethernet, Ethernet2, EthernetLarge libraries
  
    Ethernet.init (USE_THIS_SS_PIN);
  
  #elif USE_ETHERNET3
    // Use  MAX_SOCK_NUM = 4 for 4K, 2 for 8K, 1 for 16K RX/TX buffer
    #ifndef ETHERNET3_MAX_SOCK_NUM
      #define ETHERNET3_MAX_SOCK_NUM      4
    #endif
  
    Ethernet.setCsPin (USE_THIS_SS_PIN);
    Ethernet.init (ETHERNET3_MAX_SOCK_NUM);

  #elif USE_CUSTOM_ETHERNET
  
    // You have to add initialization for your Custom Ethernet here
    // This is just an example to setCSPin to USE_THIS_SS_PIN, and can be not correct and enough
    Ethernet.init(USE_THIS_SS_PIN);
    
  #endif  //( USE_ETHERNET || USE_ETHERNET2 || USE_ETHERNET3 || USE_ETHERNET_LARGE )

#endif    //defined(ESP8266)


#endif  //USE_ETHERNET_WRAPPER


  // start the ethernet connection and the server:
  // Use DHCP dynamic IP and random mac
  uint16_t index = millis() % NUMBER_OF_MAC;
  // Use Static IP
  //Ethernet.begin(mac[index], ip);
  Ethernet.begin(mac[index]);

  // Just info to know how to connect correctly
  Serial.println(F("========================="));
  Serial.println(F("Currently Used SPI pinout:"));
  Serial.print(F("MOSI:"));
  Serial.println(MOSI);
  Serial.print(F("MISO:"));
  Serial.println(MISO);
  Serial.print(F("SCK:"));
  Serial.println(SCK);
  Serial.print(F("SS:"));
  Serial.println(SS);
#if USE_ETHERNET3
  Serial.print(F("SPI_CS:"));
  Serial.println(SPI_CS);
#endif
  Serial.println(F("========================="));

  Serial.print(F("Using mac index = "));
  Serial.println(index);

  // you're connected now, so print out the data
  Serial.print(F("You're connected to the network, IP = "));
  Serial.println(Ethernet.localIP());

  myTZ = new Timezone(myDST, mySTD);

  timeClient.begin();

  rtc.attach(Wire);
}

void loop()
{
  timeClient.update();
  
  check_status();
}

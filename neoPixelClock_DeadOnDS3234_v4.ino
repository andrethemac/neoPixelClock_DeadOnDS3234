// Date and time functions using a DS1307 RTC connected via I2C and Wire lib

#include <TimerOne.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <SparkFunDS3234RTC.h>

// Avoid spurious warnings
#undef PROGMEM
#define PROGMEM __attribute__(( section(".progmem.data") ))
#undef PSTR
#define PSTR(s) (__extension__({static prog_char __c[] PROGMEM = (s); &__c[0];}))

// sparkfunDS3234RTC
//////////////////////////////////
// Configurable Pin Definitions //
//////////////////////////////////
#define DS13074_CS_PIN 10 // DeadOn RTC Chip-select pin
#define INTERRUPT_PIN 2 // DeadOn RTC SQW/interrupt pin (optional)

// adafruits neopixel 
//1 outerring 24 pixels (minutes and seconds) pixel 12-35
//and 1 inner ring 12 pixels (hours) inner ring = pixel 0-11
#include <Adafruit_NeoPixel.h>

#define PIN 6
#define Pixels 36

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(Pixels, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

//strip.Color(r,g,b)
static int8_t lastSecond = -1;
static int8_t lastMinute = -1;
static int8_t lastHour = -1;
static int8_t thisHour = -1;
#define l_high  50
#define l_off    0
const uint32_t c_hour   = strip.Color(l_high,l_high,l_off);  // white
const uint32_t c_minute = strip.Color(l_high,l_high,l_off);  // yellow
const uint32_t c_off    = strip.Color(l_off,l_off,l_off);   // off
uint32_t c_ledm   = strip.Color(l_off,l_off,l_off);   // off
uint32_t c_led1   = strip.Color(l_off,l_off,l_off);   // off
uint32_t c_led2   = strip.Color(l_off,l_off,l_off);   // off
uint32_t c_led3   = strip.Color(l_off,l_off,l_off);   // off

uint32_t c_hours[] = {
  strip.Color( 0, 0,60), // 0
  strip.Color( 0, 0,60), // 1
  strip.Color( 0, 0,60), // 2
  strip.Color( 0, 0,60), // 3
  strip.Color(10, 0,20), // 4
  strip.Color(20, 0,10), // 5
  strip.Color(60,10, 0), // 6
  strip.Color(80,10, 0), // 7
  strip.Color(80,20, 0), // 8
  strip.Color(80,30, 0), // 9
  strip.Color(80,40, 0), // 10
  strip.Color(80,50,10), // 11
  strip.Color(80,60,20), // 12
  strip.Color(80,60,20), // 13
  strip.Color(80,50,10), // 14
  strip.Color(80,40, 0), // 15
  strip.Color(80,30, 0), // 16
  strip.Color(60,20, 0), // 17
  strip.Color(40,10,10), // 18
  strip.Color(20,10,20), // 19
  strip.Color(10, 0,30), // 20
  strip.Color( 0, 0,60), // 21
  strip.Color( 0, 0,60), // 22
  strip.Color( 0, 0,60)  // 23
};

static int counter = 0;
static bool nexthalf = false;
uint8_t secondled1 = 0;
uint8_t secondled2 = 0; 
uint8_t secondled3 = 0; 
uint8_t minuteled1 = 0;
  
void setup() {
  Serial.begin(9600);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  clocksplash();

//Timer1.initialize(500000UL); // 2 hz
  Timer1.initialize(100000UL); // 10 hz
  Timer1.attachInterrupt(timerIsr);

#ifdef INTERRUPT_PIN // If using the SQW pin as an interrupt
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
#endif

  // Call rtc.begin([cs]) to initialize the library
  // The chip-select pin should be sent as the only parameter
  rtc.begin(DS13074_CS_PIN);
  //rtc.set12Hour(); // Use rtc.set12Hour to set to 12-hour mode

  // Now set the time...
  // You can use the autoTime() function to set the RTC's clock and
  // date to the compiliers predefined time. (It'll be a few seconds
  // behind, but close!)
  rtc.update();
  if(rtc.year() == 0 ) {
    //rtc.autoTime();
    rtc.setTime(0,29,20,4,16,11,16);
    rtc.enable();
    rtc.update();
    Serial.println("timeset");
  }
  Serial.print(rtc.year());
  Serial.print('/');
  Serial.print(rtc.month());
  Serial.print('/');
  Serial.print(rtc.date());
  Serial.print(" - ");
  Serial.print(rtc.hour());
  Serial.print(":"),
  Serial.print(rtc.minute());
  Serial.print(":"),
  Serial.print(rtc.second());
  Serial.println();  

  // Or you can use the rtc.setTime(s, m, h, day, date, month, year)
  // function to explicitly set the time:
  // e.g. 7:32:16 | Monday October 31, 2016:
  //rtc.setTime(16, 00, 53, 18, 27, 11, 16);  // Uncomment to manually set time

  // Update time/date values, so we can set alarms
  rtc.update();
  // Configure Alarm(s):
  // (Optional: enable SQW pin as an intterupt)
  rtc.enableAlarmInterrupt();
  // Set alarm1 to alert when seconds hits 30
  rtc.setAlarm1(rtc.second() + 5);
  // Set alarm2 to alert when minute increments by 1
  // rtc.setAlarm2(rtc.minute() + 1);
  
  Serial.println("-------------");
  Serial.println("| end setup |");
  Serial.println("-------------");
}

void clocksplash(){
  long int c = 1;
  for(int x = 0; x < 24; x++) {
    strip.setPixelColor(x,c);
    strip.setPixelColor((x/2)+24,c);
    strip.show();
    c = c << 1;
    delay(30);
  }
  for(int x = 0; x < 24; x++) {
    strip.setPixelColor(x,0);
    strip.setPixelColor((x/2)+24,0);
    strip.show();
    delay(30);
  }

  for(int x = 0; x < 24; x++) {
    strip.setPixelColor(x,c_hours[x]);
  }
  for(int x = 0; x < 12; x++) {
    strip.setPixelColor(x+24,c_hours[x*2]);
  }
  strip.show();
  delay(30*24);

  for(int x = 0; x < Pixels; x++) {
    strip.setPixelColor(x,0);
  }
  strip.show();
  strip.setPixelColor(0,strip.Color(50,50,50));
  strip.setPixelColor(6,strip.Color(50,00,00));
  strip.setPixelColor(12,strip.Color(00,50,00));
  strip.setPixelColor(18,strip.Color(00,00,50));
  
  strip.setPixelColor(24,strip.Color(50,50,50));
  strip.setPixelColor(27,strip.Color(50,00,00));
  strip.setPixelColor(30,strip.Color(00,50,00));
  strip.setPixelColor(33,strip.Color(00,00,50));
  strip.show();
  delay(1000);
  for(int x = 0; x < Pixels; x++) {
    strip.setPixelColor(x,0);
  }
  strip.show();
}

void timerIsr() {
  static bool isThereANewSecond = false;

  if ( isThereANewSecond == true) {
    if ( (rtc.second() % 5 ) == 0 ) {
      counter = 0;
      isThereANewSecond = false;
    }
  }
  if ( (rtc.second() % 5) > 0 ){
    isThereANewSecond = true;
  }
  uint8_t pwrup = (4 * (counter)) % 100;
  uint8_t pwrdown = 100-pwrup;

  ++counter;
  
  if (counter > 25) {
    secondled1 = ((rtc.second() / 5)*2)+1;
  } else {
    secondled1 = ((rtc.second() / 5)*2);
  }
  secondled2 = secondled1 == 0 ? 23 : secondled1 - 1; 
  secondled3 = secondled2 == 0 ? 23 : secondled2 - 1; 
  c_led1 = strip.Color(pwrup,0,pwrup);
  c_led2 = strip.Color(pwrdown,0,pwrdown);
  c_led3 = strip.Color(0,0,0);

  // show them all
  strip.show();

}

void loop() {
//}
//void nope() {

  // Call rtc.update() to update all rtc.seconds(), rtc.minutes(),
  // etc. return functions.
  rtc.update();

  if ( rtc.alarm1() ) {
    printDateTime();
  }

  //set hour
  thisHour = (rtc.hour() % 12) + 24;
  if(thisHour != lastHour ) {
    strip.setPixelColor(lastHour,c_off);
    strip.setPixelColor(thisHour,c_hours[rtc.hour()]);
    lastHour = thisHour;
  }

  uint8_t minuteled1 = (rtc.minute() * 60 + rtc.second()) / 150;
  uint8_t minuteled2 = minuteled1 == 0 ? 23 : minuteled1 - 1; 
  //c_ledm = c_minute;
  c_ledm = c_hours[rtc.hour()];

  if (minuteled1 == secondled1) {
    c_ledm = c_ledm | c_led1;
    c_led1 = c_ledm;
  }
  if (minuteled1 == secondled2) {
    c_ledm = c_ledm | c_led2;
    c_led2 = c_ledm;
  }
  if (minuteled1 == secondled3) {
    c_ledm = c_ledm | c_led3;
    c_led3 = c_ledm;
  }

  strip.setPixelColor(minuteled1,c_ledm);
  strip.setPixelColor(minuteled2,c_off);

  strip.setPixelColor(secondled1,c_led1);
  strip.setPixelColor(secondled2,c_led2);
  strip.setPixelColor(secondled3,c_led3);
  
  delay(50);
}

void printDateTime() {
    Serial.print(rtc.year());
    Serial.print('/');
    Serial.print(rtc.month());
    Serial.print('/');
    Serial.print(rtc.date());
    Serial.print(" - ");
    Serial.print(rtc.hour());
    Serial.print(":"),
    Serial.print(rtc.minute());
    Serial.print(":"),
    Serial.print(rtc.second());
    Serial.println();  
}


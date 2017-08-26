/*

    Project : ·NINE· [dasaki && n3m3da]
    
    Concept : BY-SA 4.0
    Code    : MIT
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    (BY-SA) 2017
*/

// External Libraries
#include <LowPower.h>           // https://github.com/rocketscream/Low-Power
#include "digitalWriteFast.h"   // modified version from: https://github.com/NicksonYap/digitalWriteFast

// Custom Code
#include "morse.h"

bool DEBUG = true;
bool HW_DEBUG = true;
bool SYS_DEBUG = true;

#define TIME_MULTIPLIER 1
#define MORSE_UNIT_TIME SLEEP_250MS
#define NUM_LEDS        9
#define LDR_PIN         A0
#define NUM_CYCLES      9
#define MIN_VOLTAGE     3200
#define MAX_VOLTAGE     4500
#define LDR_MIDNIGHT    60
#define MAX_BIN_COUNT   512


unsigned long arTime         = millis();
unsigned long reset          = millis();
unsigned int WAIT            = 150*TIME_MULTIPLIER;
unsigned int waitLynch       = 33*TIME_MULTIPLIER;
unsigned int actualPin       = 2;

int LDR            = 0;
long voltage       = 0;
bool weAreTheNight = false; 
 
unsigned int binaryCounter  = 0; // 9 bits, 0 - 512
unsigned int morseCounter   = 1;  // 1 - 9
unsigned int lynchCounter   = 1;  // 1 - 18
unsigned int batteryCounter = 1;  // 1 - 9
unsigned int actualCycle    = 1;
unsigned int actualStep     = 1;  // 1 - 5
bool endCycle               = false;
bool lynchON                = false;
unsigned int lynchFlicks[]  = { 33,66,99,101,106,109,123,132,161 };

String message              = encode( "PWNED " );

unsigned int ledPins[]      = { 2, 3, 4, 5, 6, 7, 8, 9, 10 };
unsigned int ledPinsRAND[]  = { 2, 3, 4, 5, 6, 7, 8, 9, 10 };


/////////////////////////////////////////////////////////
///////////////////////////////////////////////////////// SETUP
/////////////////////////////////////////////////////////

void setup() {
  randomSeed(analogRead(1));
  
  for(unsigned int i=0;i<NUM_LEDS;i++){
    pinModeFast(ledPins[i], OUTPUT);
    digitalWriteFast(ledPins[i], LOW);
  }

  shuffleLEDS();

  if(DEBUG){
    Serial.begin(9600);
  }
}

/////////////////////////////////////////////////////////
///////////////////////////////////////////////////////// LOOP
/////////////////////////////////////////////////////////

void loop() {
  // Avoid arduino calls between loops
  while (true) {

    // Read voltage, check charge state
    voltage = readVcc();
    // read LDR sensor, check darkness
    readLDR();

    if(HW_DEBUG){
      // Hardware Check
      if(voltage < MIN_VOLTAGE){
        if(DEBUG){
          Serial.println("Low battery");
        }
      }else{
        arTime = millis();
        if(SYS_DEBUG){
          // system check, discard LDR readings
          runCycle();
        }else{
          checkLEDS();
        }
      }
    }else{
      if(voltage >= MIN_VOLTAGE && weAreTheNight){
        // We're ON
        runCycle();
      }else{
        // Charging or before dusk
        if(DEBUG){
          Serial.println("Low battery/It' not dark yet!");
        }
        LEDSOFF();
        // Enter power down state for 8 s with ADC and BOD module disabled
        doPowerDown(SLEEP_8S, TIME_MULTIPLIER);
      }
    }
    
  } // End while
}

/////////////////////////////////////////////////////////
///////////////////////////////////////////////////////// CUSTOM FUNCTIONS
/////////////////////////////////////////////////////////

/*
 * ONE of ·NINE· runCycle
 * 
 */
void runCycle(){
  arTime = millis();

  if(endCycle){
    endCycle = false;
    randomSeed(analogRead(1));
    if(actualCycle < NUM_CYCLES){
      actualCycle++;
    }else{
      actualCycle = 1;
      actualStep++;
      if (actualStep > 5) actualStep = 1;
    }
  }

  switch (actualStep) {
    case 1:
      lynchLED();
      break;
    case 2:
      binaryLED();
      break;
    case 3:
      binaryLEDRand();
      break;
    case 4:
      sendMorseMessage();
      break;
    case 5:
      batteryMeter();
      break;
    default:
      actualStep = 1;
      LEDSOFF();
      doPowerDown(SLEEP_8S, TIME_MULTIPLIER);
    break;
  }
  
}

/*
 * Leds check test, sequencially turning on
 * 
 */
void checkLEDS(){
  for(unsigned int i=0;i<NUM_LEDS;i++){
    digitalWriteFast(ledPins[i], LOW);
    if(ledPins[i] == actualPin){
      digitalWriteFast(ledPins[i], HIGH);
    }
  }

  if(arTime-reset > WAIT){
    reset = millis();
    if(actualPin < 10){
      actualPin++;
    }else{
      actualPin = 2;
    }
  }
}

/*
 * Visualize battery power with leds as meter bar
 * 
 */
void batteryMeter(){
  // min 3200, max 4500
  int bl = map((int)voltage,MIN_VOLTAGE,MAX_VOLTAGE,0,NUM_LEDS);
  if(DEBUG){
    Serial.println(bl);
  }
  for(unsigned int i=0;i<NUM_LEDS;i++){
    if(i <= bl){
      digitalWriteFast(ledPins[i], HIGH);
    }else{
      digitalWriteFast(ledPins[i], LOW);
    }
  }

  if(arTime-reset > WAIT){
    reset = millis();
    if(batteryCounter < NUM_CYCLES){
      batteryCounter++;
    }else{
      endCycle = true;
      batteryCounter = 1;
      LEDSOFF();
      doPowerDown(SLEEP_4S, TIME_MULTIPLIER);
      
    }
  }
}


/*
 * Power down auxiliary function
 * 
 */
void doPowerDown(uint8_t sleepTime, uint8_t sleepCount) {
        do {
          LowPower.powerDown(sleepTime, ADC_OFF, BOD_OFF);
          sleepCount--;
        } while (sleepCount > 0);
}

/*
 * Simulate 9 bits binary counter (correct positions)
 * 
 */
void binaryLED(){

  for(unsigned int i=0;i<NUM_LEDS;i++){
    if(bitRead(binaryCounter,i) == 0){
      digitalWriteFast(ledPins[i], LOW);
    }else if(bitRead(binaryCounter,i) == 1){
      digitalWriteFast(ledPins[i], HIGH);
    }
  }

  if(arTime-reset > WAIT){
    reset = millis();
    if(binaryCounter < MAX_BIN_COUNT){
      binaryCounter++;
    }else{
      endCycle = true;
      binaryCounter = 0;
      LEDSOFF();
      doPowerDown(SLEEP_4S, TIME_MULTIPLIER);
    }
  }
}

/*
 * Simulate 9 bits binary counter (randomized positions)
 * 
 */
void binaryLEDRand(){

  for(unsigned int i=0;i<NUM_LEDS;i++){
    if(bitRead(binaryCounter,i) == 0){
      digitalWriteFast(ledPinsRAND[i], LOW);
    }else if(bitRead(binaryCounter,i) == 1){
      digitalWriteFast(ledPinsRAND[i], HIGH);
    }
  }

  if(arTime-reset > WAIT){
    reset = millis();
    if(binaryCounter < 512){
      binaryCounter++;
    }else{
      endCycle = true;
      binaryCounter = 0;
      LEDSOFF();
      shuffleLEDS();
      doPowerDown(SLEEP_4S, TIME_MULTIPLIER);
    }
  }
}

/*
 * A tribute to David Lynch
 * 
 */
void lynchLED(){
   for(unsigned int i=0;i<NUM_LEDS;i++){
    if(lynchON){
      digitalWriteFast(ledPins[i], HIGH);
    }else{
      digitalWriteFast(ledPins[i], LOW);
    }
  }

  if(arTime-reset > waitLynch){
    reset = millis();
    lynchON = !lynchON;
    if(lynchCounter < NUM_CYCLES*2){
      lynchCounter++;
    }else{
      endCycle = true;
      lynchCounter = 1;
      randomSeed(analogRead(1));
      int rr = random(0,9);
      waitLynch = lynchFlicks[rr]*TIME_MULTIPLIER;
      LEDSOFF();
      doPowerDown(SLEEP_4S, TIME_MULTIPLIER);
    }
  }
}

/*
 * As the function name says...
 * 
 */
void sendMorseMessage(){
  for(unsigned int i=0; i<=message.length(); i++){
    switch( message[i] ){
      case '.': // dit
        LEDSON();
        doPowerDown(MORSE_UNIT_TIME, TIME_MULTIPLIER);
        LEDSOFF();
        doPowerDown(MORSE_UNIT_TIME, TIME_MULTIPLIER);
        break;

      case '-': // dah
        LEDSON();
        doPowerDown(MORSE_UNIT_TIME, 3*TIME_MULTIPLIER);   
        LEDSOFF();
        doPowerDown(MORSE_UNIT_TIME, TIME_MULTIPLIER);  
        break;

      case ' ': //gap
        delay( MORSE_UNIT_TIME );
    }
  }

  if(morseCounter < NUM_CYCLES){
    morseCounter++;
  }else{
    endCycle = true;
    morseCounter = 1;
    LEDSOFF();
    doPowerDown(SLEEP_4S, TIME_MULTIPLIER);
  }
  
}

/*
 * Read LDR value (light/darkness detection)
 * 
 */
void readLDR(){
  LDR = analogRead(LDR_PIN);

  if(LDR < LDR_MIDNIGHT){
    weAreTheNight = true;
  }else{
    weAreTheNight = false;
  }
  
  if(DEBUG){
    Serial.println(LDR);
    delay(1);
  }
}

/*
 * Voltage reading
 * 
 */
long readVcc() { 
  long result; // Read 1.1V reference against AVcc 
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1); 
  delay(2); // Wait for Vref to settle 
  ADCSRA |= _BV(ADSC); // Convert 
  while (bit_is_set(ADCSRA,ADSC)); 
  result = ADCL; 
  result |= ADCH<<8; result = 1126400L / result; // Back-calculate AVcc in mV 
  return result; 
}

/*
 * Randomize LED pins
 * 
 */
void shuffleLEDS(){
  for (int a=0; a<NUM_LEDS; a++){
    int r = random(a,NUM_LEDS-1);
    int temp = ledPinsRAND[a];
    ledPinsRAND[a] = ledPinsRAND[r];
    ledPinsRAND[r] = temp;
  }
}

/*
 * Leds Shutdown
 * 
 */
void LEDSOFF(){
   for(unsigned int i=0;i<NUM_LEDS;i++){
    digitalWriteFast(ledPins[i], LOW);
  }
}

/*
 * Leds Up
 * 
 */
void LEDSON(){
   for(unsigned int i=0;i<NUM_LEDS;i++){
    digitalWriteFast(ledPins[i], HIGH);
  }
}

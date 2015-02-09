/*
 Parking Helper
 
 A little sketch for my parking helper. It uses a MaxBotix MB1240 Sonar, reading from the analog pin on A4.
 It also uses a micro-potentiometer to adjust the "all the way in" range after installation. The LEDs are
 installed on D80D12 and are lit in a logarithmic manner, using 1/2.5 as the expo.
 
 I set up my LEDs with 2 green, 2 yellow and a red (actually 2 red in series on one pin). If you want to use
 more or less, you should be able to adjust the code pretty easily.
 
 NOTE: Since it was initially used on a multirotor, my MB1240 has an RC noise filter attached as described here:
 https://www.youtube.com/watch?v=Rba1ZdL0vyE
 
 Enjoy!
 
 Created by Peter Provost <peter@provost.org>
 
 History:
 - 2015-02-03 : Initial version
 
 This code is released under the Creative Commons Attribution 4.0 License (CC BY 4.0).
 http://creativecommons.org/licenses/by/4.0/
 
 Acknowledgments and Additional Information:
 - MB1240 Product Information: http://www.maxbotix.com/Ultrasonic_Sensors/MB1240.htm
 - MaxSonar example: https://billwaa.wordpress.com/2014/03/11/arduino-ultrasonic-range-finder-xl-maxsonar/
 - readVcc function from:  http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
 
 */

#define DEBUG 1 // Set to 0 to disable Serial debug logging
#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#define POT_PIN    2
#define SONAR_PIN  4
#define LED1_PIN   8
#define LED2_PIN   9
#define LED3_PIN  10
#define LED4_PIN  11
#define LED5_PIN  12

#define MAX_DIST  18 // feet
#define EXPO      2.5 // used to break up the space for the series of LEDs

long readVcc() {

  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif  

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  // Calculate Vcc (in mV); 

  // Original value: 1125300 = 1.1*1023*1000.
  result = 1125300L / result; 

  // Based on my measurements on my Micro. See referenced blog post.
  // result = 1099332L / result;

  return result; // Vcc in millivolts
}

void setup()
{
  Serial.begin(9600);

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(LED4_PIN, OUTPUT);
  pinMode(LED5_PIN, OUTPUT);
}

double getInches()
{
  double distance = 0;
  double voltage = analogRead(SONAR_PIN)*.0049;  //Analog In .49 mV per unit 
  double vccRaw = readVcc() / 1000.0;

  // If using an LC filter on the MB1240, use this one
  // Update the ohms value to match what you used in your filter
  // double vcc = vccRaw - ((3.4 / 1000 /* mAh draw */) * 10 /* ohms */);

  double vcc = vccRaw; // Or not...

  double scaling = vcc / 1024.0;

  double offset = 3.2; // cm (Empirical from local measurements with my MB1240. Adjust as needed.)
  distance = (voltage / scaling) + offset;

  double cmToInch = 0.393701;

  return distance * cmToInch;
}

int getNumLights(double inches)
{
  int potValue = analogRead(POT_PIN);

  int minDist = (potValue / 1024.0) * 60;  // adjust from 0 to 60 inches
  int maxDist = MAX_DIST * 12;                              // 18 feet max

  int range = maxDist - minDist;
  int step1 = range / 2.5; // first split
  int step2 = step1 / 2.5; // once more
  int step3 = step2 / 2.5; // and one final time

  if (inches <= minDist)
    return 5;

  if (inches <= step3+minDist)
    return 4;

  if (inches <= step2+minDist)
    return 3;

  if (inches <= step1+minDist)
    return 2;

  if (inches <= maxDist+minDist)  
    return 1;

  return 0;
}

void setLEDs(int numLights)
{
  DEBUG_PRINT("Setting ");
  DEBUG_PRINT(numLights);
  DEBUG_PRINTLN(" LEDs");

  int LEDPins[5] = { 
    LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN, LED5_PIN     };

  int value;
  for (int i=1; i<=5; i++)
  {
    value = LOW;
    if (i<=numLights)
      value = HIGH;

    digitalWrite(LEDPins[i-1], value);
  }  
}


void loop()
{
  double inches = getInches();
  DEBUG_PRINT(inches);
  DEBUG_PRINTLN(" in");

  int numLights = getNumLights(inches);
  setLEDs(numLights);

  delay(100);
}






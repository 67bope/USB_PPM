/* 
 *  Simplified Logitech Extreme 3D Pro Joystick Report Parser, ready to go.
 *   
 * Script Was Combined and Tested By bope (RCGroups.com)
 * I did not create the Library this was Found online links are below
 * I did not create the PPM function Was found online links are below
 * I merged both codes to create something usefull out of it 
 * 
 * Useful Note:
 * 
 * PPM Output is on Pin 3
 * 
 *   Source:
 *   https://github.com/DroneMesh/USB_PPM
 *  
*/

#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>
#include "le3dp_rptparser2.0.h"
// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

USB                                             Usb;
USBHub                                          Hub(&Usb);
HIDUniversal                                    Hid(&Usb);
JoystickEvents                                  JoyEvents;
JoystickReportParser                            Joy(&JoyEvents);


bool printAngle;
uint8_t state = 0;

  int Xval;   // 0 - 1023
  int Yval;   // 0 - 1023
  int Hat;    // 0 - 15;
  int Twist;  // 0 - 255
  int Slider; // 0 - 255
  int Button; // 0 - 12 (0 if no button, 1-12 for other buttons)

/*
 * PPM generator originally written by David Hasko
 * on https://code.google.com/p/generate-ppm-signal/ 
 */

//////////////////////CONFIGURATION///////////////////////////////
#define CHANNEL_NUMBER 8  //set the number of chanels
#define CHANNEL_DEFAULT_VALUE 1500  //set the default servo value
#define FRAME_LENGTH 22500  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PULSE_LENGTH 300  //set the pulse length
#define onState 1  //set polarity of the pulses: 1 is positive, 0 is negative
#define sigPin 3  //set PPM signal output pin on the arduino
//////////////////////////////////////////////////////////////////

#define SWITCH_PIN 16
#define CHANNEL_TO_MODIFY 4
#define SWITCH_STEP 100

byte previousSwitchValue;

/*
 this array holds the servo values for the ppm signal
 change theese values in your code (usually servo values move between 1000 and 2000)
*/
int ppm[CHANNEL_NUMBER];

int currentChannelStep;


void setup()
{
  Serial.begin( 115200 );
  
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif

  Serial.println("Start");
  if (Usb.Init() == -1)
      Serial.println("OSC did not start.");
  delay( 200 );

  if (!Hid.SetReportParser(0, &Joy))
      ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1  );

//////////////////////PPM-Values///////////////////////////////
  previousSwitchValue = HIGH;
  
  //initiallize default ppm values
  for(int i=0; i<CHANNEL_NUMBER; i++){
    if (i == 2 || i == CHANNEL_TO_MODIFY) {
      ppm[i] = 1000;
    } else {
      ppm[i]= CHANNEL_DEFAULT_VALUE;
    }
  }

  pinMode(sigPin, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  digitalWrite(sigPin, !onState);  //set the PPM signal pin to the default state (off)
  
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  
  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();

  currentChannelStep = SWITCH_STEP;
}


/*    Channel Info Currently Setup as AETR you can change this by changing the PPM Value in the main loop
* 
Ch1 A (Roll ) ==  ppm[0]
Ch2 E (Pitch ) ==  ppm[1]
Ch3 T (Throttle ) ==  ppm[2]
Ch4 R (Yaw ) ==  ppm[3]
Ch5 AUX1 (Arm) == ppm[4]
Ch6 AUX2 (Modes) == ppm[5] is on Buttons
Ch7 AUX3 (Hat) == ppm[6]
Ch8 AUX4 (Beeper) == ppm[7]
*/
 

void loop()
{
  Usb.Task();                                                    //Use to read joystick input to controller
  //JoyEvents.PrintValues();                                       //Returns joystick values to user
  JoyEvents.GetValues(Xval, Yval, Hat, Twist, Slider, Button);   //Copies joystick values to user
/////////////////////////////////////////////////////////////////////////////////////////////////

if ( Usb.getUsbTaskState() != USB_STATE_RUNNING ) {
  ppm[4] = 1000;   
  }
  
else{
      // Roll Axis is on Joystick X Axis
      ppm[0] = map(Xval, 0 , 1023, 1000, 2000);
      // Pitch Axis on Joystick  Y Axis (Inverted to work correctly)
      ppm[1] = map(Yval, 0 , 1023, 2000, 1000);
      // Throttle is on Slider 
      ppm[2] = map(Slider, 0 , 255, 800, 2000);
      // Yaw Axis is on Joystick Z Axis
      ppm[3] = map(Twist, 0 , 255, 1000, 2000);
      // Hat (example: 8-way momentary Switch)
      ppm[6] = (Hat*125)+1000;

if (Button == 1)  {
      // AUX2 Lower Bound // Angle Mode set in Betaflight
      ppm[5] = 1000;  }
    
else if (Button == 2)  {  
      // AUX2 Upper Bound// Acro Mode
      ppm[5] = 2000;  }

else if (Button == 7)  {
      // Button7 is to Disarm if Arm is on AUX1 
      ppm[4] = 1000;  }

else if (Button == 8)  {
      // Button8 is to Arm if Arm is on AUX1
      ppm[4] = 2000;  }

else if (Button == 12)  {
      // Button12 is to activate Beeper is on AUX4
      ppm[7] = 2000;  }      

else {
      //Momentary Switch example (Button 12)
      ppm[7] = 1000;
      } 
  }//USB_STATE      
  
  Serial.print("X = ");
  Serial.print(ppm[0]);
  Serial.print("\t Y = ");
  Serial.print(ppm[1]);
  Serial.print("\t Hat = ");
  Serial.print(ppm[6]);
  Serial.print("\t Twist = ");
  Serial.print(ppm[3]);
  Serial.print("\t Slider = ");
  Serial.print(ppm[2]);
  Serial.print("\t Button = ");
  Serial.print(ppm[5]);
  Serial.print("\t Arm = ");
  Serial.print(ppm[4]);
  Serial.print("\t Beeper = ");
  Serial.println(ppm[7]);
  delay(250);
           
}//loop
///////////////////////////////////////////////////////////////////////////////////////////////// 


ISR(TIMER1_COMPA_vect){  //leave this alone
  static boolean state = true;
  
  TCNT1 = 0;
  
  if (state) {  //start pulse
    digitalWrite(sigPin, onState);
    OCR1A = PULSE_LENGTH * 2;
    state = false;
  } else{  //end pulse and calculate when to start the next pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;
  
    digitalWrite(sigPin, !onState);
    state = true;

    if(cur_chan_numb >= CHANNEL_NUMBER){
      cur_chan_numb = 0;
      calc_rest = calc_rest + PULSE_LENGTH;// 
      OCR1A = (FRAME_LENGTH - calc_rest) * 2;
      calc_rest = 0;
    }
    else{
      OCR1A = (ppm[cur_chan_numb] - PULSE_LENGTH) * 2;
      calc_rest = calc_rest + ppm[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}

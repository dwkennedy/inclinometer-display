/*
  
  inclinometer display sketch

  take inclinometer input from Serial2
  copy to/from Serial1 (for passthru when using PC software)
  update SainSmart 5" touch LCD
  hardware requirements:
     arduino DUE
     MAX3232 on tx1/rx1 and tx2/rx2 for RS-232 level shifting.
     DB9 connecters (male on tx2/rx2, female for tx1/rx1 passthru)
     measurement specialties NS-5/DMG2-S inclinometer
     sainsmart LCD driver and LCD panel SKU: 20-011-D20  
     https://www.sainsmart.com/products/sainsmart-due-5-lcd-touch-panel-sd-card-slot-tft-lcd-shield-kit-for-arduino
     12VDC, 500mA power supply with barrel connector -or- 5VDC, 1A with micro USB connector
  
  created 1 March 2018
  by Doug Kennedy
*/

#include <UTFT.h>

// Declare which fonts we will be using
//extern uint8_t SmallFont[];
//extern uint8_t SevenSegNumFont[];
extern uint8_t BigFont[];
extern uint8_t GroteskBold32x64[];

// Set the pins to the correct ones for your development shield
// ------------------------------------------------------------
// Standard Arduino Mega/Due shield            : <display model>,38,39,40,41
// CTE TFT LCD/SD Shield for Arduino Due       : <display model>,25,26,27,28
// Teensy 3.x TFT Test Board                   : <display model>,23,22, 3, 4
// ElecHouse TFT LCD/SD Shield for Arduino Due : <display model>,22,23,31,33
//
// Remember to change the model parameter to suit your display module!

UTFT myGLCD(CTE50,25,26,27,28);
// dimensions of screen
#define HEIGHT 800
#define WIDTH 480
// coordinates of center of level bubble
#define LEVEL_X_CENTER (WIDTH/2)
#define LEVEL_Y_CENTER ((HEIGHT/2)+140)
// rings
#define LEVEL_RADIUS1 50
#define LEVEL_RADIUS2 125
#define LEVEL_RADIUS3 200
// max range of inclinometer, in degrees +/-
#define LEVEL_RANGE 8
// pixels per degree (inclinometer reads in degrees +/-)
#define LEVEL_SCALE (WIDTH/(2*LEVEL_RANGE))
// "spread" of function, makes center more sensitive.  Bigger is less sensitive.
// LEVEL_SIDMOID=0.5:  0.5 degree per range ring
// LEVEL_SIGMOID=1.0:  1.0 degree per range ring
// LEVEL_SIGMOID=2.0:  2.0 degree per range ring (approx)
#define LEVEL_SIGMOID 2

// how many millis before reporting inclinometer data missing
#define INCLINOMETER_TIMEOUT 3000
// how fast to blink the TIMEOUT message (in millis)
#define BLINK_RATE 500

// handy inline function
#define SIGN(a) (((a) > 0) ? '+' : '-')
 
String inputString = "";         // a String to hold incoming data stream
String outputString1 = "";        // a String to build output
String outputString2 = "";        // a String to build output
boolean stringComplete = false;  // whether the string is complete
float inclinometerX, inclinometerY;  // hold last values of inclinometer output
float oldInclinometerX, oldInclinometerY;  // extra copy in case of output error (zero)
int bubbleX, bubbleY;              // center of plotted bubble, in pixels
unsigned long timeout;           // keep track of last inclinometer message
boolean timeoutFlag = false;     // display TIMEOUT message (true) or not (false)

void setup() {
  Serial1.begin(9600);
  Serial2.begin(9600);
  inputString.reserve(200);
  outputString1.reserve(20);
  outputString2.reserve(20);
  // force one update of the display; useful if inclinometer not sending messages (or unplugged)
  stringComplete = true;  
  // Setup the LCD
  myGLCD.InitLCD(PORTRAIT);
  //  myGLCD.setFont(SmallFont);
  //  myGLCD.setFont(SevenSegNumFont);
  myGLCD.setFont(GroteskBold32x64);
  myGLCD.clrScr();

  // print initialization message here
  myGLCD.print("NSSL/FOFS",CENTER,100);
  myGLCD.print("Inclinometer",CENTER,200);
  myGLCD.print("Display, v2.0",CENTER,300);
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  Serial1.println("NSSL Inclinometer Display v2.0");
  Serial1.println("");
  Serial1.print("setting inclinometer to continuous mode");

  // put inclinometer in continuous mode
  delay(1000);
  Serial2.write("F");
  Serial1.write(".");
  delay(750);
  Serial2.write("F");
  Serial1.write(".");
  delay(500);
  Serial2.write("F");
  Serial1.write(".");
  delay(500);
  Serial1.println();
    
  // empty buffer
  while (Serial2.available() ) {
    Serial2.read();
    delay(1);
  }

  // set initial missing values so we know it's not working yet
  
  inclinometerX = 9.999;
  inclinometerY = 9.999;
  oldInclinometerX = 9.999;
  oldInclinometerY = 9.999;
  
  myGLCD.clrScr();

  // initialize inclinometer read timeout
  timeout = millis();  
}

void loop() {
  static int page=0;
     
  if(stringComplete) {
    // parse string here

    inputString.trim();
    
    // debug code
    //Serial1.print("parse: ");
    //Serial1.println(inputString);

    //  save a copy of previous inclinometer readings, in case we get a parse error
    //     so we can roll back to the previous value
    oldInclinometerX = inclinometerX;
    oldInclinometerY = inclinometerY;
    
    if (inputString.startsWith("X=")) {
        // parse number, load into inclinometerX
        inclinometerX = inputString.substring(2).toFloat();
        if (inclinometerX == 0) {
          inclinometerX = oldInclinometerX;
        }
    } else if (inputString.startsWith("Y=")) {
        // parse number, load into inclinometerY
        inclinometerY = inputString.substring(2).toFloat();
        if (inclinometerY == 0) {
          inclinometerY = oldInclinometerY;
        }
    }


    // debug code
    // Serial1.print(inputString);
    //Serial1.print("X = ");
    //Serial1.print(inclinometerX, 3);
    //Serial1.print(": Y = ");
    //Serial1.println(inclinometerY, 3);

    // skip to next sentence in string (if there is one)
    inputString = inputString.substring(inputString.indexOf('\n'));
    inputString.trim();
    
    stringComplete = (inputString.length() > 0);  // still characters left => stringComplete is true

    //debug code
    //Serial1.print("remain: ");
    //Serial1.println(inputString);

    if (!stringComplete) {  // only update if we read a new message, and are now out of sentences in buffer
      timeoutFlag = false;  // erase timeout messgae
      updateDisplay();
      timeout = millis();  // update timeout
    }
  } else {
    // message not received, check timeout
    if ((millis() - timeout) > INCLINOMETER_TIMEOUT) {
      // timeout; set inclinometer values to missing 
      inclinometerX = 9.999;
      inclinometerY = 9.999;
      timeoutFlag = !timeoutFlag;  // blink timeout message
      updateDisplay();  //    and force display update
      // don't erase/redraw missing bubble
      bubbleX = -1;
      bubbleY = -1;
      timeout = millis() - INCLINOMETER_TIMEOUT + BLINK_RATE;     // update timer (so we don't constantly redraw while in timeout)
    }
  }
}

// copy characters from passthru port to inclinometer
void serialEvent1() {
  while (Serial1.available()) {
    Serial2.write(Serial1.read());
  }
}

// read from inclinometer
void serialEvent2() {
  static int state;

  digitalWrite(LED_BUILTIN, state);
  state = !state;
  
  while (Serial2.available()) {
    // get the new byte:
    char inChar = (char)Serial2.read();
    // echo to Serial1
    Serial1.write(inChar);   
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

// redraw the display
void updateDisplay() {
  
  // double buffering -- not implemented on CTE50 display
  //if (page == 0) {
  //  page = 1;
  //} else {
  //  page = 0;
  //}
    
  //myGLCD.setWritePage(page);
  //myGLCD.clrScr();
   
  // update numbers; multiply/divide to get rid of freakiness
  //  and by freakiness I mean ambiguity in sign when very close to zero 
  outputString1 = String("X: ") + String(SIGN(inclinometerX*1000)) + String(abs(inclinometerX*1000)/1000,3);
  outputString2 = String("Y: ") + String(SIGN(inclinometerY*1000)) + String(abs(inclinometerY*1000)/1000,3);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print(outputString1, CENTER, 20);
  myGLCD.print(outputString2, CENTER, 100);

  // update level bubble

  // erase bubble
  myGLCD.setColor(0,0,0);
  myGLCD.fillCircle(bubbleX, bubbleY, 20);

  if ((inclinometerX < 9.998) && (inclinometerY < 9.998)) {
    // draw new bubble; bubbleX and bubbleY are in units of pixels and include display center offset
    bubbleX = LEVEL_X_CENTER + LEVEL_SCALE*((2*LEVEL_RANGE/(1+exp(-inclinometerX/LEVEL_SIGMOID))) - LEVEL_RANGE);
    bubbleY = LEVEL_Y_CENTER - LEVEL_SCALE*((2*LEVEL_RANGE/(1+exp(-inclinometerY/LEVEL_SIGMOID))) - LEVEL_RANGE);
    //bubbleX = LEVEL_X_CENTER + LEVEL_SCALE*((2*LEVEL_RANGE/(1+exp(-inclinometerY/LEVEL_SIGMOID))) - LEVEL_RANGE);   // landscape?
    //bubbleY = LEVEL_Y_CENTER + LEVEL_SCALE*((2*LEVEL_RANGE/(1+exp(-inclinometerX/LEVEL_SIGMOID))) - LEVEL_RANGE);   // landscape?
    myGLCD.setColor(255,0,0);
    myGLCD.fillCircle(bubbleX, bubbleY, 20);  // portrait
    //myGLCD.fillCircle(bubbleX, bubbleY, 20);  // landscape?
  } 
      
  // range rings
  myGLCD.setColor(0,255,0);
  myGLCD.drawCircle(LEVEL_X_CENTER, LEVEL_Y_CENTER, LEVEL_RADIUS1);
  myGLCD.drawCircle(LEVEL_X_CENTER, LEVEL_Y_CENTER, LEVEL_RADIUS2);
  myGLCD.drawCircle(LEVEL_X_CENTER, LEVEL_Y_CENTER, LEVEL_RADIUS3);
  // crosshairs
  myGLCD.drawLine(LEVEL_X_CENTER, LEVEL_Y_CENTER-WIDTH/2, LEVEL_X_CENTER, LEVEL_Y_CENTER+WIDTH/2);
  myGLCD.drawLine(LEVEL_X_CENTER-WIDTH/2, LEVEL_Y_CENTER, LEVEL_X_CENTER+WIDTH/2, LEVEL_Y_CENTER);

  // draw or erase TIMEOUT message
  if (timeoutFlag) {
    myGLCD.setColor(255,0,0);
    myGLCD.print("NOT UPDATING",CENTER, 200);
  } else {
    // erase NOT UPDATING message
    myGLCD.setColor(0,0,0);
    myGLCD.fillRect(0,180,WIDTH,264);
  }
  //myGLCD.setDisplayPage(page);
}



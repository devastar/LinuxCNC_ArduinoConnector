/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com 2022

  This Software is used as IO Expansion for LinuxCNC. Here i am using a Mega 2560.

  It is NOT intended for timing and security relevant IO's. Don't use it for Emergency Stops or Endstop switches!

  You can create as many digital & analog Inputs, Outputs and PWM Outputs as your Arduino can handle.
  You can also generate "virtual Pins" by using latching Potentiometers, which are connected to one analog Pin, but are read in Hal as individual Pins.

  Currently the Software Supports:
  - analog Inputs
  - latching Potentiometers
  - 1 binary encoded selector Switch
  - digital Inputs
  - digital Outputs
  - Matrix Keypad
  - Multiplexed LEDs
  - Quadrature encoders
  - Joysticks

  The Send and receive Protocol is <Signal><PinNumber>:<Pin State>
  To begin Transmitting Ready is send out and expects to receive E: to establish connection. Afterwards Data is exchanged.
  Data is only send everythime it changes once.

  Inputs & Toggle Inputs  = 'I' -write only  -Pin State: 0,1
  Outputs                 = 'O' -read only   -Pin State: 0,1
  PWM Outputs             = 'P' -read only   -Pin State: 0-255
  Digital LED Outputs     = 'D' -read only   -Pin State: 0,1
  Analog Inputs           = 'A' -write only  -Pin State: 0-1024
  Latching Potentiometers = 'L' -write only  -Pin State: 0-max Position
  binary encoded Selector = 'K' -write only  -Pin State: 0-32
  rotary encoder          = 'R' -write only  -Pin State: up/ down / -2147483648 to 2147483647
  joystick                = 'R' -write only  -Pin State: up/ down / -2147483648 to 2147483647
  multiplexed LEDs        = 'M' -read only   -Pin State: 0,1

Keyboard Input:
  Matrix Keypad           = 'M' -write only  -Pin State: 0,1

Communication Status      = 'E' -read/Write  -Pin State: 0:0

  The Keyboard is encoded in the Number of the Key in the Matrix. The according Letter is defined in the receiving end in the Python Skript.
  Here you only define the Size of the Matrix.

  Command 'E0:0' is used for connectivity checks and is send every 5 seconds as keep alive signal. If the Signal is not received again, the Status LED will Flash.
  The Board will still work as usual and try to send it's data, so this feature is only to inform the User.


  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/




//###################################################IO's###################################################


#define INPUTS
#ifdef INPUTS
  const int Inputs = 3;         // One digital input
  int InPinmap[] = {2,3,4};         // D2 for power on switch
#endif

#define AINPUTS
#ifdef AINPUTS
  const int AInputs = 2;        // Two analog inputs
  int AInPinmap[] = {0, 1};     // A0 (feed override), A1 (spindle override)
  int smooth = 200;             // Denoising samples
#endif

//#define OUTPUTS                     //Use Arduino IO's as Outputs. Define how many Outputs you want in total and then which Pins you want to be Outputs.
#ifdef OUTPUTS
  const int Outputs = 2;              //number of outputs
  int OutPinmap[] = {11,12};
#endif

//#define PWMOUTPUTS                     //Use Arduino PWM Capable IO's as PWM Outputs. Define how many  PWM Outputs you want in total and then which Pins you want to be  PWM Outputs.
#ifdef PWMOUTPUTS
  const int PwmOutputs = 2;              //number of outputs
  int PwmOutPinmap[] = {12,11};
#endif



/*This is a special mode of AInputs. My machine had originally Selector Knobs with many Pins on the backside to select different Speed Settings.
I turned them into a "Potentiometer" by connecting all Pins with 10K Resistors in series. Then i applied GND to the first and 5V to the last Pin.
Now the Selector is part of an Voltage Divider and outputs different Voltage for each Position. This function generates Pins for each Position in Linuxcnc Hal.

It can happen, that when you switch position, that the selector is floating for a brief second. This might be detected as Position 0.
This shouldn't be an issue in most usecases, but think about that in your application.



Connect it to an Analog In Pin of your Arduino and define how many of these you want.
Then in the Array, {which Pin, How many Positions}
Note that Analog Pin numbering is different to the Print on the PCB.

*/
//#define LPOTIS
#ifdef LPOTIS
  const int LPotis = 2;
  const int LPotiPins[LPotis][2] = {
                    {1,9},             //Latching Knob Spindle Overdrive on A1, has 9 Positions
                    {2,4}              //Latching Knob Feed Resolution on A2, has 4 Positions
                    };
  int margin = 20;                      //giving it some margin so Numbers dont jitter, make this number smaller if your knob has more than 50 Positions
#endif



//#define BINSEL                   //Support of an Rotating Knob that was build in my Machine. It encodes 32 Positions with 5 Pins in Binary. This will generate 32 Pins in LinuxCNC Hal.
#ifdef BINSEL
  const int BinSelKnobPins[] = {2,6,4,3,5};  //1,2,4,8,16
#endif


//#define QUADENC
//Support for Quadrature Encoders. Define Pins for A and B Signals for your encoders. Visit https://www.pjrc.com/teensy/td_libs_Encoder.html for further explanation.
// Download Zip from here: https://github.com/PaulStoffregen/Encoder and import as Library to your Arduino IDE.
#ifdef QUADENC
  #include <Encoder.h>
  #define QUADENCS 2  //how many Rotary Encoders do you want?

    // Encoders have 2 signals, which must be connected to 2 pins. There are three options.

    //Best Performance: Both signals connect to interrupt pins.
    //Good Performance: First signal connects to an interrupt pin, second to a non-interrupt pin.
    //Low Performance: Both signals connect to non-interrupt pins, details below.

    //Board	            Interrupt Pins	            LED Pin(do not use)
    //Teensy 4.0 - 4.1	All Digital Pins	          13
    //Teensy 3.0 - 3.6	All Digital Pins	          13
    //Teensy LC	        2 - 12, 14, 15, 20 - 23	    13
    //Teensy 2.0	      5, 6, 7, 8	                11
    //Teensy 1.0	      0, 1, 2, 3, 4, 6, 7, 16
    //Teensy++ 2.0	    0, 1, 2, 3, 18, 19, 36, 37  6
    //Teensy++ 1.0	    0, 1, 2, 3, 18, 19, 36, 37
    //Arduino Due	      All Digital Pins	          13
    //Arduino Uno	      2, 3	                      13
    //Arduino Leonardo	0, 1, 2, 3	                13
    //Arduino Mega	    2, 3, 18, 19, 20, 21	      13
    //Sanguino	        2, 10, 11	                  0

Encoder Encoder0(2,3);      //A,B Pin
Encoder Encoder1(31,33);    //A,B Pin
//Encoder Encoder2(A,B);
//Encoder Encoder3(A,B);
//Encoder Encoder4(A,B);
  const int QuadEncSig[] = {2,2};   //define wich kind of Signal you want to generate.
                                  //1= send up or down signal (typical use for selecting modes in hal)
                                  //2= send position signal (typical use for MPG wheel)
  const int QuadEncMp[] = {4,4};   //some Rotary encoders send multiple Electronical Impulses per mechanical pulse. How many Electrical impulses are send for each mechanical Latch?

#endif

//#define JOYSTICK                   //Support of an Rotating Knob that was build in my Machine. It encodes 32 Positions with 5 Pins in Binary. This will generate 32 Pins in LinuxCNC Hal.
#ifdef JOYSTICK
const int JoySticks = 1;             // Number of potentiometers connected
const int JoyStickPins[JoySticks*2] = {0, 1}; // Analog input pins for the potentiometers
const int middleValue = 512;        // Middle value of the potentiometer
const int deadband = 20;            // Deadband range around the middleValue
const float scalingFactor = 0.01;   // Scaling factor to control the impact of distanceFromMiddle
#endif






//The Software will detect if there is an communication issue. When you power on your machine, the Buttons etc won't work, till LinuxCNC is running. THe StatusLED will inform you about the State of Communication.
// Slow Flash = Not Connected
// Steady on = connected
// short Flash = connection lost.

// if connection is lost, something happened. (Linuxcnc was closed for example or USB Connection failed.) It will recover when Linuxcnc is restartet. (you could also run "unloadusr arduino", "loadusr arduino" in Hal)
// Define an Pin you want to connect the LED to. it will be set as Output indipendand of the OUTPUTS function, so don't use Pins twice.
// If you use Digital LED's such as WS2812 or PL9823 (only works if you set up the DLED settings below) you can also define a position of the LED. In this case StatLedPin will set the number of the Digital LED Chain.

#define STATUSLED
#ifdef STATUSLED
  const int StatLedPin = 0;                //Pin for Status LED
  const int StatLedErrDel[] = {1000,10};   //Blink Timing for Status LED Error (no connection)
  const int DLEDSTATUSLED = 0;              //set to 1 to use Digital LED instead. set StatLedPin to the according LED number in the chain.
#endif




/* Instead of connecting LED's to Output pins, you can also connect digital LED's such as WS2812 or PL9823.
This way you can have how many LED's you want and also define it's color with just one Pin.

DLEDcount defines, how many Digital LED's you want to control. Count from 0. For Each LED an output Pin will be generated in LinuxCNC hal.
To use this funcion you need to have the Adafruit_NeoPixel.h Library installed in your Arduino IDE.

In LinuxCNC you can set the Pin to HIGH and LOW, for both States you can define an color per LED.
This way, you can make them glow or shut of, or have them Change color, from Green to Red for example.

DledOnColors defines the color of each LED when turned "on". For each LED set {Red,Green,Blue} with Numbers from 0-255.
depending on the Chipset of your LED's Colors might be in a different order. You can try it out by setting {255,0,0} for example.

You need to define a color to DledOffColors too. Like the Name suggests it defines the color of each LED when turned "off".
If you want the LED to be off just define {0,0,0}, .


If you use STATUSLED, it will also take the colors of your definition here.
*/

#define DLED
#ifdef DLED
  #include <Adafruit_NeoPixel.h>

  const int DLEDcount = 1;      // One WS2812B LED
  const int DLEDPin = 8;        // D3 for WS2812B
  const int DLEDBrightness = 100;// Brightness (0-100%)

  int DledOnColors[DLEDcount][3] = {
    {0, 255, 0}                 // Green when ON
  };
  int DledOffColors[DLEDcount][3] = {
    {255, 0, 0}                   // Red when OFF
  };

Adafruit_NeoPixel strip(DLEDcount, DLEDPin, NEO_GRB + NEO_KHZ800);//Color sequence is different for LED Chipsets. Use RGB for WS2812  or GRB for PL9823.


#endif
/*
Matrix Keypads are supported. The input is NOT added as HAL Pin to LinuxCNC. Instead it is inserted to Linux as Keyboard direktly.
So you could attach a QWERT* Keyboard to the arduino and you will be able to write in Linux with it (only while LinuxCNC is running!)
*/
//#define KEYPAD
#ifdef KEYPAD
const int numRows = 4;  // Define the number of rows in the matrix
const int numCols = 4;  // Define the number of columns in the matrix

// Define the pins connected to the rows and columns of the matrix
const int rowPins[numRows] = {2, 3, 4, 5};
const int colPins[numCols] = {6, 7, 8, 9};
int keys[numRows][numCols] = {0};
int lastKey= -1;
#endif


//#define MULTIPLEXLEDS // Special mode for Multiplexed LEDs. This mode is experimental and implemented to support Matrix Keyboards with integrated Key LEDs.
// check out this thread on LinuxCNC Forum for context. https://forum.linuxcnc.org/show-your-stuff/49606-matrix-keyboard-controlling-linuxcnc
// for Each LED an Output Pin is generated in LinuxCNC.

//If your Keyboard shares pins with the LEDs, you have to check polarity.
//rowPins[numRows] = {} are Pullup Inputs
//colPins[numCols] = {} are GND Pins
//the matrix keyboard described in the thread shares GND Pins between LEDs and KEys, therefore LedGndPins[] and colPins[numCols] = {} use same Pins.

#ifdef MULTIPLEXLEDS

const int numVccPins = 8;      // Number of rows in the matrix
const int numGndPins = 8;      // Number of columns in the matrix
const int LedVccPins[] = {30,31,32,33,34,35,36,37}; // Arduino pins connected to rows
const int LedGndPins[] = {40,41,42,43,44,45,46,47}; // Arduino pins connected to columns

// Define the LED matrix
int ledStates[numVccPins*numGndPins] = {0};

unsigned long previousMillis = 0;
const unsigned long interval = 500; // Time (in milliseconds) per LED display

int currentLED = 0;
#endif




//#define DEBUG
//#######################################   END OF CONFIG     ###########################

//###Misc Settings###
const int timeout = 10000;   // timeout after 10 sec not receiving Stuff
const int debounceDelay = 50;


//Variables for Saving States
#ifdef INPUTS
  int InState[Inputs];
  int oldInState[Inputs];
  unsigned long lastInputDebounce[Inputs];
#endif
#ifdef SINPUTS
  int sInState[sInputs];
  int soldInState[sInputs];
  int togglesinputs[sInputs];
  unsigned long lastsInputDebounce[sInputs];
#endif
#ifdef OUTPUTS
  int OutState[Outputs];
  int oldOutState[Outputs];
#endif
#ifdef PWMOUTPUTS
  int OutPWMState[PwmOutputs];
  int oldOutPWMState[PwmOutputs];
#endif
#ifdef AINPUTS
  int oldAinput[AInputs];
  unsigned long sumAinput[AInputs];
#endif
#ifdef LPOTIS
  int Lpoti[LPotis];
  int oldLpoti[LPotis];
#endif
#ifdef BINSEL
  int oldAbsEncState;
#endif
#ifdef KEYPAD
  byte KeyState = 0;
#endif
#ifdef MULTIPLEXLEDS
  byte KeyLedStates[numVccPins*numGndPins];
#endif
#if QUADENCS == 1
  const int QuadEncs = 1;
#endif
#if QUADENCS == 2
  const int QuadEncs = 2;
#endif
#if QUADENCS == 3
  const int QuadEncs = 3;
#endif
#if QUADENCS == 4
  const int QuadEncs = 4;
#endif
#if QUADENCS == 5
  const int QuadEncs = 5;
#endif
#ifdef QUADENC
  long EncCount[QuadEncs];
  long OldEncCount[QuadEncs];
#endif


#ifdef JOYSTICK
long counter[JoySticks*2] = {0};      // Initialize an array for the counters
long prevCounter[JoySticks*2] = {0};  // Initialize an array for the previous counters
float incrementFactor[JoySticks*2] = {0.0}; // Initialize an array for the incrementFactors
unsigned long lastUpdateTime[JoySticks*2] = {0}; // Store the time of the last update for each potentiometer

#endif

//### global Variables setup###
//Please don't touch them
unsigned long oldmillis = 0;
unsigned long newcom = 0;
unsigned long lastcom = 0;
int connectionState = 0;

#define STATE_CMD 0
#define STATE_IO 1
#define STATE_VALUE 2


byte state = STATE_CMD;
char inputbuffer[5];
byte bufferIndex = 0;
char cmd = 0;
uint16_t io = 0;
uint16_t value = 0;

// Function Prototypes
void readCommands();
void commandReceived(char cmd, uint16_t io, uint16_t value);
void multiplexLeds();
void readKeypad();
int readAbsKnob();
void readsInputs();
void readInputs();
void readAInputs();
void readLPoti();
void controlDLED(int Pin, int Stat);
void initDLED();
void writePwmOutputs(int Pin, int Stat);
void writeOutputs(int Pin, int Stat);
void StatLedErr(int offtime, int ontime);
void flushSerial();
void sendData(char sig, int pin, int state);
void reconnect();
void comalive();
void readEncoders();
void readJoySticks();

void setup() {

#ifdef INPUTS
//setting Inputs with internal Pullup Resistors
  for(int i= 0; i<Inputs;i++){
    pinMode(InPinmap[i], INPUT_PULLUP);
    oldInState[i] = -1;
    }
#endif

#ifdef SINPUTS
//setting Inputs with internal Pullup Resistors
  for(int i= 0; i<sInputs;i++){
    pinMode(sInPinmap[i], INPUT_PULLUP);
    soldInState[i] = -1;
    togglesinputs[i] = 0;
  }

#endif
#ifdef AINPUTS

  for(int i= 0; i<AInputs;i++){
    pinMode(AInPinmap[i], INPUT);
    oldAinput[i] = -1;
    sumAinput[i] = 0;
    }
#endif
#ifdef OUTPUTS
  for(int o= 0; o<Outputs;o++){
    pinMode(OutPinmap[o], OUTPUT);
    oldOutState[o] = 0;
    }
#endif

#ifdef PWMOUTPUTS
  for(int o= 0; o<PwmOutputs;o++){
    pinMode(PwmOutPinmap[o], OUTPUT);
    oldOutPWMState[o] = 0;
    }
#endif
#ifdef STATUSLED
  pinMode(StatLedPin, OUTPUT);
#endif

#ifdef BINSEL
  pinMode(BinSelKnobPins[0], INPUT_PULLUP);
  pinMode(BinSelKnobPins[1], INPUT_PULLUP);
  pinMode(BinSelKnobPins[2], INPUT_PULLUP);
  pinMode(BinSelKnobPins[3], INPUT_PULLUP);
  pinMode(BinSelKnobPins[4], INPUT_PULLUP);
#endif

#ifdef DLED
  initDLED();
#endif

#ifdef KEYPAD
for(int col = 0; col < numCols; col++) {
  for (int row = 0; row < numRows; row++) {
    keys[row][col] = row * numCols + col;
  }
}
#endif


//Setup Serial
  Serial.begin(115200);
  while (!Serial){}
  comalive();
}


void loop() {

  readCommands(); //receive and execute Commands
  comalive(); //if nothing is received for 10 sec. blink warning LED


#ifdef INPUTS
  readInputs(); //read Inputs & send data
#endif
#ifdef SINPUTS
  readsInputs(); //read Inputs & send data
#endif
#ifdef AINPUTS
  readAInputs();  //read Analog Inputs & send data
#endif
#ifdef LPOTIS
  readLPoti(); //read LPotis & send data
#endif
#ifdef BINSEL
  readAbsKnob(); //read ABS Encoder & send data
#endif

#ifdef KEYPAD
  readKeypad(); //read Keyboard & send data
#endif

#ifdef QUADENC
  readEncoders(); //read Encoders & send data
#endif

#ifdef JOYSTICK
  readJoySticks(); //read Encoders & send data
#endif
#ifdef MULTIPLEXLEDS
  multiplexLeds();// cycle through the 2D LED Matrix}
#endif
}

#ifdef JOYSTICK

void readJoySticks() {
  for (int i = 0; i < JoySticks*2; i++) {
    unsigned long currentTime = millis(); // Get the current time

    // Check if it's time to update the counter for this potentiometer
    if (currentTime - lastUpdateTime[i] >= 100) { // Adjust 100 milliseconds based on your needs
      lastUpdateTime[i] = currentTime; // Update the last update time for this potentiometer

      int potValue = analogRead(JoyStickPins[i]); // Read the potentiometer value

      // Calculate the distance of the potentiometer value from the middle
      int distanceFromMiddle = potValue - middleValue;

      // Apply deadband to ignore small variations around middleValue
      if (abs(distanceFromMiddle) <= deadband) {
        incrementFactor[i] = 0.0; // Set incrementFactor to 0 within the deadband range
      } else {
        // Apply non-linear scaling to distanceFromMiddle to get the incrementFactor
        incrementFactor[i] = pow((distanceFromMiddle * scalingFactor), 3);
      }

      // Update the counter if the incrementFactor has reached a full number
      if (incrementFactor[i] >= 1.0 || incrementFactor[i] <= -1.0) {
        counter[i] += static_cast<long>(incrementFactor[i]); // Increment or decrement the counter by the integer part of incrementFactor
        incrementFactor[i] -= static_cast<long>(incrementFactor[i]); // Subtract the integer part from incrementFactor
      }

      // Check if the counter value has changed
      if (counter[i] != prevCounter[i]) {
        sendData('R',JoyStickPins[i],counter[i]);
        // Update the previous counter value with the current counter value
        prevCounter[i] = counter[i];
      }
    }
  }
}
#endif

#ifdef QUADENC
void readEncoders(){
    if(QuadEncs>=1){
      #if QUADENCS >= 1
        EncCount[0] = Encoder0.read()/QuadEncMp[0];
      #endif
    }
    if(QuadEncs>=2){
      #if QUADENCS >= 2
        EncCount[1] = Encoder1.read()/QuadEncMp[1];
      #endif
    }
    if(QuadEncs>=3){
      #if QUADENCS >= 3
        EncCount[2] = Encoder2.read()/QuadEncMp[2];
      #endif
    }
    if(QuadEncs>=4){
      #if QUADENCS >= 4
        EncCount[3] = Encoder3.read()/QuadEncMp[3];
      #endif
    }
    if(QuadEncs>=5){
      #if QUADENCS >= 5
        EncCount[4] = Encoder4.read()/QuadEncMp[4];
      #endif
    }

    for(int i=0; i<QuadEncs;i++){
      if(QuadEncSig[i]==2){
        if(OldEncCount[i] != EncCount[i]){
          sendData('R',i,EncCount[i]);//send Counter
          OldEncCount[i] = EncCount[i];
        }
      }
      if(QuadEncSig[i]==1){
        if(OldEncCount[i] < EncCount[i]){
        sendData('R',i,1); //send Increase by 1 Signal
        OldEncCount[i] = EncCount[i];
        }
        if(OldEncCount[i] > EncCount[i]){
        sendData('R',i,0); //send Increase by 1 Signal
        OldEncCount[i] = EncCount[i];
        }
      }
    }
}

#endif

void comalive(){
  if(lastcom == 0){ //no connection yet. send E0:0 periodicly and wait for response
    while (lastcom == 0){
      readCommands();
      flushSerial();
      Serial.println("E0:0");
      delay(200);
      #ifdef STATUSLED
        StatLedErr(1000,1000);
      #endif
    }
    connectionState = 1;
    flushSerial();
    #ifdef DEBUG
      Serial.println("first connect");
    #endif
  }
  if(millis() - lastcom > timeout){
  #ifdef STATUSLED
     StatLedErr(500,200);
  #endif
      if(connectionState == 1){
        #ifdef DEBUG
          Serial.println("disconnected");
        #endif
        connectionState = 2;
      }

   }
   else{
      connectionState=1;
      #ifdef STATUSLED
        if(DLEDSTATUSLED == 1){
          #ifdef DLED
            controlDLED(StatLedPin, 1);
          #endif
        }
        else{
          digitalWrite(StatLedPin, HIGH);
        }
      #endif
    }
}


void reconnect(){
  #ifdef DEBUG
    Serial.println("reconnected");
    Serial.println("resending Data");
  #endif

  #ifdef INPUTS
    for (int x = 0; x < Inputs; x++){
      InState[x]= -1;
    }
  #endif
  #ifdef SINPUTS
    for (int x = 0; x < sInputs; x++){
      soldInState[x]= -1;
      togglesinputs[x] = 0;
    }
  #endif
  #ifdef AINPUTS
    for (int x = 0; x < AInputs; x++){
      oldAinput[x] = -1;
      sumAinput[x] = 0;
    }
  #endif
  #ifdef LPOTIS
    for (int x = 0; x < LPotis; x++){
      oldLpoti[x] = -1;
    }
  #endif
  #ifdef BINSEL
    oldAbsEncState = -1;
  #endif


  #ifdef INPUTS
    readInputs(); //read Inputs & send data
  #endif
  #ifdef SINPUTS
    readsInputs(); //read Inputs & send data
  #endif
  #ifdef AINPUTS
    readAInputs();  //read Analog Inputs & send data
  #endif
  #ifdef LPOTIS
    readLPoti(); //read LPotis & send data
  #endif
  #ifdef BINSEL
    readAbsKnob(); //read ABS Encoder & send data
  #endif
  #ifdef MULTIPLEXLEDS
    multiplexLeds(); //Flash LEDS.
  #endif

  connectionState = 1;


}


void sendData(char sig, int pin, int state){
        Serial.print(sig);
        Serial.print(pin);
        Serial.print(":");
        Serial.println(state);
}

void flushSerial(){
  while (Serial.available() > 0) {
  Serial.read();
  }
}

#ifdef STATUSLED
void StatLedErr(int offtime, int ontime){
  unsigned long newMillis = millis();

  if (newMillis - oldmillis >= offtime){
      #ifdef DLED
        if(DLEDSTATUSLED == 1){
          controlDLED(StatLedPin, 1);}
      #endif
      if(DLEDSTATUSLED == 0){digitalWrite(StatLedPin, HIGH);}
    }
  if (newMillis - oldmillis >= offtime+ontime){{
      #ifdef DLED
        if(DLEDSTATUSLED == 1){
          controlDLED(StatLedPin, 0);}
      #endif
      if(DLEDSTATUSLED == 0){digitalWrite(StatLedPin, LOW);}

      oldmillis = newMillis;

    }
  }

}
#endif

#ifdef OUTPUTS
void writeOutputs(int Pin, int Stat){
  digitalWrite(Pin, Stat);
}
#endif

#ifdef PWMOUTPUTS
void writePwmOutputs(int Pin, int Stat){
  analogWrite(Pin, Stat);
}

#endif

#ifdef DLED
void initDLED(){
  strip.begin();
  strip.setBrightness(DLEDBrightness);

    for (int i = 0; i < DLEDcount; i++) {
    strip.setPixelColor(i, strip.Color(DledOffColors[i][0],DledOffColors[i][1],DledOffColors[i][2]));
    }
  strip.show();
  #ifdef DEBUG
    Serial.print("DLED initialised");
  #endif
}

void controlDLED(int Pin, int Stat){
  if(Stat == 1){

    strip.setPixelColor(Pin, strip.Color(DledOnColors[Pin][0],DledOnColors[Pin][1],DledOnColors[Pin][2]));
    #ifdef DEBUG
      Serial.print("DLED No.");
      Serial.print(Pin);
      Serial.print(" set to:");
      Serial.println(Stat);

    #endif
    }
    else{

      strip.setPixelColor(Pin, strip.Color(DledOffColors[Pin][0],DledOffColors[Pin][1],DledOffColors[Pin][2]));
      #ifdef DEBUG
        Serial.print("DLED No.");
        Serial.print(Pin);
        Serial.print(" set to:");
        Serial.println(Stat);

      #endif
    }
  strip.show();
  }
#endif

#ifdef LPOTIS
void readLPoti(){
    for(int i= 0;i<LPotis; i++){
      int var = analogRead(LPotiPins[i][0])+margin;
      int pos = 1024/(LPotiPins[i][1]-1);
      var = var/pos;
      if(oldLpoti[i]!= var){
        oldLpoti[i] = var;
        sendData('L', LPotiPins[i][0],oldLpoti[i]);
      }
    }
}
#endif


#ifdef AINPUTS
void readAInputs(){
  static unsigned int samplecount = 0;

   for(int i= 0;i<AInputs; i++){

    if (samplecount < smooth) {
      sumAinput[i] = sumAinput[i] + analogRead(AInPinmap[i]);
    }
    else {
      sumAinput[i] = sumAinput[i] / smooth;
      if(oldAinput[i]!= sumAinput[i]){
        oldAinput[i] = sumAinput[i];
        sendData('A',AInPinmap[i],oldAinput[i]);
      }
      sumAinput[i] = 0;
    }
   }

  if(samplecount < smooth){ samplecount = samplecount + 1;}
  else {samplecount = 0;}

}
#endif
#ifdef INPUTS
void readInputs(){
    for(int i= 0;i<Inputs; i++){
      int State = digitalRead(InPinmap[i]);
      if(InState[i]!= State && millis()- lastInputDebounce[i] > debounceDelay){
        InState[i] = State;
        sendData('I',InPinmap[i],InState[i]);

      lastInputDebounce[i] = millis();
      }
    }
}
#endif
#ifdef SINPUTS
void readsInputs(){
  for(int i= 0;i<sInputs; i++){
    sInState[i] = digitalRead(sInPinmap[i]);
    if (sInState[i] != soldInState[i] && millis()- lastsInputDebounce[i] > debounceDelay){
      // Button state has changed and debounce delay has passed

      if (sInState[i] == LOW || soldInState[i]== -1) { // Stuff after || is only there to send States at Startup
        // Button has been pressed
        togglesinputs[i] = !togglesinputs[i];  // Toggle the LED state

        if (togglesinputs[i]) {
          sendData('I',sInPinmap[i],togglesinputs[i]);  // Turn the LED on
        }
        else {
          sendData('I',sInPinmap[i],togglesinputs[i]);   // Turn the LED off
        }
      }
      soldInState[i] = sInState[i];
      lastsInputDebounce[i] = millis();
    }
  }
}
#endif

#ifdef BINSEL
int readAbsKnob(){
  int var = 0;
  if(digitalRead(BinSelKnobPins[0])==1){
    var += 1;
  }
  if(digitalRead(BinSelKnobPins[1])==1){
    var += 2;
  }
  if(digitalRead(BinSelKnobPins[2])==1){
    var += 4;
  }
  if(digitalRead(BinSelKnobPins[3])==1){
    var += 8;
  }
  if(digitalRead(BinSelKnobPins[4])==1){
    var += 16;
  }
  if(var != oldAbsEncState){
    Serial.print("K0:");
    Serial.println(var);
    }
  oldAbsEncState = var;
  return (var);
}
#endif

#ifdef KEYPAD
void readKeypad(){
  //detect if Button is Pressed
  for (int col = 0; col < numCols; col++) {
    pinMode(colPins[col], OUTPUT);
    digitalWrite(colPins[col], LOW);
    // Read the state of the row pins
    for (int row = 0; row < numRows; row++) {
      pinMode(rowPins[row], INPUT_PULLUP);
      if (digitalRead(rowPins[row]) == LOW && lastKey != keys[row][col]) {
        // A button has been pressed
        sendData('M',keys[row][col],1);
        lastKey = keys[row][col];
        break;

      }
      if (digitalRead(rowPins[row]) == HIGH && lastKey == keys[row][col]) {
        // The Last Button has been unpressed
        sendData('M',keys[row][col],0);
        lastKey = -1; //reset Key pressed
        break;
      }
    }

    // Set the column pin back to input mode
    pinMode(colPins[col], INPUT);
  }

}
#endif

#ifdef MULTIPLEXLEDS
void multiplexLeds() {
  unsigned long currentMillis = millis();
  //init Multiplex
  #ifdef KEYPAD //if Keyboard is presend disable Pullup Resistors to not mess with LEDs while a Button is pressed.
    for (int row = 0; row < numRows; row++) {
      pinMode(rowPins[row], OUTPUT);
      digitalWrite(rowPins[row], LOW);
    }
  #endif

  for (int i = 0; i < numVccPins; i++) {
    pinMode(LedVccPins[i], OUTPUT);
    digitalWrite(LedVccPins[i], LOW); // Set to LOW to disable all Vcc Pins
  }
  for (int i = 0; i < numGndPins; i++) {
    pinMode(LedGndPins[i], OUTPUT);
    digitalWrite(LedGndPins[i], HIGH); // Set to HIGH to disable all GND Pins
  }

  for(currentLED = 0; currentLED < numVccPins*numGndPins ;currentLED ++){
    if(ledStates[currentLED] == 1){                         //only handle turned on LEDs
      digitalWrite(LedVccPins[currentLED/numVccPins],HIGH); //turn current LED on
      digitalWrite(LedGndPins[currentLED%numGndPins],LOW);

      #ifdef debug
        Serial.print("VCC: ");
        Serial.print(LedVccPins[currentLED/numVccPins]);
        Serial.print(" GND: ");
        Serial.println(LedGndPins[currentLED%numGndPins]);
      #endif

      delayMicroseconds(interval);                          //wait couple ms
      digitalWrite(LedVccPins[currentLED/numVccPins],LOW);  //turn off and go to next one
      digitalWrite(LedGndPins[currentLED%numGndPins],HIGH);
    }
  }
/*
  }
  if(ledStates[currentLED]==0){//If currentLED is Off, manage next one.
    currentLED++;
  }
  if(currentLED >= numVccPins*numGndPins){
      currentLED= 0;
  }
  */
}
#endif

void commandReceived(char cmd, uint16_t io, uint16_t value){
  #ifdef OUTPUTS
  if(cmd == 'O'){
    writeOutputs(io,value);
    lastcom=millis();

  }
  #endif
  #ifdef PWMOUTPUTS
  if(cmd == 'P'){
    writePwmOutputs(io,value);
    lastcom=millis();

  }
  #endif
  #ifdef DLED
  if(cmd == 'D'){
    controlDLED(io,value);
    lastcom=millis();
    #ifdef debug
      Serial.print("DLED:");
      Serial.print(io);
      Serial.print(" State:");
      Serial.println(DLEDstate[io]);
    #endif

  }
  #endif
  #ifdef MULTIPLEXLEDS
    if(cmd == 'M'){
      ledStates[io] = value; // Set the LED state
      lastcom=millis();
      #ifdef DEBUG
        Serial.print("multiplexed Led No:");
        Serial.print(io);
        Serial.print("Set to:");
        Serial.println(ledStates[io]);
      #endif

  }
  #endif


  if(cmd == 'E'){
    lastcom=millis();
    if(connectionState == 2){
     reconnect();
    }
  }


  #ifdef DEBUG
    Serial.print("I Received= ");
    Serial.print(cmd);
    Serial.print(io);
    Serial.print(":");
    Serial.println(value);
  #endif
}


void readCommands(){
    byte current;
    while(Serial.available() > 0){
        current = Serial.read();
        switch(state){
            case STATE_CMD:
                   cmd = current;
                   state = STATE_IO;
                   bufferIndex = 0;
                break;
            case STATE_IO:
                if(isDigit(current)){
                    inputbuffer[bufferIndex++] = current;
                }else if(current == ':'){
                    inputbuffer[bufferIndex] = 0;
                    io = atoi(inputbuffer);
                    state = STATE_VALUE;
                    bufferIndex = 0;

                }
                else{
                    #ifdef DEBUG
                    Serial.print("Ungültiges zeichen: ");
                    Serial.println(current);
                    #endif
                }
                break;
            case STATE_VALUE:
                if(isDigit(current)){
                    inputbuffer[bufferIndex++] = current;
                }
                else if(current == '\n'){
                    inputbuffer[bufferIndex] = 0;
                    value = atoi(inputbuffer);
                    commandReceived(cmd, io, value);
                    state = STATE_CMD;
                }
                else{
                  #ifdef DEBUG
                  Serial.print("Ungültiges zeichen: ");
                  Serial.println(current);
                  #endif

                }
                break;
        }

    }
}

/*
  Second fully integrated code.
  V2 includes the addition of a potential fix for instability in a real signal. 
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

//--------------------------------Variables-------------------------------

//Pin Locations
const int switchPin = 12; //Switch connected to pin 12, used for swapping APM and NATO
const int buttonZone = 3; //Button connected to pin 3, used for swapping timezones
const int buttonSync = 2; //Button connected to pin 2, used for initiating a sync
const int signalPin = 4; //Input for PWM, coming in on pin 4
const int manualPin = 11; //Button connected to pin 13
const int buttonOne = 10; //Button connected to pin 10
const int buttonTwo = 9; //Button connected to pin 9

//-----------DATA COLLECTION Variables-----------//
int buttonState = 0;
volatile int timecode[60] = {}; //Array - holds time signature values
volatile int currentVal; //Integer - holds the current digital signal
volatile unsigned long signalLength; //Unsigned Long - holds the amount of times we've sampled in one second
volatile unsigned long dutyLength; //Unsigned Long - Holds the amount of time a 1 has been sampled
volatile unsigned long zeroes; //Unsigned Long - Counts zeroes until we find a bit, ensuring we've found the right start
volatile float signalSize; //Float - dutyLength / signalLength
volatile int pastVal; //Integer - Holds the previous digital signal
volatile int pastBit; //Integer - Holds the previous bit 
volatile int presentBit; //Integer - Holds the present bit
volatile bool bitStartFound; //Boolean - Indicates the finding of the start of a bit
volatile bool bitCollected; //Boolean - Indicates the collection of an entire bit of data
volatile bool syncing; //Boolean - Indicates when we are syncing for a timecode
volatile bool signalStartFound; //Boolean - Indicates the finding of the start of an entire time signature

//-------DECODING Variables--------//
int newminute = 35; //Integer value for the new minute obtained through the sync process
int newhour = 22; //Integer value for the new hour obtained through the sync process
volatile int newday = 117; //Integer value for the new day obtained through the sync process
int newyear = 24; //Integer value for the new year obtained through the sync process
int DUTsign = 1; //Integer value signifying if there is a positive or negative DUT
float DUTvalue = 0.0; //Integer value for the DUT second difference
volatile bool LeapYear = true; //Is it a LeapYear at time of syncing?
bool LeapSecond = false; //Is there a LeapSecond during the month at the time of syncing?
int DSTstatus = 3; //00 = Not in effect, 01 = DST ends today, 10 = DST begins today, 11 = DST in effect
int newminutes[8] = {0,0,0,0,0,0,0,0}; //Integer Array for storing new minute bits
int newhours[7] = {0,0,0,0,0,0,0}; //Integer Array for storing new hour bits
volatile int newdays[12] = {0,0,0,0,0,0,0,0,0,0,0,0}; //Integer Array for storing new day bits
int DUTsigns[3] = {0,0,0}; //Integer Array for storing DUT sign bits
int DUTvalues[4] = {0,0,0,0}; //Integer Array for storing DUT value bits
int newyears[9] = {0,0,0,0,0,0,0,0,0}; //Integer Array for storing new year bits
int LeapsAndDST[4] = {0,0,0,0}; //Integer Array for storing Leap___ related bits and Daylight Saving Info

//-----------TIME SET Variables-----------//
// initial Time display is 11:59:54 PM on December 31st, 2024
static unsigned long lastInterruptTime = 0; //Unsigned long used to prevent debounce on timezone button
volatile int year = 24; //Volatile integer containing the current year value
volatile bool leapyearDummy; //Volatile boolean containing the previous "leapyear" status
volatile int mon = 4; //Volatile integer containing the current month value
volatile int day = 26; //Volatile integer containing the current day value
volatile int hourAPM = 10; //Volatile integer containing the current AP/PM hour value
volatile int hourNATO = 22; //Volatile integer containing the current NATO hour value
volatile int min = 44; //Volatile integer containing the current minute value
volatile int sec = 0; //Volatile integer containing the current second value
volatile int flag = 1; //Volatile integer funciton as an AM-PM Flag respectively
volatile int roll = 1; //Volatile integer used in the logic when rolling over from AM-PM or PM-AM respectively
volatile int trigger = 1; //Volatile integer which triggers the "Day" change when Midnight occurs
int timeswitch = 1; //Integer value which corresponds to input switch: 0 = AM/PM, 1 = NATO time display
volatile int timezone = 6; //Volatile int which initializes the timezone to west coast (GMT-06)
volatile int timezoneDummy = 0; //Volatile integer variable to compare changes in time when changing timezone
int count = 0; //Integer variable to count which stage of the manual set currently in
bool manual = false; //Boolean used to determine when manually setting time
int Button1 = 0; //Integer variable used to read "buttonOne"
int Button2 = 0; //Integer variable used to read "buttonTwo"

//------------------------------------------------------------------------

void setup() {
  Serial.begin(4800); //Uncomment for use of serial monitor
  // startMillis = millis(); // read RTC initial value  
  lcd.init();         // initialize the lcd
  lcd.backlight();    // Turn on the LCD screen backlight
  pinMode(switchPin, INPUT); //initialize the switchPin as input

  //Button Interrupt Setup
  pinMode(buttonSync, INPUT); //Intializes the sync button as an input
  pinMode(buttonZone, INPUT); //Initializes the timezone button as an input
  pinMode(manualPin, INPUT); //Initializes the "manualPin" as an input
  pinMode(buttonOne, INPUT); //Initializes the "buttonOne" as an input
  pinMode(buttonTwo, INPUT); //Initializes the "buttonTwo" as an input
  attachInterrupt(digitalPinToInterrupt(buttonZone), timezonechange, RISING); //Attaches an interrupt signal to the "buttonZone"

  pinMode(signalPin, INPUT); //Initializes the PWM pin as an input
}

//Primary looping function which runs continuously to keep time and check for inputs
void loop()
{   
  timeswitch = digitalRead(switchPin);
  manual = digitalRead(manualPin);

  if (manual == true) {
    manualSet();
  }

  buttonState = digitalRead(buttonSync);
  if((hourNATO == 22) && (min == 45) && (sec == 5)){
    buttonState = 1;
  }

  if(buttonState == 1)
  {
    lcd.clear();
    lcd.setCursor(0,0); //Sets LCD print cursor
    lcd.print("Syncing Time Signal");
    lcd.setCursor(0,1);
    lcd.print("Please be Patient :D");

    dataCollection();
    // Serial.println("Function Complete");
                  
              //--------Serial Print for Testing--------------
              Serial.print("[");
              for(int i = 0; i < 60; i++){
                Serial.print(timecode[i]); 
                Serial.print(",");
                }
              Serial.println("]");
              //----------------------------------------------
              
    variableWipe();
    breakArrays();
    getMinutes();
    getHours();
    getDays();
    getDUTSign();
    getDUTValue();
    getYears();
    getLeapsAndDST();
    lcd.clear();
    //------------Serial.print Values Used in Testing-------------//
    //------------------------------------------------------------//
    //  Serial.println("Beginning Print");
    //  Serial.println("Minutes: " + String(newminute));
    //  Serial.println("Hours: " + String(newhour));
    //  Serial.println("Day: " + String(newday));
    //  Serial.println("Year: " + String(newyear));
    //  Serial.println("DUT Sign: " + String(DUTsign));
    //  Serial.println("DUT Value: " + String(DUTvalue));
    //  Serial.println("Leap Year: " + String(LeapYear));
    //  Serial.println("Leap Second: " + String(LeapSecond));
    //  Serial.println("DST Status: " + String(DSTstatus));
    //------------------------------------------------------------//
    //------------------------------------------------------------//
    newdata();
  }

  //------------Serial.print Values Used in Testing-------------//
  //------------------------------------------------------------//
  // Serial.println(timezone);
  // Serial.println("APM Time: " + String(hourAPM));
  // Serial.println("NATO Time: " + String(hourNATO));
  // Serial.println("newday value: " + String(newday));
  // Serial.println("Roll: " + String(roll));
  // Serial.println("flag: " + String(flag));
  // Serial.println("DST: " + String(DSTstatus));
  //Update LCD Display
  //Print TIME in Hour, Min, Sec + AM/PM
  //------------------------------------------------------------//
  //------------------------------------------------------------//

  // Update LCD Display
  // Print TIME in Hour, Min, Sec + AM/PM
  lcd.setCursor(0,0); //Sets LCD print cursor
  if (timeswitch == 0) { //Determines which style of time to display
    if (hourAPM < 10) { //Checks if "hourAPM" is less than 10 and needs a 0 placed in front
      lcd.print("0"); //Ensures the "hoursAPM" is always written in a format of 2 digits
    } 
    lcd.print(hourAPM); 
    lcd.print(":");
  }
  else {
    if (hourNATO < 10) { //Checks if "hourNATO" is less than 10 and needs a 0 placed in front
      lcd.print("0"); //Ensures the "hoursNATO" is always written in a format of 2 digits
    }
    lcd.print(hourNATO);
    lcd.print(":");
  }

  if (min < 10) { //Checks if "min" is less than 10 and needs a 0 placed in front
    lcd.print("0"); //Ensures the "min" is always written in a format of 2 digits
  }
  lcd.print(min);
  lcd.print(":");

  if (sec < 10) { //Checks if "sec" is less than 10 and needs a 0 placed in front
    lcd.print("0"); //Ensures the "sec" is always written in a format of 2 digits
  }
  lcd.print(sec);
  lcd.print(" ");

  if (timeswitch == 0) { //Determines if the text 'AM' or 'PM' needs to be printed
    if (flag==0) lcd.print("AM ");
    if (flag==1) lcd.print("PM ");
  }
  lcd.print("@ GMT-0");
  lcd.print(timezone); //Prints current timezone relative to UTC signal

  if (timeswitch == 1) { //Determins if the text last 3 print locations need to be cleared
    lcd.print("   "); //Clears the last 3 print locations of row 1 on the LCD 
  }

  lcd.setCursor(0,1); //Sets LCD print cursor
  if (mon < 10) { //Checks if "mon" is less than 10 and needs a 0 placed in front
    lcd.print("0"); //Ensures the "mon" is always written in a format of 2 digits
  }
  lcd.print(mon);
  lcd.print("/");

  if (day < 10) { //Checks if "day" is less than 10 and needs a 0 placed in front
    lcd.print("0"); //Ensures the "day" is always written in a format of 2 digits
  }
  lcd.print(day);
  lcd.print("/");
  lcd.print(year);
  lcd.print("            "); //Prevents any writing errors with the "year" value and keeps the space blank

  delay(.93735922*1000); //Delays the program by ~937ms to account for timing of other operations in "1s" loop
  timeincrement(); //Increments the time keeping values
}

//----AM/PM-LCD SCREEN EXAMPLE----//    //-----NATO-LCD SCREEN EXAMPLE----// 
//--------------------------------//    //--------------------------------//
//--    11:59:54 PM @ GMT-06    --//    //--    23:59:54 @ GMT-06       --//
//--    12/31/23                --//    //--    12/31/23                --//
//--------------------------------//    //--------------------------------//


//======================================================================
//=====================Collection Methods===============================
//======================================================================

/*
 Function dataCollection
 no params

 Function will go through multiple search loops finding and decoding the given PWM
 1. Will first look for the start of a bit, as to start reading the bits in the correct place
 2. Will look for two "x" bits in a row (integer 2 in the code), these denote the beginning of a time signature
 3. Will scan each bit of the signal and store them into an array, until the array is full
 4. Once array is complete, will exit and pass data to the data decoding section
*/
void dataCollection()
{
  //Serial.println("interrupted");
  //Serial.println("Function entered");
  memset(timecode, 0, sizeof(timecode)); //resets array back to 0 for a clean timecode
  syncing = true; //Set to true to denote that we are in a sync and loop accordingly
  signalStartFound = false; //Set to false until we find signal start
  bitStartFound = false; //Set to false until we find bit start
  pastVal = 1; //Set to 1 so then we cant't get a 0 to 1 immediately
  pastBit = 0; //Set to 0 at the start to not get double buffer immediately
  zeroes = 0; //Before we start counting
  //Serial.println("Values initialized");
  while(syncing)
  {
    //Serial.println("Sync Loop Entered");
    if(signalStartFound)
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Signal Start Found -");
      lcd.setCursor(0,1);
      lcd.print("Collecting Signature");
      lcd.setCursor(0,3);
      lcd.print("Please be Patient :D");
      
      Serial.println("Starting main data collection");
      //Loop to collect data for the time signature
      //Decides bit, set it in place, resets variables, and iterates
      for(int count = 1; count <= 60; count = count + 1)
      {
        bitCollected = false;
        while(!bitCollected)
        {
          currentVal = digitalRead(signalPin); //set cV to current PWM digital value
          //Serial.println("Currently Reading: " + String(currentVal));
          dutyLength = dutyLength + currentVal; //Add cV to duty length, i.e. counting how long it is high
          signalLength = signalLength + 1; //Always add 1, to see how many times we sample
          if(currentVal == 1 && pastVal == 0) //If condition is true, indicates the end of a bit transmission and needs to be read
          {
            //--------------------------------------------
            //Need to add a layer of check, seeing if its a valid end. 
            //More than likely use existing code as an 'else' case
            //--------------------------------------------
            if(signalLength < 100000)
            {
              //This is a case where the transition is invalid, and we haven't seen enough samples
              if((signalLength - dutyLength) < 20000) //consider this a bad zero, backfill
              {
                dutyLength = signalLength; //Every sample should've been a 1 at this point
                pastVal = 1;
              }
              //else we dont backfill, considering it a proper zero and we've had a spike
              //dutyLength = signalLength; //Every sample should've been a 1 at this point
              //pastVal = 1;
            }
            else //This is the good condition, meaning we've got enough samples
            {
              bitCollected = true; //Indicate we've got a bit and to stop loop
              dutyLength = dutyLength - 1; //Subtract 1 due to a rollover issue
              timecode[count] = bitDecider(dutyLength, signalLength); //Decides bit
              signalLength = 0; //Reset variables
              dutyLength = 1;
              Serial.println("Bit found to be " + String(timecode[count])); //Uncomment for testing purposes
              pastVal = currentVal; //Sets cV to pV for next go around
            }  
          }
          else
          {
            pastVal = currentVal; //Sets cV to pV to keep collecting
          }
        }
      }
      //Once the for loop is left, all data should have been collected
      syncing = false;
    }
    else
    {
      //Serial.println("Search for signal start beginning");
      while(!signalStartFound) //This loop is looking for the start of the signal, or the "xx" or "22"
      {
        while(!bitStartFound) //This loop is looking for the start of the first bit, so we know when to start collecting data
        {
          currentVal = digitalRead(signalPin); //Read in a 0 or 1
          zeroes = zeroes + !digitalRead(signalPin);
          if(currentVal == 1 && pastVal == 0) //If transitioning from a 0 to a 1
          {
            if(zeroes > 10000)
            {
              bitStartFound = true; //We've found the bit
              pastVal = 1; //Set pastVal to 1
            }
            else
            {
              pastVal = 1;
            }
          }
          else
          {
            pastVal = currentVal; //Set pastVal to the current value and continue looking for the 0->1
          }
          
        }
        
        //Now we know we've found the start of a bit and won't be in the middle of a signal when we start counting
        //Serial.println("Beginning search for xx");
        currentVal = digitalRead(signalPin); //Read in the current 0 or 1
        //Serial.println("Currently Reading: " + String(currentVal));
        dutyLength = dutyLength + currentVal; //This is the duty cycle of the PWM, essentially we add 1 if high
        signalLength = signalLength + 1; //This is keeping track of the period, always add 1
        if(currentVal == 1 && pastVal == 0) //If we have ended our signal and are going to the next
        {
          if(signalLength < 100000)
          {
            //This is a case where the transition is invalid, and we haven't seen enough samples
            dutyLength = signalLength; //Every sample should've been a 1 at this point
            pastVal = 1;
          }
          else
          {  
            //Serial.println("Entire bit found, sending to bit decider");
            pastBit = presentBit; //Move the previously collected bit to the past bit location
            presentBit = bitDecider(dutyLength, signalLength); //Go determine what bit we were working with
            Serial.println("Bit found to be " + String(presentBit));
            //Serial.println("Present bit: " + String(presentBit) + "Past Bit: " + String(pastBit));
            if(pastBit == 2 && presentBit == 2) //If both the current bit and the previous bit are 2 or "x"
            {
              //Serial.println("Double x found");
              signalStartFound = true; //Denote we found our signal
              currentVal = 1; //Reset variables for future sync or counts
              dutyLength = 0;
              signalLength = 0;
              pastVal = 1;
              timecode[0] = presentBit; //Set this buffer value as the first in our timecode
            }
            else
            {
              dutyLength = 0; //If we didn't get our double x reset variables and wait for the next bit
              signalLength = 0;
              pastVal = currentVal;
            }
          }
        }
        else
        {
          pastVal = currentVal;
        }
      }
    }
  }
}

/*
 Function bitDecider
 int dutyCycle: how many 1's we sampled
 int period: how many samples we took
 returns: value of the bit, either a 0, 1, or buffer (2)
*/
int bitDecider(unsigned long dutyCycle, unsigned long period)
{
  //Serial.println("Entered bit decider");
  signalSize = (float) dutyCycle / period;
  Serial.println("Duty Cycle: " + String(dutyCycle));
  Serial.println("Signal Length: " + String(period));
  Serial.println("Signal Size determined to be: " + String(signalSize));
  if(signalSize > 0.65)
  {
    return 2; //2 if real signal, 0 if dummy (before fix)
  }
  else if(signalSize < 0.32)
  {
    return 0; //0 if real signal, 2 if dummyy
  }
  else
  {
    return 1;
  }
}

//======================================================================
//=====================Decoding Methods=================================
//======================================================================
//Wipes the decoding variables to default states
void variableWipe() {
  newminute = 0;
  newhour = 0;
  newday = 0;
  newyear = 0;
  DUTsign = 0;
  DUTvalue = 0.0;
  LeapYear = false;
  LeapSecond = false;
  DSTstatus = 0;
}

//Breaks up the 60 bit Integer Array which holds the signal into smaller segments for decoding
void breakArrays() {
  for (int i = 0; i < 8; i++) { //Loops '8' times and copies the bits from the 'timecode' array into its corresponding smaller segment
    newminutes[i] = timecode[i + 1];
  }
  for (int i = 0; i < 7; i++) { //Loops '7' times and copies the bits from the 'timecode' array into its corresponding smaller segment
    newhours[i] = timecode[i + 12];
  }
  for (int i = 0; i < 12; i++) { //Loops '12' times and copies the bits from the 'timecode' array into its corresponding smaller segment
    newdays[i] = timecode[i + 22];
  }
  for (int i = 0; i < 3; i++) { //Loops '3' times and copies the bits from the 'timecode' array into its corresponding smaller segment
    DUTsigns[i] = timecode[i + 36];
  }
  for (int i = 0; i < 4; i++) { //Loops '4' times and copies the bits from the 'timecode' array into its corresponding smaller segment
    DUTvalues[i] = timecode[i + 40];
  }
  for (int i = 0; i < 9; i++) { //Loops '9' times and copies the bits from the 'timecode' array into its corresponding smaller segment
    newyears[i] = timecode[i + 45];
  }
  for (int i = 0; i < 4; i++) { //Loops '4' times and copies the bits from the 'timecode' array into its corresponding smaller segment
    LeapsAndDST[i] = timecode[i + 55];
  }
}

//Compiles bits found in the signal array related to the "newminute" value
void getMinutes() {
  if (newminutes[0] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newminute = newminute + 40;
  }
  if (newminutes[1] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newminute = newminute + 20;
  }
  if (newminutes[2] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newminute = newminute + 10;
  }
  if (newminutes[4] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newminute = newminute + 8;
  }
  if (newminutes[5] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newminute = newminute + 4;
  }
  if (newminutes[6] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newminute = newminute + 2;
  }
  if (newminutes[7] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newminute = newminute + 1;
  }
}

//Compiles bits found in the signal array related to the "newhour" value
void getHours() {
  if (newhours[0] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newhour = newhour + 20;
  }
  if (newhours[1] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newhour = newhour + 10;
  }
  if (newhours[3] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newhour = newhour + 8;
  }
  if (newhours[4] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newhour = newhour + 4;
  }
  if (newhours[5] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newhour = newhour + 2;
  }
  if (newhours[6] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newhour = newhour + 1;
  }
}

//Compiles bits found in the signal array related to the "newday" value
void getDays() {
  if (newdays[0] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newday = newday + 200;
  }
  if (newdays[1] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newday = newday + 100;
  }
  if (newdays[3] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newday = newday + 80;
  }
  if (newdays[4] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newday = newday + 40;
  }
  if (newdays[5] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newday = newday + 20;
  }
  if (newdays[6] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newday = newday + 10;
  }
  if (newdays[8] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newday = newday + 8;
  }
  if (newdays[9] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newday = newday + 4;
  }
  if (newdays[10] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newday = newday + 2;
  }
  if (newdays[11] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newday = newday + 1;
  }
}

//Compiles bits found in the signal array related to the "DUTsign" value
void getDUTSign() {
  if (DUTsigns[0] == 1 && DUTsigns[2] == 1) { //Determines if "DUTsign" should be a 1
    DUTsign = 1;
  }
  if (DUTsigns[1] == 1) {
    DUTsign = 0;
  }
}

//Compiles bits found in the signal array related to the "DUTvalue" value
void getDUTValue() {
  if (DUTvalues[0] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    DUTvalue = DUTvalue + 0.8;
  }
  if (DUTvalues[1] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    DUTvalue = DUTvalue + 0.4;
  }
  if (DUTvalues[2] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    DUTvalue = DUTvalue + 0.2;
  }
  if (DUTvalues[3] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    DUTvalue = DUTvalue + 0.1;
  }
}

//Compiles bits found in the signal array related to the "newyear" value
void getYears() {
  if (newyears[0] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newyear = newyear + 80;
  }
  if (newyears[1] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newyear = newyear + 40;
  }
  if (newyears[2] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newyear = newyear + 20;
  }
  if (newyears[3] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newyear = newyear + 10;
  }
  if (newyears[5] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newyear = newyear + 8;
  }
  if (newyears[6] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newyear = newyear + 4;
  }
  if (newyears[7] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newyear = newyear + 2;
  }
  if (newyears[8] == 1) { //Checks location in the array for a binary '1' value, adds its corresponding weight to the variable if found
    newyear = newyear + 1;
  }
}

//Compiles bits found in the signal array related to the "Leap___" and "DSTstatus" values
void getLeapsAndDST ()  {
  if (LeapsAndDST[0] == 1) { //Checks location in the array for a binary '1' value, sets "LeapYear" to 'true' if found
    LeapYear = true;
  }
  if (LeapsAndDST[1] == 1) { //Checks location in the array for a binary '1' value, sets "LeapSecond" to 'true' if found
    LeapSecond = true;
  }
  if (LeapsAndDST[2] == 0 && LeapsAndDST[3] == 0) { //Checks '2' locations for their binary values, based on the recieved value assigns an integer value to "DSTstatus"
    DSTstatus = 0;
  }
  if (LeapsAndDST[2] == 0 && LeapsAndDST[3] == 1) { //Checks '2' locations for their binary values, based on the recieved value assigns an integer value to "DSTstatus"
    DSTstatus = 1;
  }
  if (LeapsAndDST[2] == 1 && LeapsAndDST[3] == 0) { //Checks '2' locations for their binary values, based on the recieved value assigns an integer value to "DSTstatus"
    DSTstatus = 2;
  }
  if (LeapsAndDST[2] == 1 && LeapsAndDST[3] == 1) { //Checks '2' locations for their binary values, based on the recieved value assigns an integer value to "DSTstatus"
    DSTstatus = 3;
  }
}
//------------------END OF DECODING--------------------//

//======================================================================
//=====================Timekeeping Methods==============================
//======================================================================

//Takes new decoded values found during syncing and assigns them to the old timekeeping/display values
void newdata() {
  min = newminute + 1;

  if (min >= 60) {
    min = 0;
  }

  hourAPM = newhour - timezone; //Assigns "newhour" value to "hourAPM" and subtracts the current timezone 
  hourNATO = newhour - timezone; //Assigns "newhour" value to "hourNATO" and subtracts the current timezone
  year = newyear;

  if (DSTstatus == 3) {
    hourAPM = hourAPM + 1;
    hourNATO = hourNATO + 1;
  }
  
  sec = 1; //************* THIS IS SUPPOSED TO BE A '0' UNLESS TESTING *****************//
  
  if (hourNATO < 0) { //Checks if "hourNATO" is less than '0' after being assigned to the adjusted "newhour" value
    flag = 1; //Assigns "flag" value to '1' since the time must be PM
    trigger = 1; //Assigns "trigger" value to '1' since the trigger will occur soon when the day changes
    roll = 1; //Assigns "roll" value to '1' since the rollover from PM-AM will occur soon
    hourAPM = 12 + hourAPM; //Adds the currently negative "hourAPM" value to '12' to find the current time
    hourNATO = 24 + hourNATO; //Adds the currently negative "hourNATO" value to '12' to find the current time
    if(newday == 1) { //Checks if the sync day value is a '1', which means the first day of the year
      year = year - 1; //Need to check since going back a day would mean going back a year too
      if (LeapYear == true) { //If it is a LeapYear than there are 366 days in a year 
        newday = 366;
      }
      else { //If it is not a LeapYear than there are 365 days in a year 
        newday = 365;
      }
    }
    else { //Subtracts 1 from the "newday" sync value if it is not a '1'
      newday = newday - 1;
    }
  }
  else if (hourNATO == 0) { //Checks if "hourNATO" is equal to '0'
    flag = 0; //Assigns "flag" value to '0' since the time must be AM
    roll = 1; //Assigns the "roll" value to '1' since the hour counter has not rolled over yet from '12' to '1'
    trigger = 0; //Assigns the "trigger" value to '0' since it is the same day as recieved by sync
    hourAPM = 12;
  }
  else if (hourNATO > 0 && hourNATO < 12) { //Checks if "hourNATO" is less than '12' and greater than '0'
    flag = 0; //Assigns "flag" value to '0' since the time must be AM
    roll = 0; //Assigns the "roll" value to '0' since the hour counter has rolled over and is greater than '0'
    trigger = 0; //Assigns the "trigger" value to '0' since it is the same day as recieved by sync
  }
  else if (hourNATO == 12) { //Checks if "hourNATO" is equal to '12'
    flag = 1; //Assigns "flag" value to '1' since the time must be PM
    roll = 0; //Assigns the "roll" value to '0' since the hour counter has rolled over and is greater than '0'
    trigger = 0; //Assigns the "trigger" value to '0' since it is the same day as recieved by sync
  }
  else if (hourNATO > 12) {
    hourAPM = hourAPM - 12;
    flag = 1; //Assigns "flag" value to '1' since the time must be PM
    roll = 1; //Assigns the "roll" value to '1' since the hour counter will soon be reaching midnight and must act as a flag to keep '12' from rolling over to '1'
    trigger = 1; //Assigns the "trigger" value to '1' since a new day will be occuring when rollover happens
  }
  exactday(); //Verifies the current day and month based on the "newday" value
  if (DUTsign == 0) { //Checks if "DUTsign" is equal to zero (negative) so a delay can be issued
    delay(DUTvalue * 1000);
  }
  else { //Adds the "DUTvalue" to the "sec" counter
    sec = sec + DUTvalue;
  }
  syncing = false; //Sets "syncing" value to false so no future updates will occur and time keeping can resume
}

//Finds the exact day of the year and breaks into "day" and "mon" variables
volatile void exactday() {
  if (LeapYear == true) { //Checks if it is currently a leapyear which would hold 366 days
    if (newday >= 1 && newday <= 31) { //Finds the current month and date based on the "newday" value: January has 31 days
      day = newday;
      mon = 1;
    }
    if (newday >= 32 && newday <= 60) { //Finds the current month and date based on the "newday" value: February has 29 days
      day = newday - 31;
      mon = 2;
    }
    if (newday >= 61 && newday <= 91) { //Finds the current month and date based on the "newday" value : March has 31 days
      day = newday - 60;
      mon = 3;
    }
    if (newday >= 92 && newday <= 121) { //Finds the current month and date based on the "newday" value: April has 30 days
      day = newday - 91;
      mon = 4;
    }
    if (newday >= 122 && newday <= 152) { //Finds the current month and date based on the "newday" value: May has 31 days
      day = newday - 121;
      mon = 5;
    }
    if (newday >= 153 && newday <= 182) { //Finds the current month and date based on the "newday" value: June has 30 days
      day = newday - 152;
      mon = 6;
    }
    if (newday >= 183 && newday <= 213) { //Finds the current month and date based on the "newday" value: July has 31 days
      day = newday - 182;
      mon = 7;
    }
    if (newday >= 214 && newday <= 244) { //Finds the current month and date based on the "newday" value: August has 31 days
      day = newday - 213;
      mon = 8;
    }
    if (newday >= 245 && newday <= 274) { //Finds the current month and date based on the "newday" value: September has 30 days
      day = newday - 244;
      mon = 9;
    }
    if (newday >= 275 && newday <= 305) { //Finds the current month and date based on the "newday" value: October has 31 days
      day = newday - 274;
      mon = 10;
    }
    if (newday >= 306 && newday <= 335) { //Finds the current month and date based on the "newday" value: November has 30 days
      day = newday - 305;
      mon = 11;
    }
    if (newday >= 336 && newday <= 366) { //Finds the current month and date based on the "newday" value: December has 31 days
      day = newday - 335;
      mon = 12;
    }
  }
  else {
    if (newday >= 1 && newday <= 31) { //Finds the current month and date based on the "newday" value: January has 31 days
      day = newday;
      mon = 1;
    }
    if (newday >= 32 && newday <= 59) { //Finds the current month and date based on the "newday" value: February has 28 days
      day = newday - 31;
      mon = 2;
    }
    if (newday >= 60 && newday <= 90) { //Finds the current month and date based on the "newday" value: March has 31 days
      day = newday - 59;
      mon = 3;
    }
    if (newday >= 91 && newday <= 120) { //Finds the current month and date based on the "newday" value: April has 30 days
      day = newday - 90;
      mon = 4;
    }
    if (newday >= 121 && newday <= 151) { //Finds the current month and date based on the "newday" value: May has 31 days
      day = newday - 120;
      mon = 5;
    }
    if (newday >= 152 && newday <= 181) { //Finds the current month and date based on the "newday" value: June has 30 days
      day = newday - 151;
      mon = 6;
    }
    if (newday >= 182 && newday <= 212) { //Finds the current month and date based on the "newday" value: July has 31 days
      day = newday - 181;
      mon = 7;
    }
    if (newday >= 213 && newday <= 243) { //Finds the current month and date based on the "newday" value: August has 31 days
      day = newday - 212;
      mon = 8;
    }
    if (newday >= 244 && newday <= 273) { //Finds the current month and date based on the "newday" value: September has 30 days
      day = newday - 243;
      mon = 9;
    }
    if (newday >= 274 && newday <= 304) { //Finds the current month and date based on the "newday" value: October has 31 days
      day = newday - 273;
      mon = 10;
    }
    if (newday >= 305 && newday <= 334) { //Finds the current month and date based on the "newday" value: November has 30 days
      day = newday - 304;
      mon = 11;
    }
    if (newday >= 335 && newday <= 365) { //Finds the current month and date based on the "newday" value: December has 31 days
      day = newday - 334;
      mon = 12;
    }
  }
}

//Increments the internal clock by '1' second and updates any domino values
void timeincrement() {
  sec = sec + 1; //Increment "sec" count by '1'

  //Manages seconds, minutes, day, and year updates
  if (sec == 60) { //Checks if "sec" has reached '60'
    sec = 0; //Resets "sec" counter to '0'
    min = min + 1; //Iterates "min" by '1'
  }
  if (min == 60) { //Chekcs if "min" has reached '60'
    min = 0; //Resets "min" counter to '0'
    hourAPM = hourAPM + 1; //Iterates "hourAPM" by '1'
    hourNATO = hourNATO + 1; //Iterates "hourNATO" by '1'
  }
  
  hourincrement(); //Completes hour calculations
  
  if ((mon == 1) && (day == 2)) { //Checks if the date is currently January 2nd
    if ((year%4) == 0) { //Checks if currently in a "LeapYear"
      LeapYear = true;
    }
    else {
      LeapYear = false;
    }
  }
  else if ((mon == 2) && (day == 29) && ((year%4) == 0)) { //Checks if the date is February 29th (ensures leap day occurs)
    day = 29;
    LeapYear = true; //Reaffirms "LeapYear" status
    newday = 60;
  }
  else if ((mon == 2) && (day > 28)) { //Checks if the date is higher than February 28th (only changes anything if previous 'else if' statement isn't active)
    day = 1;
    mon = 3;
  }
  else if ((mon == 4) | (mon == 6) | (mon == 9) | (mon == 11)) { //Checks if currently in a month with only 30 days
    if (day > 30) { //Checks if day is greater than 30
      day = 1;
      mon = mon + 1;
      if (LeapSecond == true) { //Checks for "LeapSecond" (only occurs during June or December)
      sec = sec + 1;
      }
    }
  }
  else if (day > 31) { //Checks if day is greater than 31 (used for any month with 31 days)
    day = 1;
    mon = mon + 1;
    if (mon > 12) { //Checks if 'exiting' the month of December
      mon = 1;
      newday = 1;
      year = year + 1;
      leapyearDummy = LeapYear; //Assigns "leapyearDummy" to previous "LeapYear" status
      if (LeapSecond == true) { //Checks for "LeapSecond" (only occurs during June or December)
        sec = sec + 1;
      }
    }
  }
  exactday(); //Verifies the current date
}

//Performs calculations for incrementing the "hourAPM" and "hourNATO" values correctly
void hourincrement() {
  if (roll == 0) { //Checks "roll" status (Determines when a new day will occur and acts as a flag to prevent AM/PM from switching at the wrong time)
    if (DSTstatus == 2) { //Checks the "DSTstatus" to see if Daylight Savings is starting
      if (hourAPM >= 2) { //Performs Daylight Savings jump at 2 AM
        hourAPM = hourAPM + 1;
        hourNATO = hourNATO + 1;
        DSTstatus = 3; //Sets "DSTstatus" to active (Currently in DST)
      }
    }
    if (DSTstatus == 1) { //Checks the "DSTstatus" to see if Daylight Savings is ending
      if (hourAPM >= 2) { //Performs Daylight Savings jump at 2 AM
        hourAPM = hourAPM - 1;
        hourNATO = hourNATO - 1;
        DSTstatus = 0; //Sets "DSTstatus" to inactive (Outside of DST)
      }
    }
    if (hourAPM >= 12) { //Checks if currently at 12 PM (Noon) and sets flag from AM to PM without rolling over [> sign in case sync occurs greater than 12]
      flag = 1;
    }
    if (hourAPM > 12) { //Checks if greater than 12 PM (Noon) and sets "hourAPM" to '1' while setting "roll" to '1' and "trigger" to '1'
      hourAPM = 1;
      roll = 1;
      trigger = 1;
    }
  }
  if (roll == 1) { //Checks "roll" status (Determines when a new day will occur and acts as a flag to prevent AM/PM from switching at the wrong time)
    if (hourAPM >= 12) { //Checks if currently at 12 AM (Midnight) and sets flag from PM to AM without rolling over [> sign in case sync occurs greater than 12]
      flag = 0;
      if (trigger == 1) { //"Trigger" gets flipped the first time this statement is ran indicating a "day" change has occured
        if (timeswitch == 0) { //Checks if currently reading AM/PM time to prevent performing an additional "day" change
          day = day + 1;
          newday = newday + 1;
        }
        trigger = 0; //"Trigger" set to '0' to prevent incrementing additional days
      }
    }
    if (hourAPM > 12) { //Checks if greater than 12 AM (Midnight) and sets "hourAPM" to '1' while setting "roll" to '0'
      hourAPM = 1;
      roll = 0;
    }
  }
  if (hourNATO > 23) { //Checks if "hourNATO" is greater than 23, indicating that a new "day" change has occured and rolls over to time '0'
    hourNATO = 0;
    if (timeswitch == 1) { //Checks if currently reading NATO time to prevent performing an additional "day" change
      day = day + 1;
      newday = newday + 1;
    }
  }
}

//Keeps track of which timezone currently in and performs changes to clock values when a timezone change occurs
void timezonechange() {
  unsigned long interruptTime = millis(); //Used to measure the starting time of the interrupt to prevent debounce issues
  if(interruptTime - lastInterruptTime > 1000) { //Checks the elapsed time since the button press before performing any operations
    timezone = timezone - 1; //Subtracts the timezone up front so the correct operation will be performed
    timezoneDummy = hourNATO; //Grabs the current time before operation to accurately determine hour changes when going backwards
    if (timezone == 7) { //Checks if switching into GMT-07
      hourAPM = hourAPM + 1;
      hourNATO = hourNATO + 1;
    }
    else if (timezone == 6) { //Checks if switching into GMT-06
      hourAPM = hourAPM + 1;
      hourNATO = hourNATO + 1;
    }
    else if (timezone == 5) { //Checks if switching into GMT-05
      hourAPM = hourAPM + 1;
      hourNATO = hourNATO + 1;
    }
    else if (timezone == 4) { //Checks if switching into GMT-08
      timezone = 8;
      hourAPM = hourAPM - 3;
      hourNATO = hourNATO - 3;
      if (timezoneDummy >= 0 && timezoneDummy < 3) { //Checks if the timezone switch was triggered during the hours of 0 to 3 AM
        flag = 1; //Assigns appropriate PM values
        trigger = 1; //Assigns appropriate PM values
        roll = 1; //Assigns appropriate PM values
        hourAPM = 12 + hourAPM; //By using the now negative "hourAPM" and adding to max "hourAPM" value, can accurately find new "hourAPM" time
        hourNATO = 24 + hourNATO; //By using the now negative "hourNATO" and adding to max "hourNATO" value, can accurately find new "hourNATO" time
        if(newday == 1) { //Checks if current day is January 1st
          year = year - 1;
          if (leapyearDummy == true) { //Checks if previous year was a leap year and assigns appropriate "newday" value representative of December 31st
            newday = 366;
          }
          else {
            newday = 365;
          }
        }
        else {
          newday = newday - 1; //If not January 1st, simply subtracts "newday" by '1'
        }
      }
      if (timezoneDummy >= 12 && timezoneDummy < 15) { //Checks if the timezone switch was triggered during the hours of 12 to 3 PM
        flag = 0; //Assigns appropriate AM values
        trigger = 0; //Assigns appropriate AM values
        roll = 0; //Assigns appropriate AM values
        hourAPM = 12 + hourAPM; //By using the now negative "hourAPM" and adding to max "hourAPM" value, can accurately find new "hourAPM" time
      }
      timezoneDummy = hourNATO; //Reaffirms timezoneDummy value
      exactday(); //Verifies current date based on changes possibly applied
    }
  }
  lastInterruptTime = interruptTime; //Updates "lastInterruptTime" to be used in comparison to prevent debounce
}

//Allows for manual setting of the clock time, month, date, and year
void manualSet() {
  manual = digitalRead(manualPin); //Reads manualPin input, necessary to update value when checking within loops
  while (manual == true){ //If the manual set button is still being pressed, a messsage will be sent to the user
    manual = digitalRead(manualPin); //Reads manualPin input, necessary to update value when checking within loops
    lcd.setCursor(0, 3); //Sets cursor to bottom row
    lcd.print("Release Button"); //Informs user to take finger off button to successfully advance to next stage
  }
  lcd.print("              "); //Clears bottom row of text
  count = 0; //Sets count to '0', necessary for keeping track of which stage currently in
  while (count != 6){ //Forces the code to lock inside of while loop until all 6 stages are cleared
    while (count == 0) { //Stage 0 locking mechanism
      Button1 = digitalRead(buttonOne); //Reads buttonOne input, necessary to update value when checking within loops
      Button2 = digitalRead(buttonTwo); //Reads buttonTwo input, necessary to update value when checking within loops
      manual = digitalRead(manualPin); //Reads manualPin input, necessary to update value when checking within loops
      if (Button1 == 1){  //If buttonOne pressed, a high signal is recieved which equates to '1'
        delay (350); //Delay to prevent from performing multiple operations per button press
        hourAPM = hourAPM + 1; //Adds 1 hour to hourAPM
        hourNATO = hourNATO + 1; //Adds 1 hour to hourNATO
        if (hourNATO >= 24) { //Checks if hourNATO has overflowed
          hourNATO = 0; //Reassigns hourNATO to '0'
          hourAPM = 12; //Reassigns hourAPM to '12'
        }
        if (hourAPM >= 13) { //Checks if hourAPM has crossed from 12PM to 1AM
          hourAPM = 1;
        }
      }
      if (Button2 == 1){ //If buttonTwo pressed, a high signal is recieved which equates to '1'
        delay (350); //Delay to prevent from performing multiple operations per button press
        hourAPM = hourAPM - 1; //Subtracts 1 hour from hourAPM
        hourNATO = hourNATO - 1; //Subtracts 1 hour from hourNATO
        if (hourAPM == 0) { //Checks if hourAPM has rolled back to '0'
          hourAPM = 12; //Assigns hourAPM to '12'
        }
        if (hourNATO <= -1) { //Checks if hourNATO has rolled under '0'
          hourNATO = 23; //Assigns hourNATO to '23'
          hourAPM = 11; //Assigns hourAPM to '11'
        }
      }
      if (manual == true){ //If manualPin pressed, a high signal is received which sets the button to 'true'
        delay (350); //Delay to prevent from performing multiple operations per button press
        count = count + 1; //Iterates count by '1' moving to the next stage
      }
      lcd.setCursor(0,0); //Sets LCD print cursor
      if (timeswitch == 0) { //Determines which style of time to display
        if (hourAPM < 10) { //Checks if "hourAPM" is less than 10 and needs a 0 placed in front
          lcd.print("0"); //Ensures the "hoursAPM" is always written in a format of 2 digits
        } 
        lcd.print(hourAPM); 
        lcd.print(":");
      }
      else {
        if (hourNATO < 10) { //Checks if "hourNATO" is less than 10 and needs a 0 placed in front
          lcd.print("0"); //Ensures the "hoursNATO" is always written in a format of 2 digits
        }
        lcd.print(hourNATO);
        lcd.print(":");
      }

      if (min < 10) { //Checks if "min" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "min" is always written in a format of 2 digits
      }
      lcd.print(min);
      lcd.print(":");

      if (sec < 10) { //Checks if "sec" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "sec" is always written in a format of 2 digits
      }
      lcd.print(sec);
      lcd.print(" ");

      if (timeswitch == 0) { //Determines if the text 'AM' or 'PM' needs to be printed
        if (flag==0) lcd.print("AM ");
        if (flag==1) lcd.print("PM ");
      }
      lcd.print("@ GMT-0");
      lcd.print(timezone); //Prints current timezone relative to UTC signal

      if (timeswitch == 1) { //Determins if the text last 3 print locations need to be cleared
        lcd.print("   "); //Clears the last 3 print locations of row 1 on the LCD 
      }

      lcd.setCursor(0,1); //Sets LCD print cursor
      if (mon < 10) { //Checks if "mon" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "mon" is always written in a format of 2 digits
      }
      lcd.print(mon);
      lcd.print("/");

      if (day < 10) { //Checks if "day" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "day" is always written in a format of 2 digits
      }
      lcd.print(day);
      lcd.print("/");
      lcd.print(year);
      lcd.print(" "); //Prevents any writing errors with the "year" value and keeps the space blank

      
      lcd.setCursor(0,3); //Sets cursor to bottom row
      lcd.print("Changing 'Hour'  "); //Prints text to user informing them of which change is being performed
    }
    while (count == 1) { //Stage 1 of locking mechanism
      Button1 = digitalRead(buttonOne); //Reads buttonOne input, necessary to update value when checking within loops
      Button2 = digitalRead(buttonTwo); //Reads buttonTwo input, necessary to update value when checking within loops
      manual = digitalRead(manualPin); //Reads manualPin input, necessary to update value when checking within loops
      if (Button1 == 1){ //If buttonOne is pressed, a high signal is received which equates to a '1'
        delay (350);
        min = min + 1;
        if (min >= 60) { //If 'min' has rolled over, it is reassigned to '0'
          min = 0;
        }
      }
      if (Button2 == 1){ //If buttonTwo is pressed, a high signal is recieved which equates to a '1'
        delay (350);
        min = min - 1;
        if (min <= -1) { //If 'min' has rolled under, it is reassigned to '59'
          min = 59;
        }
      }
      if (manual == true){ //If manualPin is pressed, a high signal is recieved which equates to 'true'
        delay (350);
        count = count + 1;
      }
      lcd.setCursor(0,0); //Sets LCD print cursor
      if (timeswitch == 0) { //Determines which style of time to display
        if (hourAPM < 10) { //Checks if "hourAPM" is less than 10 and needs a 0 placed in front
          lcd.print("0"); //Ensures the "hoursAPM" is always written in a format of 2 digits
        } 
        lcd.print(hourAPM); 
        lcd.print(":");
      }
      else {
        if (hourNATO < 10) { //Checks if "hourNATO" is less than 10 and needs a 0 placed in front
          lcd.print("0"); //Ensures the "hoursNATO" is always written in a format of 2 digits
        }
        lcd.print(hourNATO);
        lcd.print(":");
      }

      if (min < 10) { //Checks if "min" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "min" is always written in a format of 2 digits
      }
      lcd.print(min);
      lcd.print(":");

      if (sec < 10) { //Checks if "sec" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "sec" is always written in a format of 2 digits
      }
      lcd.print(sec);
      lcd.print(" ");

      if (timeswitch == 0) { //Determines if the text 'AM' or 'PM' needs to be printed
        if (flag==0) lcd.print("AM ");
        if (flag==1) lcd.print("PM ");
      }
      lcd.print("@ GMT-0");
      lcd.print(timezone); //Prints current timezone relative to UTC signal

      if (timeswitch == 1) { //Determins if the text last 3 print locations need to be cleared
        lcd.print("   "); //Clears the last 3 print locations of row 1 on the LCD 
      }

      lcd.setCursor(0,1); //Sets LCD print cursor
      if (mon < 10) { //Checks if "mon" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "mon" is always written in a format of 2 digits
      }
      lcd.print(mon);
      lcd.print("/");

      if (day < 10) { //Checks if "day" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "day" is always written in a format of 2 digits
      }
      lcd.print(day);
      lcd.print("/");
      lcd.print(year);
      lcd.print(" "); //Prevents any writing errors with the "year" value and keeps the space blank

      
      lcd.setCursor(0,3); //Sets cursor to bottom row
      lcd.print("Changing 'Minute'  "); //Prints text to user informing them of which change is being performed
    }
    while (count == 2) {
      Button1 = digitalRead(buttonOne); //Reads buttonOne input, necessary to update value when checking within loops
      Button2 = digitalRead(buttonTwo); //Reads buttonTwo input, necessary to update value when checking within loops
      manual = digitalRead(manualPin); //Reads manualPin input, necessary to update value when checking within loops
      if (Button1 == 1){ //If buttonOne is pressed, a high signal is received which equates to a '1'
        delay (350);
        sec = sec + 1;
        if (sec >= 60) { //Checks if 'sec' has rolled over 60, in which case it is reassigned to '0'
          sec = 0;
        }
      }
      if (Button2 == 1){ //If buttonTwo is pressed, a high signal is recieved which equates to '1'
        delay (350);
        sec = sec - 1;
        if (sec <= -1) { //Checks if 'sec' has rolled under, in which case it is reassigned to '59'
          sec = 59;
        }
      }
      if (manual == true){ //If manualPin is pressed, a high signal is receieved which equates to 'true'
        delay (350);
        count = count + 1;
      }
      lcd.setCursor(0,0); //Sets LCD print cursor
      if (timeswitch == 0) { //Determines which style of time to display
        if (hourAPM < 10) { //Checks if "hourAPM" is less than 10 and needs a 0 placed in front
          lcd.print("0"); //Ensures the "hoursAPM" is always written in a format of 2 digits
        } 
        lcd.print(hourAPM); 
        lcd.print(":");
      }
      else {
        if (hourNATO < 10) { //Checks if "hourNATO" is less than 10 and needs a 0 placed in front
          lcd.print("0"); //Ensures the "hoursNATO" is always written in a format of 2 digits
        }
        lcd.print(hourNATO);
        lcd.print(":");
      }

      if (min < 10) { //Checks if "min" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "min" is always written in a format of 2 digits
      }
      lcd.print(min);
      lcd.print(":");

      if (sec < 10) { //Checks if "sec" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "sec" is always written in a format of 2 digits
      }
      lcd.print(sec);
      lcd.print(" ");

      if (timeswitch == 0) { //Determines if the text 'AM' or 'PM' needs to be printed
        if (flag==0) lcd.print("AM ");
        if (flag==1) lcd.print("PM ");
      }
      lcd.print("@ GMT-0");
      lcd.print(timezone); //Prints current timezone relative to UTC signal

      if (timeswitch == 1) { //Determins if the text last 3 print locations need to be cleared
        lcd.print("   "); //Clears the last 3 print locations of row 1 on the LCD 
      }

      lcd.setCursor(0,1); //Sets LCD print cursor
      if (mon < 10) { //Checks if "mon" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "mon" is always written in a format of 2 digits
      }
      lcd.print(mon);
      lcd.print("/");

      if (day < 10) { //Checks if "day" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "day" is always written in a format of 2 digits
      }
      lcd.print(day);
      lcd.print("/");
      lcd.print(year);
      lcd.print(" "); //Prevents any writing errors with the "year" value and keeps the space blank

      
      lcd.setCursor(0,3); //Sets cursor to bottom row
      lcd.print("Changing 'Second'  "); //Prints text to inform user of which change is being performed
    }
    while (count == 3) {
      Button1 = digitalRead(buttonOne); //Reads buttonOne input, necessary to update value when checking within loops
      Button2 = digitalRead(buttonTwo); //Reads buttonTwo input, necessary to update value when checking within loops
      manual = digitalRead(manualPin); //Reads manualPin input, necessary to update value when checking within loops
      if (Button1 == 1){ //If buttonOne is pressed, a high signal is received which equates to a '1'
        delay (350);
        mon = mon + 1;
        if (mon >= 13) { //Checks if 'mon' has rolled over, in which case it is reassigned to '1'
          mon = 1;
        }
      }
      if (Button2 == 1){ //If buttonTwo is pressed, a high signal is recieved which equates to a '1'
        delay (350);
        mon = mon - 1;
        if (mon <= 0) { //Checks if 'mon' has rolled under, in which case it is reassigned to '12'
          mon = 12;
        }
      }
      if (manual == true){ //If manualPin is pressed, a high signal is received which equates to 'true'
        delay (350);
        count = count + 1;
      }
      lcd.setCursor(0,0); //Sets LCD print cursor
      if (timeswitch == 0) { //Determines which style of time to display
        if (hourAPM < 10) { //Checks if "hourAPM" is less than 10 and needs a 0 placed in front
          lcd.print("0"); //Ensures the "hoursAPM" is always written in a format of 2 digits
        } 
        lcd.print(hourAPM); 
        lcd.print(":");
      }
      else {
        if (hourNATO < 10) { //Checks if "hourNATO" is less than 10 and needs a 0 placed in front
          lcd.print("0"); //Ensures the "hoursNATO" is always written in a format of 2 digits
        }
        lcd.print(hourNATO);
        lcd.print(":");
      }

      if (min < 10) { //Checks if "min" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "min" is always written in a format of 2 digits
      }
      lcd.print(min);
      lcd.print(":");

      if (sec < 10) { //Checks if "sec" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "sec" is always written in a format of 2 digits
      }
      lcd.print(sec);
      lcd.print(" ");

      if (timeswitch == 0) { //Determines if the text 'AM' or 'PM' needs to be printed
        if (flag==0) lcd.print("AM ");
        if (flag==1) lcd.print("PM ");
      }
      lcd.print("@ GMT-0");
      lcd.print(timezone); //Prints current timezone relative to UTC signal

      if (timeswitch == 1) { //Determins if the text last 3 print locations need to be cleared
        lcd.print("   "); //Clears the last 3 print locations of row 1 on the LCD 
      }

      lcd.setCursor(0,1); //Sets LCD print cursor
      if (mon < 10) { //Checks if "mon" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "mon" is always written in a format of 2 digits
      }
      lcd.print(mon);
      lcd.print("/");

      if (day < 10) { //Checks if "day" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "day" is always written in a format of 2 digits
      }
      lcd.print(day);
      lcd.print("/");
      lcd.print(year);
      lcd.print(" "); //Prevents any writing errors with the "year" value and keeps the space blank

      
      lcd.setCursor(0,3); //Sets cursor to bottom row
      lcd.print("Changing 'Month'    "); //Prints text to inform user of which change is being performed
    }
    while (count == 4) {
      Button1 = digitalRead(buttonOne); //Reads buttonOne input, necessary to update value when checking within loops
      Button2 = digitalRead(buttonTwo); //Reads buttonTwo input, necessary to update value when checking within loops
      manual = digitalRead(manualPin); //Reads manualPin input, necessary to update value when checking within loops
      if (Button1 == 1){ //If buttonOne is pressed, a high signal is received which equates to a '1'
        delay (350);
        day = day + 1;
        
        //Using '32' for the check because this day is being assigned by the user, it is not necessary to check which month currently in
        //when assigning the day value as it is up to the user to assign the correct date. If the wrong date is chosen it will roll over automatically to next month.
        if (day >= 32) { //Checks if 'day' has rolled over, in which case it is reassigned to '1'
          day = 1;
        }
      }
      if (Button2 == 1){ //If buttonTwo is pressed, a high signal is recieved which equates to a '1'
        delay (350);
        day = day - 1;
        if (day <= 0) { //Checks if 'day' has rolled under, in which case it is reassigned to '31'
          day = 31;
        }
      }
      if (manual == true){ //If manualPin is pressed, a high signal is recieved which equates to 'true'
        delay (350);
        count = count + 1;
      }
      lcd.setCursor(0,0); //Sets LCD print cursor
      if (timeswitch == 0) { //Determines which style of time to display
        if (hourAPM < 10) { //Checks if "hourAPM" is less than 10 and needs a 0 placed in front
          lcd.print("0"); //Ensures the "hoursAPM" is always written in a format of 2 digits
        } 
        lcd.print(hourAPM); 
        lcd.print(":");
      }
      else {
        if (hourNATO < 10) { //Checks if "hourNATO" is less than 10 and needs a 0 placed in front
          lcd.print("0"); //Ensures the "hoursNATO" is always written in a format of 2 digits
        }
        lcd.print(hourNATO);
        lcd.print(":");
      }

      if (min < 10) { //Checks if "min" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "min" is always written in a format of 2 digits
      }
      lcd.print(min);
      lcd.print(":");

      if (sec < 10) { //Checks if "sec" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "sec" is always written in a format of 2 digits
      }
      lcd.print(sec);
      lcd.print(" ");

      if (timeswitch == 0) { //Determines if the text 'AM' or 'PM' needs to be printed
        if (flag==0) lcd.print("AM ");
        if (flag==1) lcd.print("PM ");
      }
      lcd.print("@ GMT-0");
      lcd.print(timezone); //Prints current timezone relative to UTC signal

      if (timeswitch == 1) { //Determins if the text last 3 print locations need to be cleared
        lcd.print("   "); //Clears the last 3 print locations of row 1 on the LCD 
      }

      lcd.setCursor(0,1); //Sets LCD print cursor
      if (mon < 10) { //Checks if "mon" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "mon" is always written in a format of 2 digits
      }
      lcd.print(mon);
      lcd.print("/");

      if (day < 10) { //Checks if "day" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "day" is always written in a format of 2 digits
      }
      lcd.print(day);
      lcd.print("/");
      lcd.print(year);
      lcd.print(" "); //Prevents any writing errors with the "year" value and keeps the space blank

      
      lcd.setCursor(0,3); //Sets cursor to bottom row
      lcd.print("Changing 'Day'      "); //Prints text to the user informing them of which change is being performed
    }
    while (count == 5) {
      Button1 = digitalRead(buttonOne); //Reads buttonOne input, necessary to update value when checking within loops
      Button2 = digitalRead(buttonTwo); //Reads buttonTwo input, necessary to update value when checking within loops
      manual = digitalRead(manualPin); //Reads manualPin input, necessary to update value when checking within loops
      if (Button1 == 1){ //If buttonOne is pressed, a high signal is received which equates to a '1'
        delay (350);
        year = year + 1;
        if (year > 100) { //Checks if 'year' has rolled over, in which case it is reassigned to '1'
          year = 1;
        }
      }
      if (Button2 == 1){ //If buttonTwo is pressed, a high signal is recieved which equates to a '1'
        delay (350);
        year = year - 1;
        if (year < 0) { //Checks if 'year' has rolled under, in which case it is reassigned to '1'
          year = 99;
        }
      }
      if (manual == true){ //If manualPin is pressed, a high signal is recieved which equates to a 'true'
        delay (350);
        count = count + 1;
      }
      lcd.setCursor(0,0); //Sets LCD print cursor
      if (timeswitch == 0) { //Determines which style of time to display
        if (hourAPM < 10) { //Checks if "hourAPM" is less than 10 and needs a 0 placed in front
          lcd.print("0"); //Ensures the "hoursAPM" is always written in a format of 2 digits
        } 
        lcd.print(hourAPM); 
        lcd.print(":");
      }
      else {
        if (hourNATO < 10) { //Checks if "hourNATO" is less than 10 and needs a 0 placed in front
          lcd.print("0"); //Ensures the "hoursNATO" is always written in a format of 2 digits
        }
        lcd.print(hourNATO);
        lcd.print(":");
      }

      if (min < 10) { //Checks if "min" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "min" is always written in a format of 2 digits
      }
      lcd.print(min);
      lcd.print(":");

      if (sec < 10) { //Checks if "sec" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "sec" is always written in a format of 2 digits
      }
      lcd.print(sec);
      lcd.print(" ");

      if (timeswitch == 0) { //Determines if the text 'AM' or 'PM' needs to be printed
        if (flag==0) lcd.print("AM ");
        if (flag==1) lcd.print("PM ");
      }
      lcd.print("@ GMT-0");
      lcd.print(timezone); //Prints current timezone relative to UTC signal

      if (timeswitch == 1) { //Determins if the text last 3 print locations need to be cleared
        lcd.print("   "); //Clears the last 3 print locations of row 1 on the LCD 
      }

      lcd.setCursor(0,1); //Sets LCD print cursor
      if (mon < 10) { //Checks if "mon" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "mon" is always written in a format of 2 digits
      }
      lcd.print(mon);
      lcd.print("/");

      if (day < 10) { //Checks if "day" is less than 10 and needs a 0 placed in front
        lcd.print("0"); //Ensures the "day" is always written in a format of 2 digits
      }
      lcd.print(day);
      lcd.print("/");
      lcd.print(year);
      lcd.print(" "); //Prevents any writing errors with the "year" value and keeps the space blank

      lcd.setCursor(0,3); //Sets cursor to bottom row
      lcd.print("Changing 'Year'    "); //Prints text to user informing them of which change is being performed
    }
  }
  lcd.clear(); //Clears lcd of text

  if (hourNATO == 0) { //Checks if "hourNATO" is equal to '0'
    flag = 0; //Assigns "flag" value to '0' since the time must be AM
    roll = 1; //Assigns the "roll" value to '1' since the hour counter has not rolled over yet from '12' to '1'
    trigger = 0; //Assigns the "trigger" value to '0' since it is the same day as recieved by sync
  }
  else if (hourNATO > 0 && hourNATO < 12) { //Checks if "hourNATO" is less than '12' and greater than '0'
    flag = 0; //Assigns "flag" value to '0' since the time must be AM
    roll = 0; //Assigns the "roll" value to '0' since the hour counter has rolled over and is greater than '0'
    trigger = 0; //Assigns the "trigger" value to '0' since it is the same day as recieved by sync
  }
  else if (hourNATO == 12) { //Checks if "hourNATO" is equal to '12'
    flag = 1; //Assigns "flag" value to '1' since the time must be PM
    roll = 0; //Assigns the "roll" value to '0' since the hour counter has rolled over and is greater than '0'
    trigger = 0; //Assigns the "trigger" value to '0' since it is the same day as recieved by sync
  }
  else if (hourNATO > 12) {
    flag = 1; //Assigns "flag" value to '1' since the time must be PM
    roll = 1; //Assigns the "roll" value to '1' since the hour counter will soon be reaching midnight and must act as a flag to keep '12' from rolling over to '1'
    trigger = 1; //Assigns the "trigger" value to '1' since a new day will be occuring when rollover happens
  }

  if ((year%4) == 0) { //Checks if currently in a leapyear, in which case 'LeapYear' assigned 'true'
    LeapYear == true;
  }
  else {
    LeapYear == false; //If not in leapyear, 'LeapYear' assigned 'false'
  }
  if (LeapYear == true) { //Checks if it is currently a leapyear which would hold 366 days
    if (mon == 1) { //Finds the current "newday" value based on the day and month: January has 31 days
      newday = day;
    }
    if (mon == 2) { //Finds the current "newday" value based on the day and month: February has 29 days
      newday = day + 31;
    }
    if (mon == 3) { //Finds the current "newday" value based on the day and month: March has 31 days
      newday = day + 60;
    }
    if (mon == 4) { //Finds the current "newday" value based on the day and month: April has 30 days
      newday = day + 91;
    }
    if (mon == 5) { //Finds the current "newday" value based on the day and month: May has 31 days
      newday = day + 121;
    }
    if (mon == 6) { //Finds the current "newday" value based on the day and month: June has 30 days
      newday = day + 152;
    }
    if (mon == 7) { //Finds the current "newday" value based on the day and month: July has 31 days
      newday = day + 182;
    }
    if (mon == 8) { //Finds the current "newday" value based on the day and month: August has 31 days
      newday = day + 213;
    }
    if (mon == 9) { //Finds the current "newday" value based on the day and month: September has 30 days
      newday = day + 244;
    }
    if (mon == 10) { //Finds the current "newday" value based on the day and month: October has 31 days
      newday = day + 274;
    }
    if (mon == 11) { //Finds the current "newday" value based on the day and month: November has 30 days
      newday = day + 305;
    }
    if (mon == 12) { //Finds the current "newday" value based on the day and month: December has 31 days
      newday = day + 335;
    }
  }
  else {
    if (mon == 1) { //Finds the current "newday" value based on the day and month: January has 31 days
      newday = day;
    }
    if (mon == 2) { //Finds the current "newday" value based on the day and month: February has 28 days
      newday = day + 31;
    }
    if (mon == 3) { //Finds the current "newday" value based on the day and month: March has 31 days
      newday = day + 59;
    }
    if (mon == 4) { //Finds the current "newday" value based on the day and month: April has 30 days
      newday = day + 90;
    }
    if (mon == 5) { //Finds the current "newday" value based on the day and month: May has 31 days
      newday = day + 120;
    }
    if (mon == 6) { //Finds the current "newday" value based on the day and month: June has 30 days
      newday = day + 151;
    }
    if (mon == 7) { //Finds the current "newday" value based on the day and month: July has 31 days
      newday = day + 181;
    }
    if (mon == 8) { //Finds the current "newday" value based on the day and month: August has 31 days
      newday = day + 212;
    }
    if (mon == 9) { //Finds the current "newday" value based on the day and month: September has 30 days
      newday = day + 243;
    }
    if (mon == 10) { //Finds the current "newday" value based on the day and month: October has 31 days
      newday = day + 273;
    }
    if (mon == 11) { //Finds the current "newday" value based on the day and month: November has 30 days
      newday = day + 304;
    }
    if (mon == 12) { //Finds the current "newday" value based on the day and month: December has 31 days
      newday = day + 334;
    }
  }
}
//----------------END OF TIME KEEPING------------------//
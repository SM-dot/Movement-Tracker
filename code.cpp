/*
  Team Name: Exer-Vibes!

  Team Members:
  Akyra Lee
  Nikki Chi
  Syona Mehra
  Jenny tran

  Date of Last Edit: 3/10/21

  Code Summary:
  Our team's code was written in accordance to the project's two sensors, the accelerometer and barometer, and to comply with BLYNK.
  Our overall goals were to calculate the number of steps a user takes, allow them to set a step goal, and use a vibration motor to
  indicate when a goal is met. This overall involved creating a step count algorithm and an algorithm to calculate the user's steady
  state values. We also wished to calculate the number of flights of stairs a user climbed/descended. Both the step and stair count was
  displayed on the app. Numerous buttons and other value displays were also created on our app for confirmation, debugging, and user
  engagement purposes.

*/

//libraries required for barometer/accelerometer data collection
#include <Wire.h>
#include <DFRobot_LIS2DH12.h>
#include "DFRobot_BMP388.h"
#include "DFRobot_BMP388_I2C.h"
#include "SPI.h"
#include "math.h"
#include "bmp3_defs.h"
#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleSerialBLE.h>

#define BLYNK_PRINT Serial
#define CALIBRATE_Altitude

DFRobot_BMP388_I2C bmp388; //for Barometer
DFRobot_LIS2DH12 LIS; //for Accelerometer

//Personal authorization token - used to connect to bluetooth
char auth[] = "9lIT_DhFXKmk6DKfTB8p_Qq8O58-vipR";

//**************************************ALL VARIABLES**************************************

//step count algorithm variables
int steps = 0; //keeps record of the total number of steps
bool Lets_Look_At_The_Forward_Swing = true; //assumes that the user moves the hand wearing the device forward when they take their first step
bool Lets_Look_At_The_Backward_Swing = false; //assumes that the user's hand is not moving backward when they take their first step

//steady state calibration variables
int max_steady_state_value; //declares a variable for the maximum steady state value
int min_steady_state_value; //declares a variable for the minimum steady state value
int array_that_holds_calibration_values[100]; //this array will hold the first 100 values gathered from the accelerometer
int array_size = 100; //size parameter for the array

//stair count variables
float seaLevel; //used by the barometer to evaluate altitude
float current_Altitude; //declares a variable for the user's altitude
float past_Altitude; //declares a variable for a comparative altitude
int staircases = 0; //indicates that the initial number of stairs climbed is zero
float height_change = 0; //indicates that the change in altitude is initially zero

//global variables for the accelerometer values
int16_t x, y, z;

//declares that the vibration motor is located in digital port 2
int VIBE = 2;

//declares a variable for the goal
int SWOL_GOAL;

//used to declare the existence of a button the user presses on the app to calibrate their steady state values, on is represented by 1 and off is represented by 0
int CALIBRATION_BUTTON;

//used to declare the existence of a button that turns on the vibration motor, on is represented by 1 and off is represented by 0
int VIBE_BUTTON;

//used to declare the existence of a button that the user presses on the app to acquire their initial altitude, on is represented by 1 and off is represented by 0
int ALTITUDE_BUTTON;

//used to declare the existence of a button that the user presses on the app to initiate KEANU MODE (a simple gif), on is represented by 1 and off is represented by 0
int KEANU_BUTTON;

//used to declare the existence of a button that the user presses on the app to flash a meme, on is represented by 1 and off is represented by 0
int SURPRISE_BUTTON;


//******************************************BLYNK APP BUTTONS************************************************************

//Step goal input will be in virtual port 3
BLYNK_WRITE(V3)
{
  SWOL_GOAL = param.asInt(); // assigns an incoming value from pin V3 to a variable
}

//button for calibrating initial altitude is in port V14
BLYNK_WRITE(V14)
{
  ALTITUDE_BUTTON = param.asInt();
}

//button for calibrating steady state values is in port V11
BLYNK_WRITE(V11)
{
  CALIBRATION_BUTTON = param.asInt();
}

//button that turns on the vibration motor for fun is in port V7
BLYNK_WRITE(V7)
{
  VIBE_BUTTON = param.asInt();
}

//button that initiates KEANU MODE is in port V13
BLYNK_WRITE(V13)
{
  KEANU_BUTTON = param.asInt();
}

//button that displays a surprise is in port V15
BLYNK_WRITE(V15)
{
  SURPRISE_BUTTON = param.asInt();
}


void setup() {
  Wire.begin(); //communicates with I2C bus line
  Serial.begin(115200); //baud rate

  pinMode(VIBE, OUTPUT); //declares the VIBRATION MOTOR to be output

  //prepares accelerometer
  while (!Serial);
  delay(100);
  while (LIS.init(LIS2DH12_RANGE_16GA) == -1) { //Equipment connection exception or I2C address error
    Serial.println("No I2C devices found");
    delay(1000);
  }

  //prepares Blynk
  Blynk.begin(Serial, auth);

  //prepares barometer
  bmp388.set_iic_addr(BMP3_I2C_ADDR_SEC);
  while (bmp388.begin()) {
    Serial.println("Initialize error!");
    delay(1000);
  }

  //used to prepare altitude data gathered by the barometer
  delay(100);
  seaLevel = bmp388.readSeaLevel(525.0);
  Serial.print("seaLevel : ");
  Serial.print(seaLevel);
  Serial.println(" Pa");
}

void loop() {

  Blynk.run(); //starts running Blynk

  //used to acquire altitude data from barometer
#ifdef CALIBRATE_Altitude
  /* Read the calibrated altitude */
  float altitude = bmp388.readCalibratedAltitude(seaLevel);
  //Serial.print("calibrate Altitude : ");
  //Serial.print(altitude);
  //Serial.println(" m");
#else
  /* Read the altitude */
  float altitude = bmp388.readAltitude();
  //Serial.print("Altitude : ");
  //Serial.print(altitude);
  //Serial.println(" m");
#endif
  delay(100);

  //inserts 100 acceleration y values into an array
  for (int i = 0; i < 100; i++)
  {
    LIS.readXYZ(x, y, z); //reads values from accelerometer
    array_that_holds_calibration_values[i] = y; //inserts the y values of the accelerometer data into the array
  }

  acceleration(); //calls the acceleration function

  //value displays on BLYNK
  Blynk.virtualWrite(V12, 1); //displays team logo on app
  Blynk.virtualWrite(V6, bmp388.readTemperature()); ////displays the current altitude to verify that the barometer is connected/working
  Blynk.virtualWrite(V8, altitude); //displays the current altitude to verify that the barometer is connected/working
  Blynk.virtualWrite(V10, max_steady_state_value); //displays the maximum steady state value to verify that is was collected
  Blynk.virtualWrite(V9, min_steady_state_value); //displays the minimum steady state value to verify that is was collected
  Blynk.virtualWrite(V1, steps); //displays the step count on the app

  //considers if the step goal is ever met
  if (steps == SWOL_GOAL)//if the number of steps ever equals the goal
  {
    digitalWrite(VIBE, HIGH); //turns the vibration motor on for a second
    delay(1000); //waits 1 second
    digitalWrite(VIBE, LOW); //turns the vibration motor off
    delay(500); //waits half a second
    Blynk.notify("SWOL GOAL MET YAYYYYYYYYY"); //sends the user a message that tells them they made their goal
  }

  //considers if the button that turns on the vibration motor is ever switched on
  //used to verify that the vibration motor is connected/working
  while (VIBE_BUTTON == 1) //1 = on, 0 = off
  {
    digitalWrite(VIBE, HIGH); //turns on the vibration motor
    Blynk.notify("WE'RE VIBING"); //spams the user with an obnoxious message
    Blynk.virtualWrite(V12, 2); //displays a new image, this time of the joker and peter parker dancing
  }

  //ensures that if the vibration motor button is switched off the vibration motor does not remain on
  if (VIBE_BUTTON == 0)
  {
    digitalWrite(VIBE, LOW);
  }

  //considers if the button used to calibrate the steady state values is pressed
  if (CALIBRATION_BUTTON == 1)
  {
    //calls the function that find the maximum calibration value
    int maximum_CALIBRATION_value = find_me_the_maximum_please(array_that_holds_calibration_values, 100);
    //calls the function that finds the minimum calibration value
    int minimum_CALIBRATION_value = find_me_the_minimum_please(array_that_holds_calibration_values, 100);

    //going to be inserted into the accelerometer step count algorithm
    max_steady_state_value = maximum_CALIBRATION_value;
    min_steady_state_value = minimum_CALIBRATION_value;

    Blynk.notify("CALIBRATION COMPLETE"); //sends a message that confirms to the user that their steady state values have been calibrated
  }

  //considers if the attitude button is ever pressed
  if (ALTITUDE_BUTTON == 1)
  {
    past_Altitude = altitude; //sets the past altitude equal to the first altitude reading acquired
    Blynk.notify("ALTITUDE INITIALIZED"); //sends a message that confirms to the user that their initial altitude has been calibrated
  }

  //considers if the keanu button on the app is ever switched on
  //used for fun/experimental purposes
  while (KEANU_BUTTON == 1)
  {
    //while the button is switched on a jif of the keanu meme will replace the logo image
    Blynk.virtualWrite(V12, 3); //displays tall keanu image
    delay(500); //waits half a second
    Blynk.virtualWrite(V12, 4); //displays smol keanu image
    delay(500); //waits half a second
    Blynk.notify("KEANU MODE INITIATED"); //spams user with obnoxious message
  }

  //considers if the surprise button is ever pressed
  if (SURPRISE_BUTTON == 1)
  {
    Blynk.virtualWrite(V12, 5); //flashes a surprise meme image
    Blynk.notify("YEAHHHHHHHHHHHHH"); //sends user obnoxious message
  }


  //********************************************************STAIRCLIMBING ALGORITHM**********************************************************
  current_Altitude = altitude; //sets the user's current altitude equal to the accelerometer value

  height_change = abs(current_Altitude - past_Altitude);
  //calculates the change in altitude and sets it equal to the height change variable
  //uses absolute value function to consider both stairs ascension and descension

  if (height_change >= 2.4 && height_change <= 4)
    //Height of ET staris is ~8 ft = 2.4 meters
    //maximum stair height is 4 meters
  {
    staircases ++; //adds one to the number of staircases climbed
    past_Altitude = current_Altitude; //resets the past altitude value
  }
  Blynk.virtualWrite(V5, staircases); //prints the number of staircases climbed/descended by user to the app
}

//FUNCTION USED TO DETERMINE MAXIMUM STEADY STATE VALUE
int find_me_the_maximum_please(int *vals, int array_size)
{
  int i = 1;
  //we want to observe the second value in the array
  int you_are_the_MAXIMUM = array_that_holds_calibration_values[0];
  //sets the comparative value equal to the first value in the array

  while (i < 100)
    //as long as i does not exceed the maximum number of values in the array
  {
    if (array_that_holds_calibration_values[i] > you_are_the_MAXIMUM)
      //checks if the value in the array is bigger than the current maximum
    {
      you_are_the_MAXIMUM = array_that_holds_calibration_values[i];
      //if the value in the array is bigger than the current maximum this set that value as the new maximum
    }
    i++;
    //if the value in the array is not bigger than the current maximum the next value in the array will be checked
  }
  return you_are_the_MAXIMUM;
  //function returns the maximum value of the array
}

//FUNCTION USED TO DETERMINE MINIMUM STEADY STATE VALUE
int find_me_the_minimum_please(int *vals, int array_size)
{
  int i = 1;
  //we want to observe the second value in the array
  int you_are_the_MINIMUM = array_that_holds_calibration_values[0];
  //sets the comparative value equal to the first value in the array

  while (i < 100)
    //as long as i does not exceed the maximum number of values in the array
  {
    if (array_that_holds_calibration_values[i] < you_are_the_MINIMUM)
      //checks if the value in the array is bigger than the current maximum
    {
      you_are_the_MINIMUM = array_that_holds_calibration_values[i];
      //if the value in the array is bigger than the current maximum this set that value as the new maximum
    }
    i++;
    //if the value in the array is not bigger than the current maximum the next value in the array will be checked
  }
  return you_are_the_MINIMUM;
  //function returns the maximum value of the array
}

//function used the acquire data from the accelerometer
void acceleration(void)
{
  delay(100);
  LIS.readXYZ(x, y, z);
  LIS.mgScale(x, y, z);
  //Serial.print("Acceleration x: "); //print acceleration
  //Serial.print(x);
  //Serial.print(" mg \ty: ");
  //Serial.print(y);
  //Serial.print(" mg \tz: ");
  //Serial.print(z);
  //Serial.println(" mg");

  //**********************************************************STEP COUNT ALGORITHM******************************************************
  //Goal: calculate the number of steps taken using the accelerometer y values

  if (Lets_Look_At_The_Forward_Swing == true) //means that we want to look at the forward hand
  {
    //Serial.println("hand is moving forward"); //Used to debug - verifies that the program recognizes the hand is moving forward
    if (y > max_steady_state_value)
    {
      //Serial.println("left foot took a step"); //Used to debug - when right arm moves out the left foot takes a step
      steps = steps + 1;
      Lets_Look_At_The_Backward_Swing = true; //going to start the other if statement below
      Lets_Look_At_The_Forward_Swing = false; //halts this if statement, stops more steps from being counted and moves on to the next if statement
    }
  }

  if (Lets_Look_At_The_Backward_Swing == true) //means that we want to look at the backhand
  {
    //Serial.println("hand is moving backward"); //Used to debug - verifies that the program recognizes the hand is moving backward
    if (y < min_steady_state_value)
    {
      //Serial.println("right foot took a step"); //Used to debug - when right arm moves back the right foot takes a step
      steps = steps + 1;
      Lets_Look_At_The_Forward_Swing = true; //going to start the other if statement above
      Lets_Look_At_The_Backward_Swing = false; //halts this if statement, stops more steps from being counted and hops back to the other if statement
    }
  }

}



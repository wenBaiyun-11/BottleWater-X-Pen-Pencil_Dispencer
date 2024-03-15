#include <Arduino.h>
#include <LCDi2c.h>
#include <HX711.h>
#include <Servo.h>

#include "LCDStateMachine.h"

/* [Settings: Start] */

//The lid servo rotation
#define lidStartingRot 0
#define lidTargetRot 180

//The rotation of for dispencing servos
// usage: {0, 180, 0, 0, 180, 0}
//         ^   ^   ^  ^   ^   ^
//         |Servo 1| |Servo 2|
int releaseAction[6] = { 0, 180, 0, 0, 180, 0};

//Target distance before storage is full
#define maxDistBottleStorage 10 //cm

//Set this to the size of your container
#define maxDistPenstorage 50 //cm
#define maxDistPencilStorage  50 //cm

//Bottle Required to claim a price
const int bottleRqrd = 3;

//ScaleFactor update this!!!
const int scaleCalFact = -7050;

//The required weight to accept bottles
const int requiredWeight = 12; //in grams

/* [Settings: END] */

//Put pin mapping here
struct pinsMaps
{
    /* Sensor Modules */

    //[triggerPin, echoPin]
     int BStorageFillLD[2];
     int PnlStorageFillLD[2];
     int penStorageFillLD[2];

    //Lid sensors
     int bottleDetector[2]; // Trigger and Echo
     int weightSensor[2]; // DT and SCK
     int transparencyDetector;


     /* Mechanical Modules */
     int lidServo;
     int pencilDispenser;
     int penDispenser;
     int releaseReward;


     /* Display/Control Modules */
     int LCDI2C[2]; // [0]SDA, [1]SCL
     int LED[2]; // [0] Red, [1] Green
     int BTTN1;
     int BTTN2;
    
};

/* This data structure contains the timing this is used to efficently time sensor data/ operations to cycles.
This will help keep track of time when an object is in front of the sensor */
unsigned long currentTime;
struct timingConstants
{
    //Timings
    const unsigned long sysCHKStatsIntervals = 200; //interval to check for sensor data
    const unsigned long weightSensorIntervals = 4000; //3 seconds long to measure Weight
    const unsigned long actionInterval = 1500; //Given time to dispence item
    /* data */
};

//Stuct Generators
timingConstants TConst;
pinsMaps Pm;
HX711 scale;
LCDi2c LCDController(0x27); //Remember to change this to match the i2C address...
LCDStateMachine displayUI(&LCDController, 20, 4);


// Static Variables
float scaleMeasures = 0;
int rewards = 0;
int bottleDrpCounter = 0;
int machineState = 1; //base on priority [Measure, Idle]
int pen = 0, pencil = 0;

unsigned long pages[2] = {0, 0};
int counter = 0;
int dCounter = 0;

//System Statuses
bool storageCompartFull = false;
bool itemInserted = false;
bool transparentDetector = false;
bool rewardAvailableStatus[2] = {true, true}; //Reward Pen, Pencil


// put function declarations here:

bool isDistance(int triggerPin, int echoPin, int maxRange);
bool isWeightLessThan(int requiredWieght);
bool isTransparent(int receiverPin);
void systemStats();//Always call this function
void dispenceReward();
void stateMachine(int state);


//Timing temp Values
unsigned long systemStartsTime = 0;
unsigned long itemInsertionElapsed = 0;
unsigned long DispenceTime = 0;


//Servo
Servo lidOpener;
Servo pencilContainer;
Servo penContainer;
Servo rewardDispencer;

Servo penAction[6] = {penContainer,penContainer,penContainer,rewardDispencer,rewardDispencer,rewardDispencer};
Servo pencilAction[6] = {pencilContainer,pencilContainer,pencilContainer,rewardDispencer,rewardDispencer,rewardDispencer};

void setup() {
    // Put all pinmode Setup here..
    displayUI.begin();

    /* Setup Scale */
    scale.begin(Pm.weightSensor[0], Pm.weightSensor[1]);
    scale.set_scale(scaleCalFact);
    scale.tare();

    /*Setup servo*/
    lidOpener.attach(Pm.lidServo);
    pencilContainer.attach(Pm.pencilDispenser);
    penContainer.attach(Pm.penDispenser);
    rewardDispencer.attach(Pm.releaseReward);

    /*Button setup*/
    pinMode(Pm.BTTN1, INPUT_PULLUP);
    pinMode(Pm.BTTN2, INPUT_PULLUP);

    /*LED Setup*/
    pinMode(Pm.LED[0], OUTPUT);
    pinMode(Pm.LED[1], OUTPUT);

    /*Ultrasonic setup*/
    pinMode(Pm.bottleDetector[0], OUTPUT);
    pinMode(Pm.bottleDetector[1], INPUT);

    pinMode(Pm.BStorageFillLD[0], OUTPUT);
    pinMode(Pm.BStorageFillLD[1], INPUT);

    pinMode(Pm.penStorageFillLD[0], OUTPUT);
    pinMode(Pm.penStorageFillLD[1], INPUT);

    pinMode(Pm.PnlStorageFillLD[0], OUTPUT);
    pinMode(Pm.PnlStorageFillLD[1], INPUT);

    /*reset Servo positions*/
    lidOpener.write(0);
    pencilContainer.write(0);
    penContainer.attach(0);
    rewardDispencer.attach(0);

    /*Light up the Green LED*/
    digitalWrite(Pm.LED[1], HIGH);
}


void loop() {
    currentTime = millis();

    // Loop for checking sensorValues
    systemStats();

    // Perform action
    stateMachine(machineState);

    //Always dispence the rewards
    dispenceReward();
}

/* [For Processing] */
//Place SensorTiming and detection Logic Here
void systemStats() {
    //Get All sensor data
    if (currentTime - systemStartsTime >= TConst.sysCHKStatsIntervals){

        /*This reads all utrasonic sensor every 500ms to check if an object is detected on a certain distance*/
        rewardAvailableStatus[0] = isDistance(Pm.penStorageFillLD[0], Pm.penStorageFillLD[1], maxDistPenstorage);
        rewardAvailableStatus[1] = isDistance(Pm.PnlStorageFillLD[0], Pm.PnlStorageFillLD[1], maxDistPencilStorage);
        itemInserted = isDistance(Pm.bottleDetector[0], Pm.bottleDetector[2], 10);
        storageCompartFull = isDistance(Pm.BStorageFillLD[0], Pm.BStorageFillLD[1], maxDistBottleStorage);
        transparentDetector = isTransparent(Pm.transparencyDetector);
    
    }

    //Flash red LED when storage is full
    if (storageCompartFull){
        digitalWrite(Pm.LED[0], HIGH);
    } else {
        digitalWrite(Pm.LED[1], LOW);
    }

    //If bottle counter reaches 3, add one to reward.
    if (bottleDrpCounter >= 3){
        rewards = (int)(bottleDrpCounter/3);
        bottleDrpCounter = bottleDrpCounter - rewards*3;
    }

    //Button
    int rewardBTTNState[2] = { digitalRead(Pm.BTTN1), digitalRead(Pm.BTTN2)};
    int rewardBTTNStatePrev[2] = { 0, 0};

    //If pen button is pressed
    if (rewardBTTNState[0] != rewardBTTNStatePrev[0]){
        if ((rewardBTTNState[0] == HIGH) && (rewards > 0) && (rewardAvailableStatus[0])){
            pen++;
            rewards--;
        }
        delay(80);
    }

    //If Pencil button is pressed
    if (rewardBTTNState[1] != rewardBTTNStatePrev[0]){
        if ((rewardBTTNState[1] == HIGH) && (rewards > 0) && (rewardAvailableStatus[1])){
            pencil++;
            rewards--;
        }
        delay(80);
    }

    //always check if there is an item inside the lid
    if (itemInserted) {
        machineState = 0;
        itemInsertionElapsed = currentTime;
    } else {
        machineState = 1;
    }

    systemStartsTime = currentTime;
}


//Perform action!!
void stateMachine(int state){
    switch (state)
    {
    case 0://Measure
            if (currentTime - pages[0] >= 2000){
                displayUI.Measure(scale.get_units(), requiredWeight);
                pages[0] = currentTime;
            }

            //Display error
            if ((currentTime - pages[1] >= 4000)){
                switch (counter)
                {
                case 0:
                    if (!transparentDetector){
                        displayUI.MeasureErr(1);
                    }
                    break;
                case 1:
                    if (!isWeightLessThan(requiredWeight)){
                        displayUI.MeasureErr(0);
                    }
                    break;
                default:
                    counter = 0;
                    break;
                }
                pages[1] = currentTime;
            }

            //wait for 3 seconds before opening the lid
            if (currentTime - itemInsertionElapsed >= TConst.weightSensorIntervals){
                if (isWeightLessThan(requiredWeight) && transparentDetector){
                    lidOpener.write(lidTargetRot);
                    delay(2000);
                    lidOpener.write(lidStartingRot);
                    bottleDrpCounter++;
                }
                itemInsertionElapsed = currentTime;
            }
        break;

    case 1://Idle
        /* Display reward counter */
        displayUI.Idle(rewards);

        if (currentTime - pages[3] >= 2000)
        {
            if (rewardAvailableStatus[0]){
                displayUI.storageStatus(3);
            }

            if (rewardAvailableStatus[1]){
                displayUI.storageStatus(2);
            }
            pages[3] = currentTime;
        }
        break;
    }
}

//Always run this!
void dispenceReward(){
    
    if (currentTime - DispenceTime >= TConst.actionInterval){
        
        if (pen > 0){
            penAction[dCounter].write(releaseAction[dCounter]);
        } else if (pencil > 0){
            pencilAction[dCounter].write(releaseAction[dCounter]);
        }

        if (dCounter > 6){
            if (pen > 0) pen--;
            else if (pencil > 0) pencil--;
            dCounter = 0;
        }
        DispenceTime = currentTime;
    }
}


//HelpFull Functions

//Measure and check if the distance detected matches the  maxrange. will return true if object is within range.
bool isDistance(int triggerPin, int echoPin, int maxRange) {
    
    int distance = -1;
    long pulseDuration = 0;

    //generate a 10µs of pulse
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);

    //Get the pulse Time of arrival
    pulseDuration = pulseIn(echoPin, HIGH);
    distance = pulseDuration*0.034/2;

    //match the distance
    if (distance < maxRange){
        return true;
    }

    return false;
}

// Will return true if the laser is not obstructed
bool isTransparent(int receiverPin) {
    if (digitalRead(receiverPin) == HIGH){
        return true;
    }
    return false;
}

bool isWeightLessThan(int requiredWieght) {
    float measuredWeight = scale.get_units(3);
    if (measuredWeight < requiredWeight){
        return true;
    }
    return false;
}
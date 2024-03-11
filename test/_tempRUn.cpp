#include <Arduino.h>
#include <LCDi2c.h>
#include <HX711.h>

#include "LCDStateMachine.h"
#include "ServoController.h"

/* [Put Constatnt variables here] */
//Put pin mapping here
struct pinsMaps
{
    /* Sensor Modules */

    //[triggerPin, echoPin]
     int bottleDetector[2];
     int BStorageFillLD[2];
     int PnlStorageFillLD[2];
     int penStorageFillLD[2];

     int transparencyDetector;
     int weightSensor[2];


     /* Mechanical Modules */

     int pencilDispenser;
     int penDispenser;
     int releaseReward;


     /* Display/Control Modules */

     int LCDI2C[2];
     int BTTN1;
     int BTTN2;
    /* data */
};

const int bottleRqrd = 3;
const int scaleCalFact = -7050;
const int requiredWeight = 10; //in KG

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

//System Statuses
bool storageCompartFull = false;
bool itemInserted = false;
bool transparentDetector = false;
bool rewardAvailableStatus[2] = {true, true}; //Reward 1, 2


// put function declarations here:

bool isDistance(int triggerPin, int echoPin, int maxRange);
bool isWeightLessThan(int requiredWieght);
bool isTransparent(int receiverPin);
void systemStats();//Always call this function
void dispenceReward(int type);
void stateMachine(int state);


//Timing temp Values
long systemStartsTime = 0;
long itemInsertionElapsed = 0;
long DispenceTime = 0;


void setup() {
    // Put all pinmode Setup here..



    /* Setup Scale */
    scale.begin(Pm.weightSensor[0], Pm.weightSensor[1]);
    scale.set_scale(scaleCalFact);
    scale.tare();
}


void loop() {
    currentTime = millis();

    // Loop for checking sensorValues
    systemStats();

    // Perform action
    stateMachine(machineState);
}

/* [For Processing] */
//Place SensorTiming and detection Logic Here
void systemStats() {
    if (currentTime - systemStartsTime >= TConst.sysCHKStatsIntervals){

        /*This reads all utrasonic sensor every 500ms to check if an object is detected on a certain distance*/
        rewardAvailableStatus[0] = isDistance(Pm.penStorageFillLD[0], Pm.penStorageFillLD[1], 50);
        rewardAvailableStatus[1] = isDistance(Pm.PnlStorageFillLD[0], Pm.PnlStorageFillLD[1], 50);
        itemInserted = isDistance(Pm.bottleDetector[0], Pm.bottleDetector[2], 10);
        storageCompartFull = isDistance(Pm.BStorageFillLD[0], Pm.BStorageFillLD[1], 50);
        transparentDetector = isTransparent(Pm.transparencyDetector);
    
    }

    if (bottleDrpCounter >= 3){
        rewards = bottleDrpCounter%3;
        bottleDrpCounter = bottleDrpCounter - rewards*3;
    }

    int rewardBTTNState[2] = { digitalRead(Pm.BTTN1), digitalRead(Pm.BTTN2)};
    int rewardBTTNStatePrev[2] = { 0, 0};

    if (rewardBTTNState[0] != rewardBTTNStatePrev[0]){
        if ((rewardBTTNState[0] == HIGH) && rewards > 0){
            dispenceReward(0);
            rewards--;
        }
    }

    if (rewardBTTNState[1] != rewardBTTNStatePrev[0]){
        if ((rewardBTTNState[1] == HIGH) && rewards > 0){
            dispenceReward(1);
            rewards--;
        }
    }

    if (itemInserted && transparentDetector) {
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
            displayUI.Measure(scale.get_units(), requiredWeight);
            if (currentTime - itemInsertionElapsed >= TConst.weightSensorIntervals){
                if (isWeightLessThan(requiredWeight)){
                    /* Move Servo here */
                }
                itemInsertionElapsed = currentTime;
            }
        break;

    case 1://Idle
        /* Display reward counter */
        displayUI.Idle(rewards);
        break;
    }
}

void dispenceReward(int type){
    /* Servo queues */
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
    
    return false;
}
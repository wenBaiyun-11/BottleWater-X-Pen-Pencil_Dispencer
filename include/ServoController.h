#include <Servo.h>

class ServoController
{
private:
    int startDeg;
    int endDeg;
    unsigned long prevTiming = 0;
    unsigned long interval;
    int actionQueue = 0;
    Servo controller;
public:
    void servoQueuedRelease(unsigned long currentMillis);
    void forceRelease(unsigned long currentMillis);
    void queueRelease();
    int getDeg();
    ServoController(int servoPMWPin, int startDeg, int endDeg, long interval);
    ~ServoController();
};

ServoController::ServoController(int servoPMW, int startDeg, int endDeg, long interval)
{
    this->interval = interval;
    this->endDeg = endDeg;
    this->startDeg = startDeg;
    controller.attach(servoPMW);
}

void ServoController::queueRelease(){
    actionQueue++;
}

void ServoController::forceRelease(unsigned long currentMillis) {
    if (currentMillis - prevTiming >= interval/2){
        controller.write(startDeg);
        prevTiming = currentMillis;
    }

    if (currentMillis - prevTiming >= interval*2){
        controller.write(endDeg);
        prevTiming = currentMillis;
    }
}

void ServoController::servoQueuedRelease(unsigned long currentMillis){
    if (actionQueue != 0){
        
        if (currentMillis - prevTiming >= interval/2){
             controller.write(startDeg);
            prevTiming = currentMillis;
        }

        if (currentMillis - prevTiming >= interval/2){
             controller.write(endDeg);
            prevTiming = currentMillis;
            actionQueue--;
        }
        
    }
}

int ServoController::getDeg(){
    return controller.read();
}
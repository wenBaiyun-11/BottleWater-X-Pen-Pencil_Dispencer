#include <Wire.h>
#include <LCDi2c.h>

class LCDStateMachine
{
private:
    LCDi2c *LCDobject;
    unsigned long *millisPtr;
public:
    LCDStateMachine(LCDi2c *LCDobject, int col, int row);
    LCDStateMachine(LCDi2c *LCDobject, int col, int row, unsigned long *millisPtr);
    void Idle(int rewardCounter);
    void Measure(int weightMetrics, int maxWeight);
};

LCDStateMachine::LCDStateMachine(LCDi2c *LCDobject, int col, int row){
    LCDobject->begin(col, row);
    this->LCDobject = LCDobject;
}

LCDStateMachine::LCDStateMachine(LCDi2c *LCDobject, int col, int row, unsigned long *millisPtr){
    LCDobject->begin(col, row);
    this->LCDobject = LCDobject;
    this->millisPtr = millisPtr;
}

void LCDStateMachine::Idle(int rewardCounter)
{
    LCDobject->cls();
    LCDobject->locate(2, 0);
    LCDobject->print("[Insert Bottle]");
    LCDobject->locate(1, 2);
    LCDobject->printf("Current Reward: %d", rewardCounter);
}


void LCDStateMachine::Measure(int weightMetrics, int maxWeight)
{
    String stats = "Bad";

    if (weightMetrics > maxWeight){
        stats = "Good";
    }

    LCDobject->cls();
    LCDobject->locate(2, 0);
    LCDobject->print("[Insert Bottle]");
    LCDobject->locate(2, 2);
    LCDobject->printf("%d  %s", maxWeight, stats);
}
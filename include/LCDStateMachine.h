#include <Wire.h>
#include <LCDi2c.h>

class LCDStateMachine
{
private:
    LCDi2c *LCDobject;
    unsigned long *millisPtr;
    int col;
    int row;

public:
    LCDStateMachine(LCDi2c *LCDobject, int col, int row);
    LCDStateMachine(LCDi2c *LCDobject, int col, int row, unsigned long *millisPtr);
    void begin();
    void Idle(int rewardCounter);
    void storageStatus(int e);
    void Measure(int weightMetrics, int maxWeight);
    void MeasureErr(int e);

};

LCDStateMachine::LCDStateMachine(LCDi2c *LCDobject, int col, int row){
    this->row = row;
    this->col = col;
    this->LCDobject = LCDobject;
}

LCDStateMachine::LCDStateMachine(LCDi2c *LCDobject, int col, int row, unsigned long *millisPtr){
    this->row = row;
    this->col = col;
    this->LCDobject = LCDobject;
    this->millisPtr = millisPtr;
}

void LCDStateMachine::begin(){
    LCDobject->begin(col, row);
    LCDobject->home();
    LCDobject->display(DISPLAY_ON);
    LCDobject->display(BACKLIGHT_ON);
    LCDobject->display(CURSOR_OFF);
    LCDobject->display(AUTOSCROLL_OFF);
    LCDobject->display(BLINK_OFF);
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
    String stats = "Denied";
    int spacing = 7;

    if (weightMetrics < maxWeight){
        stats = "Accepted";
        spacing = 5;
    }

        LCDobject->cls();
        LCDobject->locate(2, 0);
        LCDobject->print("[Checking Bottle]");
        LCDobject->locate(5, 2);
        LCDobject->printf("%dg < [%dg]", weightMetrics, maxWeight);
        LCDobject->locate(spacing, 3);
        LCDobject->printf("%s", stats.c_str());

}

void LCDStateMachine::MeasureErr(int e){
    switch (e)
    {
    case 0:
        LCDobject->cls();
        LCDobject->locate(2, 0);
        LCDobject->print("[Checking Bottle]");
        LCDobject->locate(4, 2);
        LCDobject->printf("Please Empty");
        LCDobject->locate(5, 3);
        LCDobject->printf("The bottle");
        break;

    case 1:
        LCDobject->cls();
        LCDobject->locate(2, 0);
        LCDobject->print("[Checking Bottle]");
        LCDobject->locate(3, 2);
        LCDobject->printf("Invalid bottle");
        LCDobject->locate(8, 3);
        LCDobject->printf("Type");
        break;

    default:
        break;
    }
}

void LCDStateMachine::storageStatus(int e){
    switch (e)
    {
    case 1:
        LCDobject->cls();
        LCDobject->locate(2, 0);
        LCDobject->print("[Insert Bottle]");
        LCDobject->locate(4, 2);
        LCDobject->printf("Storage Full");
        break;
    case 2:
        LCDobject->locate(0, 2);
        LCDobject->printf("                   ");
        LCDobject->locate(4, 3);
        LCDobject->printf("No Pencils");
        break;

    case 3:
        LCDobject->locate(0, 2);
        LCDobject->printf("                   ");
        LCDobject->locate(4, 2);
        LCDobject->printf("No Ballpens");
        break;

    default:
        break;
    }
}
#include <Arduino.h>
#include <HX711.h>

#define DoutPINCELL 2
#define SCKPINCELL 3

HX711 scale;

// calibrationFactor measuredScaleWeight/knownWeight

void setup(){
    Serial.begin(9800);
    Serial.println("LoadCell Calibration:");
    scale.begin(DoutPINCELL, SCKPINCELL);
    scale.tare();
}

void loop(){
    if (scale.is_ready())
    {
        scale.get_scale();
        Serial.println("Tare Remove anyweight from scale....");
        delay(5000);
        scale.tare();
        Serial.println("Tare done...");
        Serial.print("Place a known weight on the scale...");
        delay(5000);
        long reading = scale.get_units(10);
        Serial.print("Result: ");
        Serial.println(reading);
    } else {
        Serial.println("HX711 not found.");
    }
    delay(1000);
}
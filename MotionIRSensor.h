#ifndef MotionIRSensor_H
#define MotionIRSensor_H

#include "Debuggable.h"
#include "Sensor.h"
#include "Arduino.h"


class MotionIRSensor : protected Sensor, public Debuggable {
    public:
        inline MotionIRSensor( int boardMotionPin ) { 
            motionPin = boardMotionPin;
        };

        inline void setthresholdSensitivity(int sensitivity){
            thresholdSensitivity = sensitivity; // if ultrasonic reading < thresholdDistancecm return true
        };

        inline int getMotion() { // check if there is a motion
            int motion = digitalRead(motionPin);
            if (motion == HIGH)
              return 1;
            else
              return 0;
        };
        
    private:
        int motionPin;
        int thresholdSensitivity;

};

#endif

#ifndef UltraSonicSensor_H
#define UltraSonicSensor_H

#include "Debuggable.h"
#include "Sensor.h"
#include "UltraSonicSensor.h"
#include "Arduino.h"



class UltraSonicSensor : protected Sensor, public Debuggable {
    public:
        inline UltraSonicSensor( int boardTrigPin, int boardEchoPin, int dist, int repeat ) { 
            trigPin = boardTrigPin;
            echoPin = boardEchoPin; 
			      thresholdDistancecm = dist;
			      repeatReading = repeat;	 //repeat ultrasonic reading to reduce fakse alamr
        };

        inline void updateParameter( int dist, int repeat ) { 
            thresholdDistancecm = dist;
            repeatReading = repeat;  //repeat ultrasonic reading to reduce fakse alamr
        };
        
        int getDistancecm(); // return distance of ultrasonic sensor in cm
        int checkAvgThreshold(); // return 0 or 1 if avarge of repeatReading  < thresholdDistance 
        

    private:
        int trigPin;
        int echoPin;
        int thresholdDistancecm;
        int repeatReading;
		int repeatanimalDist;
};

#endif

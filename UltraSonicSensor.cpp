#include "Sensor.h"
#include "Debuggable.h"
#include "UltraSonicSensor.h"


int UltraSonicSensor::getDistancecm(){
        // Clears the trigPin
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
  
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    int duration = pulseIn(echoPin, HIGH);
    // Calculating the distance in cm
    int localDistance= (duration*0.034/2);
    return localDistance;
};


int UltraSonicSensor::checkAvgThreshold() {

  int distance = getDistancecm();
  
   //check if 
  if (distance < thresholdDistancecm && distance > 0) {
    int sumDistance = distance;
    // recheck the distance repeatReading number of times
    for (int i=1; i < repeatReading; i++) {
      sumDistance = sumDistance + getDistancecm();
    }//for loop
 
    distance = sumDistance / (repeatReading); // average distance over all calculation

    if (distance < thresholdDistancecm && distance > 0)
      return 1;
  }//if animalDist

  return 0; 
};


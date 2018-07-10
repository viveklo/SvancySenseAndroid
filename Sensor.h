#ifndef Sensor_h
#define Sensor_h

#include "Arduino.h"

class Sensor {
    public:
        //initialize the sensor
        inline virtual void begin(){ /*nothing*/ }; 
        //read function must be implemented
        //this is called a pure virtual function
        //virtual int read() = 0; 
		inline void setSensorParameters(char sens_name[12], long sens_ver, long sens_id, long sens_type, float max_val, float min_val, float res){
			strcpy(sensor_name, sens_name);
			sensor_version = sens_ver;
			sensor_id = sens_id;
			sensor_type = sens_type;
			max_value = max_val;
			min_value = min_val;
			resolution = res;
		};
     protected:
        char     sensor_name[12];                      /**< sensor name */
        long     sensor_version;                         /**< version of the hardware + driver */
        long     sensor_id;                       /**< unique sensor identifier */
        long     sensor_type;                            /**< this sensor's type (ex. SENSOR_TYPE_LIGHT) */
        float    max_value;                       /**< maximum value of this sensor's value in SI units */
        float    min_value;                       /**< minimum value of this sensor's value in SI units */
        float    resolution;                      /**< smallest difference between two values reported by this sensor */
};

#endif

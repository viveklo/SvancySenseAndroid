#ifndef Debuggable_H
#define Debuggable_H

class Debuggable {
    public:
        inline void debug(char* debugString){ 
            Serial.println(debugString); 
        }
};

#endif

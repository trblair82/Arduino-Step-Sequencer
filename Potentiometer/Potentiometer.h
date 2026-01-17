#ifndef POTENTIOMETER
#define POTENTIOMETER
#include "Arduino.h"
class Potentiometer{
  private:
    int potPin;
    long smoothedValue;
    int minRange;
    int maxRange;
    int fudge;
    int alpha = 4;


  public:
    Potentiometer();
    init(int pin, int min_range, int max_range, int fudge);
    checkPot();
};




#endif
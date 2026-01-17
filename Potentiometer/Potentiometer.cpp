#include "Potentiometer.h"

Potentiometer::Potentiometer(){
  potPin = 0;
  smoothedValue = 0;
  minRange = 0;
  maxRange = 1023;
  fudge = 2;
}

Potentiometer::init(int pin, int min_range, int max_range, int fudge){
  potPin = pin;
  minRange = min_range;
  maxRange = max_range;
  fudge = fudge;
  pinMode(potPin, INPUT);
  digitalWrite(potPin, LOW);
}

Potentiometer::checkPot(){
  
    int rawValue = analogRead(potPin);
    //long newSmooth = (rawValue + (smoothedValue * (2^K - 1))) >> K;
    long newSmooth = (alpha*smoothedValue + (32-alpha)*rawValue + 16)/32;
    
    if (abs(smoothedValue - newSmooth) > fudge){
      smoothedValue = newSmooth;
      return map(smoothedValue, 0, 1023, minRange, maxRange);
    }else{
      return map(smoothedValue, 0, 1023, minRange, maxRange);
    }
    
}

#ifndef BUTTON
#define BUTTON
#include "Arduino.h"

class Button{

  private:
    int buttonPin;
    int buttonState;
    int debounceDelay;
    unsigned long lastDebounce;
    unsigned long debounceTimer;

  public:
    Button();

    void init(int pin, int debounce_milli);

    bool checkButton();

    bool checkButton(unsigned int ticks);
};
#endif
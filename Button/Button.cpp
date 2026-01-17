#include <Button.h>

#include "Button.h"
Button::Button() {
  buttonPin = 0;
  buttonState = false;
  debounceDelay = 10;
  lastDebounce = 0;
  debounceTimer = 0;
}

void Button::init(int pin, int debounce_milli) {
  buttonPin = pin;
  debounceDelay = debounce_milli;
  pinMode(buttonPin, INPUT_PULLUP);
}

bool Button::checkButton() {
  boolean next_state;
  if (digitalRead(buttonPin) == LOW) {
    next_state = true;
  } else {
    next_state = false;
  }
  //See if the state has changed
  if (next_state != buttonState) {
    //See if enough time has passed to change the state
    if ((millis() - lastDebounce) > debounceDelay) {
      //Okay to change state:
      buttonState = next_state;
      //Reset time count
      lastDebounce = millis();
    }
  }
  return buttonState;
}


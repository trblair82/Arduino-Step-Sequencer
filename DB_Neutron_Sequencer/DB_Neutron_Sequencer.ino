#include <Potentiometer.h>
#include "MIDIUSB.h"  // Library for midi over usb
#include <Button.h>
#include <Wire.h>              // Required for I2C communication
#include <Adafruit_GFX.h>      // Core graphics library
#include <Adafruit_SSD1306.h>  // SSD1306 driver library

// Define display dimensions
#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH 128
// Define noteBank selection constants
#define OCTAVE 2
#define NOTE 0
#define VELOCITY 1

// Initialize display with I2C address (0x3C or 0x3D)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
// Determines when to update display
bool displayChanged = false;
// Cordinates for start of step notes display
byte stepLineY = 20;
byte stepLineX = 0;
// Cordinates for displaying octave of selected step
const byte octaveLineX = 0;
const byte octaveLineY = 0;
// Cordinates for displaying selected bank
const byte bankLineX = 0;
const byte bankLineY = 10;
// Cordinates for displaying selected note for editing
const byte editLineX = 55;
const byte editLineY = 40;
// Output midi channel
const byte midiChannel = 1;
// Pin to read from external cv clock source
const byte clockPin = 7;
// Values needed for play/stop button
const byte playButtonPin = 6;
Button playButton;
bool playButtonState = false;
bool playButtonPstate = false;
// Used to track current state of play ie off or on
bool playState = false;
bool playPState = true;
// Variables used to track bank select button
Button bankButton;
const byte bankButtonPin = 12;
bool bankButtonState = false;
bool bankButtonPState = false;
// Number of steps per bank
const byte numSteps = 8;
// Variables needed to track step select buttons
Button stepButtons[numSteps];
const byte stepButtonPins[numSteps] = { A5, A4, 4, 5, 8, 9, 10, 11 };
bool stepButtonStates[numSteps];
bool stepButtonPStates[numSteps];
// Variables needed to track note select potentiometer
const byte potentiometerPin = A3;
Potentiometer potentiometer;
byte potState = 0;
byte potPState = 0;
const int POT_TIMEOUT = 300;
const int potThreshold = 2;
boolean potMoving = true;
unsigned long potTime;
unsigned long timer;
// Variables used to track current step according to external clock or bpm
byte clockCount = 0;
byte lastClockCount = 0;
byte clockPState = LOW;
// Set start number of banks, max number of banks and start bank
byte numBanks = 1;
const byte maxBanks = 8;
byte currentBank = 0;
// used to initialize note values
byte firstNote = 0;
byte startOctave = 5;
// 12 notes or one octave available for note selection
const byte numNotes = 12;
String noteNames[numNotes] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
// Container for step note, velocity and octave values
byte noteBanks[maxBanks][numSteps][3];
// note sustain in milliseconds
byte noteLength = 200;
// Used to trigger note off messages after note length
bool noteDone = true;
int noteTimer = 0;
// Note value of currently playing step
byte currentNote = 0;
// Variables used to clear and update display
byte editStep = 0;
byte lastEditStep = 0;
byte editBank = 0;
byte lastEditBank = 0;
byte lastOctave = 0;
String lastEditLine = "C";
// Beats per minute value and timer for when no external clock
byte bpm = 80;
int bpmTimer = 0;
unsigned long bpmDelay = 0;
// Whether to use external clock or internal bpm logic
bool externalClock = false;

void setup() {
  Serial.begin(9600);
  if (externalClock == true) {
    pinMode(clockPin, INPUT);
  }
  // Initialize play, bank select and note select
  playButton.init(playButtonPin, 10);
  bankButton.init(bankButtonPin, 10);
  potentiometer.init(potentiometerPin, 0, 50, potThreshold);
  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Use 0x3D if 0x3C fails
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed
  }
  // Show initial Adafruit splash screen
  display.display();
  delay(2000);
  // Clear the display buffer
  display.clearDisplay();
  // Set text properties
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  // Set up step select buttons
  for (int i = 0; i < numSteps; i++) {
    stepButtons[i].init(stepButtonPins[i], 10);
    stepButtonStates[i] = false;
    stepButtonPStates[i] = false;
  }
  // Initialize banks with note, velocity and octave values
  for (int i = 0; i < numBanks; i++) {
    for (int j = 0; j < numSteps; j++) {
      noteBanks[i][j][NOTE] = firstNote;
      noteBanks[i][j][VELOCITY] = 127;
      noteBanks[i][j][OCTAVE] = startOctave;
      if (firstNote == numNotes - 1) {
        startOctave++;
        firstNote = 0;
      } else {
        firstNote++;
      }
    }
  }
  // Calls update functions for each display section
  updateDisplay();
  // Sets the delay to use with timer to set bpm
  bpmDelay = 60000 / bpm / 2;
}

void loop() {
  // Update external clock or internal bpm timers
  if (externalClock == true) {
    updateClock();
  } else {
    updateBpm();
  }
  // Check to see if current note has been held long enough
  // Send note of message if it has
  int currentTime = millis();
  if (currentTime - noteTimer > noteLength) {
    if (noteDone == false) {
      noteOn(midiChannel, calculateMidiNote(currentBank, currentNote), 0);
      MidiUSB.flush();
      noteDone = true;
    }
  }
  // Checks state of step select and bank select buttons and updates display values
  // Sets selected step and bank to edit
  updateStepEditor();
  updateBankEditor();
  // Update note for selected step based on potentiometer
  updateStepNote();
  // Updated display if it has been flagged for changes
  if (displayChanged == true) {
    updateDisplay();
    Serial.println("fuck");
  }
  // Checks state of play/stop button and updates current state
  updatePlayState();
  // Check to see if we are currently playing a sequence
  if (playState == true) {
    if (playState != playPState) {
      // reset sequence to begining on play
      clockCount = 0;
      currentBank = 0;
    }
    playPState = playState;
    // If our clock has cycled go to next step
    if (clockCount != lastClockCount) {
      updateStep();
      lastClockCount = clockCount;
    }
    // if stop button has been pressed reset sequence and kill all notes
  } else if (playState != playPState) {
    playPState = playState;
    currentBank = 0;
    clockCount = 0;
    for (int i = 0; i < numBanks; i++) {
      for (int j = 0; j < numSteps; j++) {
        noteOn(midiChannel, noteBanks[i][j][calculateMidiNote(noteBanks[i], noteBanks[j])], 0);
      }
    }
    // Send messages from midi buffer
    MidiUSB.flush();
  }
}
// Function used to update external clock from digital pin input when using external clock
void updateClock() {
  byte clockState = digitalRead(clockPin);
  if (clockState == HIGH) {
    if (clockState != clockPState) {
      if (clockCount == numSteps - 1) {
        clockCount = 0;
        if (currentBank < numBanks - 1) {
          currentBank++;
        } else {
          currentBank = 0;
        }
      } else {
        clockCount++;
      }
    }
  }
  clockPState = clockState;
}
// Function that uses a timer to update our clock based on defined bpm setting
void updateBpm() {
  int currentTime = millis();
  if (currentTime - bpmTimer > bpmDelay) {
    if (clockCount == numSteps - 1) {
      clockCount = 0;
      if (currentBank < numBanks - 1) {
        currentBank++;
      } else {
        currentBank = 0;
      }
    } else {
      clockCount++;
    }
    bpmTimer = millis();
  }
}
// Checks step select buttons to select which step to edit
void updateStepEditor() {
  for (int i = 0; i < numSteps; i++) {
    stepButtonStates[i] = stepButtons[i].checkButton();
    if (stepButtonStates[i] != stepButtonPStates[i]) {
      stepButtonPStates[i] = stepButtonStates[i];
      if (stepButtonStates[i] == true) {
        editStep = i;
        if (lastEditStep != editStep) {
          displayChanged = true;
          lastEditStep = editStep;
          break;
        }
        break;
      }
      break;
    }
  }
}
// Checks bank select button to select bank for editing
void updateBankEditor() {
  bankButtonState = bankButton.checkButton();
  if (bankButtonState != bankButtonPState) {
    bankButtonPState = bankButtonState;
    if (bankButtonState == true) {
      if (editBank < numBanks - 1) {
        editBank++;
      } else {
        editBank = 0;
      }
      displayChanged = true;
    }
  }
}
// Reads note select potentiometer and updates currently selected step note in note banks
void updateStepNote() {
  potState = potentiometer.checkPot();
  Serial.println(potState);
  if (abs(potState - potPState) > potThreshold) {
    potTime = millis();
  }
  timer = millis() - potTime;
  if (timer < POT_TIMEOUT) {
    potMoving = true;
  } else {
    potMoving = false;
  }
  if (potMoving == true) {
    if (potPState != potState) {
      byte note = noteBanks[editBank][editStep][NOTE];
      if (editStep != clockCount) {
        if (potState > potPState and note < numNotes - 1) {
          noteBanks[editBank][editStep][NOTE]++;
        }
        if (potState < potPState and note > 0) {
          noteBanks[editBank][editStep][NOTE]--;
        }
      }
      potPState = potState;
      displayChanged = true;
    }
  }
}
// Checks play button to see if we should start or stop the sequence
void updatePlayState() {
  playButtonState = playButton.checkButton();
  if (playButtonState != playButtonPstate) {
    playButtonPstate = playButtonState;
    if (playButtonState == true) {
      playState = !playState;
    }
  }
}
// Called when our clock increments, sends note on message for next step
void updateStep() {
  if (noteDone == true) {
    int nextNote = calculateMidiNote(currentBank, clockCount);
    noteOn(midiChannel, nextNote, noteBanks[currentBank][clockCount][VELOCITY]);
    currentNote = clockCount;
    MidiUSB.flush();
    // Used to determine when to send note off message for the step that just played
    noteTimer = millis();
    noteDone = false;
  }
}
// Function to call update functions for display sections and update display
// Note we are never clearing the display always overwriting with black for performance
void updateDisplay() {
  updateOctaveLine();
  updateBankLine();
  updateStepLine();
  updateEditLine();
  display.display();
  displayChanged = false;
}
// Updates octave of selected step note on display
void updateOctaveLine() {
  display.setCursor(octaveLineX, octaveLineY);
  display.setTextColor(BLACK);
  display.print("octave = ");
  display.print(lastOctave);
  display.setCursor(octaveLineX, octaveLineY);
  display.setTextColor(WHITE);
  display.print("octave = ");
  display.print(noteBanks[editBank][editStep][OCTAVE]);
  lastOctave = noteBanks[editBank][editStep][OCTAVE];
}
// Updates selected bank on display
// Previous display data is overwitten in black
// This is much faster than calling display.clearDisplay()
void updateBankLine() {
  display.setCursor(bankLineX, bankLineY);
  display.setTextColor(BLACK);
  display.print("bank = ");
  display.print(lastEditBank);
  display.setCursor(bankLineX, bankLineY);
  display.setTextColor(WHITE);
  display.print("bank = ");
  display.print(editBank);
  lastEditBank = editBank;
}

// Updates list of step notes for currently selected bank on display
void updateStepLine() {
  display.fillRect(stepLineX, stepLineY, 128, 10, BLACK);
  for (int i = 0; i < numSteps; i++) {
    display.setCursor(stepLineX, stepLineY);
    display.setTextColor(WHITE);
    display.print(noteNames[noteBanks[editBank][i][NOTE]]);
    int x_offset = noteNames[noteBanks[editBank][i][NOTE]].length() * 5;
    stepLineX += x_offset + 7;
  }
  stepLineX = 0;
}
// Updates display of currentlu selected step note to edit
void updateEditLine() {
  display.setTextSize(2);
  display.setCursor(editLineX, editLineY);
  display.setTextColor(BLACK);
  display.print(lastEditLine);
  display.setCursor(editLineX, editLineY);
  display.setTextColor(WHITE);
  display.print(noteNames[noteBanks[editBank][editStep][NOTE]]);
  display.setTextSize(1);
  lastEditLine = noteNames[noteBanks[editBank][editStep][NOTE]];
}




// Used if we ever want to recieve midi from external gear for processing
void checkMessage(midiEventPacket_t rx) {
  switch (rx.header) {
    case 0x09:
      break;
    case 0x08:
      break;
    case 0x0B:
      break;
  }
}
// Helper function to calculate midi note number for given note and octave values
byte calculateMidiNote(byte bank, byte note) {
  return noteBanks[bank][note][NOTE] + noteBanks[bank][note][OCTAVE] * numNotes;
}
// Helper function to send note off midi messages
void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = { 0x08, 0x80 | channel, pitch, velocity };
  MidiUSB.sendMIDI(noteOff);
}
// Helper function to send midi cc messages
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = { 0x0B, 0xB0 | channel, control, value };
  MidiUSB.sendMIDI(event);
}
// Helper function to send midi pitch bend control messages
void pitchBendChange(byte channel, byte value) {
  byte lowValue = value & 0x7F;
  byte highValue = value >> 7;
  midiEventPacket_t event = { 0x0E, 0xE0 | channel, lowValue, highValue };
  MidiUSB.sendMIDI(event);
}
// Helper function to send midi note on messages
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = { 0x09, 0x90 | channel, pitch, velocity };
  MidiUSB.sendMIDI(noteOn);
}

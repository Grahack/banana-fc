#include <Arduino.h>
#include <HardwareSerial.h>

// number of buttons
#define NB 4
// long press interval in millis
#define LONG_PRESS_INTERVAL 500

// from https://www.instructables.com/id/Send-and-Receive-MIDI-with-Arduino/
// see also https://www.midi.org/
// 0b10000000 = note off
#define N0 128
// 0b10010000 note on
#define N1 144
// 0b10100000 aftertouch
#define AT 160
// 0b10110000 continuous controller
#define CC 176
// 0b11000000 patch change
#define PC 192
// 0b11010000 channel pressure
#define CP 208
// 0b11100000 pitch bend
#define PB 224
// 0b11110000 non-musical commands
#define NM 240

// the state of the buttons
bool B[NB];
// the previous state of the buttons
bool P[NB];
// the date of last press
long D[NB];

void MIDImessage1(int command, int data1) {
    Serial.write(command);
    Serial.write(data1);
}

void MIDImessage2(int command, int data1, int data2) {
    Serial.write(command);
    Serial.write(data1);
    Serial.write(data2);
}

void press(int button) {
    MIDImessage1(PC, button);
}

void release(int button) {
}

void long_press(int button) {
}

void setup() {
    Serial.begin(31250);
    for (int i = 0; i < NB; i++){
        // button number i is on pin i+2
        pinMode(i+2, INPUT_PULLUP);
        B[i] = 0;
        P[i] = 0;
        D[i] = 0;
    }
}

void loop() {
    // check button state
    for (int i = 0; i < NB; i++){
        // button number i is on pin i+2
        B[i] = digitalRead(i+2)==LOW? 1 : 0;
    }
    // handle button state
    for (int i = 0; i < NB; i++){
        // press detect
        if (B[i] && !P[i]) {
            press(i);
            D[i] = millis();
        }
        // release and long press detect
        if (!B[i] && P[i]) {
            release(i);
            if (millis() - D[i] > LONG_PRESS_INTERVAL) {
                long_press(i);
            }
        }
    }
    // store "previous button state" for next loop
    for (int i = 0; i < NB; i++){
        P[i] = B[i];
    }
}

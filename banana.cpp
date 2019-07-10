#include <Arduino.h>
#include <HardwareSerial.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

/**
 * PINS
 *      1 serial OUT
 * 2 -  5 buttons
 * 6 - 11 screen
 **/
// number of buttons
#define NB 4
// number of pages
#define NP 99
// LCD pins init
LiquidCrystal lcd(11, 10, 9, 8, 7, 6);
// long press interval in millis
#define LONG_PRESS_INTERVAL 1200

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

// mode 0: normal mode
// mode 1: edit mode
int mode;
// the state of the buttons
bool B[NB];
// the previous state of the buttons
bool P[NB];
// the date of last press
long D[NB];
// the number of buttons that are pressed during a loop
int total_pressed;
// the number of buttons that were pressed during the previous loop
int prev_total_pressed;
// long press flag
bool already_long_pressed;
// the buttons that will be taken into account for a simultaneous press
bool S[NB];
// the NP pages of NB buttons
int pages[NP][NB];
// the current page (normal mode and edit mode)
int page;
// the edited button (last long pressed, edit mode only)
int edited_button;
// the preset to be commited to EEPROM
int preset;

void update_LCD_page(int page) {
    lcd.setCursor(0, 0);
    lcd.print(" <<  PAGE ");
    if (page < 10 - 1) {
        lcd.print(" ");
    }
    lcd.print(page + 1);
    lcd.print("  >>");
}

void update_LCD_preset(int preset) {
    lcd.setCursor(0, 1);
    lcd.print("PRESET ");
    if (preset + 1 < 10) {
        lcd.print(" ");
    }
    lcd.print(preset + 1);
    // clean potential trailing characters
    lcd.print("         ");
}

void MIDImessage1(int command, int data1) {
    Serial.write(command);
    Serial.write(data1);
}

void MIDImessage2(int command, int data1, int data2) {
    Serial.write(command);
    Serial.write(data1);
    Serial.write(data2);
}

void press_normal(int button) {
    int data = pages[page][button];
    update_LCD_preset(data);
    MIDImessage1(PC, data);
}

void press_edit(int button) {
}

void release_normal(int button) {
}

void release_edit(int button) {
    switch (button) {
        case 0:
            if (preset >= 10) {
                preset -= 10;
            }
            break;
        case 1:
            if (preset >= 1) {
                preset -= 1;
            }
            break;
        case 2:
            if (preset < 127) {
                preset += 1;
            }
            break;
        case 3:
            if (preset < 117) {
                preset += 10;
            }
            break;
    }
    update_LCD_preset(preset);
    MIDImessage1(PC, preset);
}

void simultaneous_release_normal(bool S[]) {
    if (S[0] && S[1] && !S[2] && !S[3] && page > 0) {
        page--;
        update_LCD_page(page);
    } else if (!S[0] && !S[1] && S[2] && S[3] && page < 98) {
        page++;
        update_LCD_page(page);
    }
    // use first preset of the page
    int data = pages[page][0];
    update_LCD_preset(data);
    MIDImessage1(PC, data);
}

void simultaneous_release_edit(bool S[]) {
    bool when_quiting = false;
    if (S[0] && S[1] && !S[2] && !S[3]) {
        // CANCEL
        int data = pages[page][edited_button];
        MIDImessage1(PC, data);
        mode = 0;
        lcd.home();
        lcd.print("    CANCEL     ");
        when_quiting = true;
    } else if (!S[0] && !S[1] && S[2] && S[3]) {
        // COMMIT
        pages[page][edited_button] = preset;
        lcd.home();
        int addr = 4*page + edited_button;
        if (EEPROM.read(addr) != preset) {
            EEPROM.write(addr, preset);
            lcd.print("       OK       ");
        } else {
            lcd.print("    NO CHANGE   ");
        }
        MIDImessage1(PC, preset);
        mode = 0;
        when_quiting = true;
    }
    if (when_quiting) {
        delay(1000);
        update_LCD_page(page);
        update_LCD_preset(pages[page][edited_button]);
    }
}

void long_press_normal(int button) {
    mode = 1;
    edited_button = button;
    preset = pages[page][button];
    lcd.home();
    lcd.print("Editing ");
    if (page + 1 < 10) {
        lcd.print(" ");
    }
    lcd.print(page + 1);
    lcd.print("-");
    lcd.print(button + 1);
    lcd.print("    ");
}

void long_press_edit(int button) {
}

void long_release_normal(int button) {
}

void long_release_edit(int button) {
}

void press(int button) {
    switch (mode) {
        case 0: press_normal(button); break;
        case 1: press_edit(button); break;
    }
}

void release(int button) {
    switch (mode) {
        case 0: release_normal(button); break;
        case 1: release_edit(button); break;
    }
}

void simultaneous_release(bool S[]) {
    switch (mode) {
        case 0: simultaneous_release_normal(S); break;
        case 1: simultaneous_release_edit(S); break;
    }
}

void long_press(int button) {
    switch (mode) {
        case 0: long_press_normal(button); break;
        case 1: long_press_edit(button); break;
    }
}

void long_release(int button) {
    switch (mode) {
        case 0: long_release_normal(button); break;
        case 1: long_release_edit(button); break;
    }
}

void setup() {
    // Welcome !
    lcd.begin(16, 2);
    lcd.print(" << Banana FC >>");
    // init vars
    mode = 0;
    for (int i = 0; i < NB; i++) {
        // button number i is on pin i+2
        // i starts from 0: i=0 for button numbered 1 on the hardware
        pinMode(i+2, INPUT_PULLUP);
        B[i] = false;
        P[i] = false;
        D[i] = 0;
        S[i] = false;
    }
    total_pressed = 0;
    prev_total_pressed = 0;
    already_long_pressed = 0;
    page = 0;
    edited_button = 0;
    preset = 0;
    // check for BANANA at the end of the EEPROM
    //int L = EEPROM.length();
    //lcd.print(L);
    int offset = 400;
    int ASCII_BANANA[6] = {66, 65, 78, 65, 78, 65};
    bool do_not_initialize = true;
    for (int i = 0; i < 6; i++) {
        int value = EEPROM.read(offset + i);
        if (value != ASCII_BANANA[i]) {
            do_not_initialize = false;
        }
    }
    lcd.setCursor(6, 1);
    if (!do_not_initialize) {
        // init EEPROM and pages var
        lcd.print("Init!");
        for (int i = 0; i < NP; i++) {
            for (int j = 0; j < NB; j++) {
                pages[i][j] = 4*i + j;
                EEPROM.write(4*i + j, 4*i + j);
            }
        }
        // set the marker
        for (int i = 0; i < 6; i++) {
            EEPROM.write(offset + i, ASCII_BANANA[i]);
        }
    } else {
        // read from EEPROM
        lcd.print("Ready");
        for (int i = 0; i < NP; i++) {
            for (int j = 0; j < NB; j++) {
                pages[i][j] = EEPROM.read(4*i + j);
            }
        }
    }
    // other setup tasks
    Serial.begin(31250);
}

void loop() {
    total_pressed = 0;
    // check button state
    for (int i = 0; i < NB; i++) {
        // button number i is on pin i+2
        B[i] = digitalRead(i+2)==LOW? true : false;
        if (B[i]) {
            total_pressed++;
        }
    }
    // handle single button state
    for (int i = 0; i < NB; i++) {
        // press detect
        if (B[i] && !P[i] && (total_pressed == 1)) {
            press(i);
            D[i] = millis();
            already_long_pressed = 0;
        }
        // long press detect
        if (!already_long_pressed && B[i] && P[i] && (total_pressed == 1)) {
            if (millis() - D[i] > LONG_PRESS_INTERVAL) {
                long_press(i);
                already_long_pressed = 1;
            }
        }
        // release and long release detect
        if (!B[i] && P[i] && !already_long_pressed) {
            release(i);
            if (millis() - D[i] > LONG_PRESS_INTERVAL) {
                long_release(i);
            }
        }
    }
    // accumulate pressed buttons
    if (total_pressed > prev_total_pressed) {
        for (int i = 0; i < NB; i++) {
            S[i] = B[i];
        }
        prev_total_pressed = total_pressed;
    }
    // handle simultaneous release
    if (prev_total_pressed > 1 && total_pressed == 0) {
        simultaneous_release(S);
        for (int i = 0; i < NB; i++) {
            S[i] = 0;
        }
        prev_total_pressed = 0;
    }
    // store "previous button state" for next loop
    for (int i = 0; i < NB; i++) {
        P[i] = B[i];
    }
}

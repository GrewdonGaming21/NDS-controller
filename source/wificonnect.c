/*
 * Copyright (c) 2015 Louis van Harten
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "wificonnect.h"

int WFCConnect(PrintConsole *console) {
    consoleSelect(console);
    consoleClear();

    iprintf("\nLoading connection info \nfrom WFC storage \n\n(set-up using commercial rom)");
    iprintf("\n\nConnecting...");

    if(!Wifi_InitDefault(WFC_CONNECT)) {
        iprintf("Failed to connect!");
        return -1;
    } else {
        iprintf("Done\n");
        return 0;
    }
}

int ManualConnect(PrintConsole *top_screen, PrintConsole *bot_screen) {
    int status = ASSOCSTATUS_DISCONNECTED, wepmode = WEPMODE_NONE;
    char wepkey[64];

    consoleSelect(bot_screen);
    iprintf("\n\nScanning...");
    //consoleSelect(top_screen); TODO decide where to put it all
    consoleClear();

    Wifi_InitDefault(INIT_ONLY);
    Wifi_AccessPoint* ap = findAP();

    iprintf("Connecting to %s\n", ap->ssid);

    Wifi_SetIP(0,0,0,0,0);

    if (ap->flags & WFLAG_APDATA_WEP) {
        iprintf("Enter Wep Key\n");
        while (wepmode == WEPMODE_NONE) {
            scanf("%s",wepkey);
            if (strlen(wepkey)==13) {
                wepmode = WEPMODE_128BIT;
            } else if (strlen(wepkey) == 5) {
                wepmode = WEPMODE_40BIT;
            } else {
                iprintf("Only 5-digit and 13-digit WEP keys are supported!\n");
                return -1;
            }
        }
        Wifi_ConnectAP(ap, wepmode, 0, (u8*)wepkey);
    } else {
        Wifi_ConnectAP(ap, WEPMODE_NONE, 0, 0);
    }
    consoleClear();
    while(status != ASSOCSTATUS_ASSOCIATED && status != ASSOCSTATUS_CANNOTCONNECT) {
        status = Wifi_AssocStatus();
        consoleClear();
        iprintf("Connecting. \n\nPress (B) to cancel");
        scanKeys();
        if(keysDown() & KEY_B) {
            break;
        }
        swiWaitForVBlank();
    }
    if(status == ASSOCSTATUS_ASSOCIATED) {
        return 0;
    } else {
        return -1;
    }
}

Wifi_AccessPoint* findAP() {
    int i;
    int selected = 0;
    int count = 0, displaytop = 0, displayend;
    int pressed = 0;
    static Wifi_AccessPoint ap;
    Wifi_ScanMode();
    do {
        scanKeys();
        pressed = keysDown();

        count = Wifi_GetNumAP();
        consoleClear();
        iprintf(" %d Available APs Detected \n\n\n\n\n", count);
        displayend = displaytop + 4;
        if (displayend > count) {
            displayend = count;
        }
        for(i = displaytop; i < displayend; i++) {
            Wifi_AccessPoint tap;
            Wifi_GetAPData(i, &tap);
            if(tap.flags & WFLAG_APDATA_WPA) {
                iprintf(" %s %.28s\n    WPA Sig:%i\n\n", i == selected ? "-X" : "  ", tap.ssid,
                 tap.rssi * 100 / 0xD0);
            } else {
                iprintf(" %s %.28s\n    %s Sig:%i\n\n", i == selected ? "->" : "  ", tap.ssid,
                        tap.flags & WFLAG_APDATA_WEP ? "WEP  " : "Open ", tap.rssi * 100 / 0xD0);
            }
        }
        if(pressed & KEY_UP) {
            if(selected > 0) {
                selected--;
            }
            if(selected<displaytop) {
                displaytop = selected;
            }
        }
        if(pressed & KEY_DOWN) {
            if(selected+1 < count) {
                selected ++;
            }
            displaytop = selected - 3;
            if (displaytop<0) {
                displaytop = 0;
            }
        }
        if(pressed & KEY_B) {
            return NULL;
        }
        swiWaitForVBlank();
        Wifi_GetAPData(selected, &ap);
    } while(!(pressed & KEY_A) || ap.flags & WFLAG_APDATA_WPA);

    return &ap;
}
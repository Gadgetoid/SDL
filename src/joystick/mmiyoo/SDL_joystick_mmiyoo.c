/*
  Customized version for Miyoo-Mini handheld.
  Only tested under Miyoo-Mini stock OS (original firmware) with Parasyte compatible layer.

  Copyright (C) 1997-2022 Sam Lantinga <slouken@libsdl.org>
  Copyright (C) 2022-2022 Steward Fu <steward.fu@gmail.com>


  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if defined(SDL_JOYSTICK_MMIYOO)

#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

#include "SDL_events.h"
#include "SDL_error.h"
#include "SDL_mutex.h"
#include "SDL_timer.h"
#include "../../thread/SDL_systhread.h"

#define MYKEY_UP            0
#define MYKEY_DOWN          1
#define MYKEY_LEFT          2
#define MYKEY_RIGHT         3
#define MYKEY_A             4
#define MYKEY_B             5
#define MYKEY_X             6
#define MYKEY_Y             7
#define MYKEY_L1            8
#define MYKEY_R1            9
#define MYKEY_L2            10
#define MYKEY_R2            11
#define MYKEY_SELECT        12
#define MYKEY_START         13
#define MYKEY_MENU          14
#define MYKEY_QSAVE         15
#define MYKEY_QLOAD         16
#define MYKEY_FF            17
#define MYKEY_EXIT          18
#define MYKEY_POWER         19
#define MYKEY_VOLUP         20
#define MYKEY_VOLDOWN       21

static SDL_sem *pad_sem = NULL;
static SDL_Thread *thread = NULL;
static int running = 0;
static int event_fd = -1;

uint32_t current_input = 0;
uint32_t previous_input = 0;

int JoystickUpdate(void *data)
{
    uint32_t bit = 0;
    struct input_event ev = {0};

    while (running) {
        SDL_SemWait(pad_sem);
        if (event_fd > 0) {
            if (read(event_fd, &ev, sizeof(struct input_event))) {
                if ((ev.type == EV_KEY) && (ev.value != 2)) {
                    //printf("%s, code:%d\n", __func__, ev.code);

                    switch (ev.code) {
                    case 103: bit = (1 << MYKEY_UP);      break;
                    case 108: bit = (1 << MYKEY_DOWN);    break;
                    case 105: bit = (1 << MYKEY_LEFT);    break;
                    case 106: bit = (1 << MYKEY_RIGHT);   break;
                    case 57:  bit = (1 << MYKEY_A);       break;
                    case 29:  bit = (1 << MYKEY_B);       break;
                    case 42:  bit = (1 << MYKEY_X);       break;
                    case 56:  bit = (1 << MYKEY_Y);       break;
                    case 28:  bit = (1 << MYKEY_START);   break;
                    case 97:  bit = (1 << MYKEY_SELECT);  break;
                    case 18:  bit = (1 << MYKEY_L1);      break;
                    case 15:  bit = (1 << MYKEY_L2);      break;
                    case 20:  bit = (1 << MYKEY_R1);      break;
                    case 14:  bit = (1 << MYKEY_R2);      break;
                    case 1:   bit = (1 << MYKEY_MENU);    break;
                    case 116: bit = (1 << MYKEY_POWER);   break;
                    case 115: bit = (1 << MYKEY_VOLUP);   break;
                    case 114: bit = (1 << MYKEY_VOLDOWN); break;
                    }

                    if(bit){
                        if(ev.value){
                            current_input |= bit;
                        } else{
                            current_input &= ~bit;
                        }
                    }
                }
            }
        }
        SDL_SemPost(pad_sem);
        usleep(1000000 / 60);
    }

    return 0;
}

static int MMIYOO_JoystickInit(void)
{
    event_fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if(event_fd < 0){
        printf("failed to open /dev/input/event0\n");
        return -1;
    }

    if((pad_sem =  SDL_CreateSemaphore(1)) == NULL) {
        return SDL_SetError("Can't create input semaphore");
    }

    running = 1;
    if((thread = SDL_CreateThreadInternal(JoystickUpdate, "MMIYOOInputThread", 4096, NULL)) == NULL) {
        return SDL_SetError("Can't create input thread");
    }
    return 1;
}

static int MMIYOO_JoystickGetCount(void)
{
    return 1;
}

static void MMIYOO_JoystickDetect(void)
{
}

static const char* MMIYOO_JoystickGetDeviceName(int device_index)
{
    return "MMiyoo Joystick";
}

static const char *MMIYOO_JoystickGetDevicePath(int index)
{
    return NULL;
}

static int MMIYOO_JoystickGetDevicePlayerIndex(int device_index)
{
    return -1;
}

static void MMIYOO_JoystickSetDevicePlayerIndex(int device_index, int player_index)
{
}

static SDL_JoystickGUID MMIYOO_JoystickGetDeviceGUID(int device_index)
{
    SDL_JoystickGUID guid;
    const char *name = MMIYOO_JoystickGetDeviceName(device_index);
    SDL_zero(guid);
    SDL_memcpy(&guid, name, SDL_min(sizeof(guid), SDL_strlen(name)));
    return guid;
}

static SDL_JoystickID MMIYOO_JoystickGetDeviceInstanceID(int device_index)
{
    return device_index;
}

static int MMIYOO_JoystickOpen(SDL_Joystick *joystick, int device_index)
{
    joystick->nbuttons = 15;
    joystick->naxes = 0;
    joystick->nhats = 0;
    return 0;
}

static int MMIYOO_JoystickRumble(SDL_Joystick *joystick, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble)
{
    return SDL_Unsupported();
}

static int MMIYOO_JoystickRumbleTriggers(SDL_Joystick *joystick, Uint16 left_rumble, Uint16 right_rumble)
{
    return SDL_Unsupported();
}

static Uint32 MMIYOO_JoystickGetCapabilities(SDL_Joystick *joystick)
{
    return 0;
}

static int MMIYOO_JoystickSetLED(SDL_Joystick *joystick, Uint8 red, Uint8 green, Uint8 blue)
{
    return SDL_Unsupported();
}

static int MMIYOO_JoystickSendEffect(SDL_Joystick *joystick, const void *data, int size)
{
    return SDL_Unsupported();
}

static int MMIYOO_JoystickSetSensorsEnabled(SDL_Joystick *joystick, SDL_bool enabled)
{
    return SDL_Unsupported();
}

static void MMIYOO_JoystickUpdate(SDL_Joystick *joystick)
{
    uint32_t buttons;
    uint32_t changed;
    uint32_t i;
    uint32_t bit;

    SDL_SemWait(pad_sem);
    buttons = current_input;
    SDL_SemPost(pad_sem);

    changed = previous_input ^ buttons;
    previous_input = buttons;

    for(i = 0; i < 15; i++) {
        bit = 1 << i;
        if(changed & bit) {
            SDL_PrivateJoystickButton(joystick, i, (buttons & bit) ? SDL_PRESSED : SDL_RELEASED);
        }
    }
}

static void MMIYOO_JoystickClose(SDL_Joystick *joystick)
{
}

static void MMIYOO_JoystickQuit(void)
{
    running = 0;
    SDL_WaitThread(thread, NULL);
    SDL_DestroySemaphore(pad_sem);
    if(event_fd > 0) {
        close(event_fd);
        event_fd = -1;
    }
}

static SDL_bool MMIYOO_JoystickGetGamepadMapping(int device_index, SDL_GamepadMapping *out)
{
    return SDL_FALSE;
}

SDL_JoystickDriver SDL_MMIYOO_JoystickDriver = {
    MMIYOO_JoystickInit,
    MMIYOO_JoystickGetCount,
    MMIYOO_JoystickDetect,
    MMIYOO_JoystickGetDeviceName,
    MMIYOO_JoystickGetDevicePath,
    MMIYOO_JoystickGetDevicePlayerIndex,
    MMIYOO_JoystickSetDevicePlayerIndex,
    MMIYOO_JoystickGetDeviceGUID,
    MMIYOO_JoystickGetDeviceInstanceID,
    MMIYOO_JoystickOpen,
    MMIYOO_JoystickRumble,
    MMIYOO_JoystickRumbleTriggers,
    MMIYOO_JoystickGetCapabilities,
    MMIYOO_JoystickSetLED,
    MMIYOO_JoystickSendEffect,
    MMIYOO_JoystickSetSensorsEnabled,
    MMIYOO_JoystickUpdate,
    MMIYOO_JoystickClose,
    MMIYOO_JoystickQuit,
    MMIYOO_JoystickGetGamepadMapping
};

#endif


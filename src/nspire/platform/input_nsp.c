/**
 * input_nsp.c — TI-Nspire CX keypad → N64 controller mapping
 *
 * Maps the Nspire's keyboard/keypad to N64 controller inputs.
 * Uses the same approach as sm64-nsp.
 */
#include <stdint.h>
#include <stdbool.h>

#ifdef TARGET_NSP
#include <libndls.h>
#endif

typedef int32_t s32;
typedef uint32_t u32;
typedef int16_t s16;
typedef uint16_t u16;
typedef int8_t s8;
typedef uint8_t u8;

/* N64 Controller Button Masks */
#define CONT_A 0x8000
#define CONT_B 0x4000
#define CONT_G 0x2000 /* Z trigger */
#define CONT_START 0x1000
#define CONT_UP 0x0800
#define CONT_DOWN 0x0400
#define CONT_LEFT 0x0200
#define CONT_RIGHT 0x0100
#define CONT_L 0x0020
#define CONT_R 0x0010
#define CONT_E 0x0008 /* C-Up */
#define CONT_D 0x0004 /* C-Down */
#define CONT_C 0x0002 /* C-Left */
#define CONT_F 0x0001 /* C-Right */

/* N64 Controller State */
typedef struct {
    u16 button;
    s8 stick_x;
    s8 stick_y;
    u8 errnum;
} OSContPad;

/* Current controller state — read by the game each frame */
static OSContPad nsp_controller;

/* Default key mappings — Nspire scancodes */
static struct {
    u32 key;    /* Nspire scancode */
    u16 button; /* N64 button mask */
} key_map[] = {
    { 0x26, CONT_A },     /* L key → A */
    { 0x33, CONT_B },     /* period → B */
    { 0x39, CONT_START }, /* Space/Enter → Start */
    { 0x36, CONT_R },     /* comma → R */
    { 0x25, CONT_G },     /* K → Z */
    { 0x148, CONT_E },    /* Arrow Up → C-Up */
    { 0x150, CONT_D },    /* Arrow Down → C-Down */
    { 0x14B, CONT_C },    /* Arrow Left → C-Left */
    { 0x14D, CONT_F },    /* Arrow Right → C-Right */
    { 0, 0 }              /* sentinel */
};

/* Analog stick via WASD-equivalent keys */
#define STICK_MAG 80 /* Maximum stick magnitude */

static struct {
    u32 key;
    s8 dx;
    s8 dy;
} stick_map[] = {
    { 0x11, 0, STICK_MAG },  /* W → Up */
    { 0x1F, 0, -STICK_MAG }, /* S → Down */
    { 0x1E, -STICK_MAG, 0 }, /* A → Left */
    { 0x20, STICK_MAG, 0 },  /* D → Right */
    { 0, 0, 0 }              /* sentinel */
};

/**
 * Poll the Nspire keypad and update the N64 controller state.
 * Called once per game frame.
 */
void input_nsp_poll(void) {
    nsp_controller.button = 0;
    nsp_controller.stick_x = 0;
    nsp_controller.stick_y = 0;
    nsp_controller.errnum = 0;

#ifdef TARGET_NSP
    /* Poll button keys */
    for (int i = 0; key_map[i].key != 0; i++) {
        if (isKeyPressed(key_map[i].key)) {
            nsp_controller.button |= key_map[i].button;
        }
    }

    /* Poll analog stick keys */
    for (int i = 0; stick_map[i].key != 0; i++) {
        if (isKeyPressed(stick_map[i].key)) {
            nsp_controller.stick_x += stick_map[i].dx;
            nsp_controller.stick_y += stick_map[i].dy;
        }
    }

    /* Clamp stick values */
    if (nsp_controller.stick_x > 80)
        nsp_controller.stick_x = 80;
    if (nsp_controller.stick_x < -80)
        nsp_controller.stick_x = -80;
    if (nsp_controller.stick_y > 80)
        nsp_controller.stick_y = 80;
    if (nsp_controller.stick_y < -80)
        nsp_controller.stick_y = -80;
#endif
}

/**
 * Get current controller state.
 * Called by the game's PadMgr to read input.
 */
OSContPad* input_nsp_get_pad(void) {
    return &nsp_controller;
}

/**
 * Check if the Nspire's ESC key is pressed (for quitting).
 */
bool input_nsp_escape_pressed(void) {
#ifdef TARGET_NSP
    return isKeyPressed(KEY_NSPIRE_ESC);
#else
    return false;
#endif
}

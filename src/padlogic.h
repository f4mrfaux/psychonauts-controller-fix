#ifndef PADLOGIC_H
#define PADLOGIC_H

#define PAD_SLOTS 4
#define PAD_TRIGGER_THRESHOLD 30 /* XINPUT_GAMEPAD_TRIGGER_THRESHOLD */
#define PAD_STICK_ACTIVE 16000   /* well past a worn stick's resting drift */

typedef struct PadState {
    unsigned long packet;
    unsigned short buttons;
    unsigned char left_trigger, right_trigger;
    short lx, ly, rx, ry;
} PadState;

typedef struct PadSample {
    int connected;
    PadState state;
} PadSample;

/* The game reads triggers as pressed at any value above 0. */
void pad_filter_triggers(PadState *state);

/* True when someone is deliberately holding this pad. */
int pad_in_use(const PadState *state);

/* Which physical slot the game's slot 0 should read. Idle virtual pads never
   take over from a pad in use; a pad in use takes over from an idle one. */
int pad_choose_slot(int current, const PadSample samples[PAD_SLOTS]);

#endif

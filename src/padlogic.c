#include "padlogic.h"

void pad_filter_triggers(PadState *state)
{
    if (state->left_trigger < PAD_TRIGGER_THRESHOLD) state->left_trigger = 0;
    if (state->right_trigger < PAD_TRIGGER_THRESHOLD) state->right_trigger = 0;
}

static int stick_active(short value)
{
    return value > PAD_STICK_ACTIVE || value < -PAD_STICK_ACTIVE;
}

int pad_in_use(const PadState *state)
{
    return state->buttons != 0
        || state->left_trigger >= PAD_TRIGGER_THRESHOLD
        || state->right_trigger >= PAD_TRIGGER_THRESHOLD
        || stick_active(state->lx) || stick_active(state->ly)
        || stick_active(state->rx) || stick_active(state->ry);
}

int pad_choose_slot(int current, const PadSample samples[PAD_SLOTS])
{
    int i;
    int current_ok = current >= 0 && current < PAD_SLOTS && samples[current].connected;

    if (current_ok && pad_in_use(&samples[current].state))
        return current;
    for (i = 0; i < PAD_SLOTS; i++)
        if (samples[i].connected && pad_in_use(&samples[i].state))
            return i;
    if (current_ok)
        return current;
    for (i = 0; i < PAD_SLOTS; i++)
        if (samples[i].connected)
            return i;
    return current;
}

#ifndef GAMEPATCH_H
#define GAMEPATCH_H

/* Everything here is specific to the GOG Psychonauts.exe 1.1.500 build. */

/* True when the mapped image is the build these offsets were read from. */
int gamepatch_recognized(const unsigned char *image);

/* Routes the game's key-name lookup through map_key, cdecl int(int). */
int gamepatch_hook_key_lookup(unsigned char *image, int (*map_key)(int));

/* Turns on the game's XInput path; its own check only runs at startup, so a
   pad switched on later would otherwise never be read. */
void gamepatch_enable_xinput(unsigned char *image);

#endif

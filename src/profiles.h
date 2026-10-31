#ifndef PROFILES_H
#define PROFILES_H

/* Gives a profile the Xbox pad bindings unless Jump is already on a Joy
   button. Backs the file up first. Returns 1 if it changed the file. */
int profile_fix_file(const char *ini_path);

/* Runs profile_fix_file over <game_dir>\Profiles\*\*.ini; returns files changed. */
int profile_fix_all(const char *game_dir);

#endif

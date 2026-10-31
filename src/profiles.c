#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "profiles.h"

#define SECTION "Input"
#define BACKUP_SUFFIX ".psychopadx.bak"

static const char *const BINDINGS[][2] = {
    { "MoveForward", "JoyY" },       { "MoveBackward", "JoyY" },
    { "MoveLeft", "JoyX" },          { "MoveRight", "JoyX" },
    { "Jump", "Joy2" },              { "Attack", "Joy1" },
    { "Use", "Joy4" },               { "Cancel", "Joy3" },
    { "LockOn/Float", "Joy5" },      { "PsiPower1", "Joy6" },
    { "PsiPower2", "Joy7" },         { "PsiPower3", "Joy8" },
    { "InventoryMenu", "DPadLeft" }, { "PsiPowersMenu", "DPadRight" },
    { "FirstPerson", "Joy12" },      { "Journal", "Joy9" },
    { "Stats", "Joy11" },
    /* The game wires the right stick to JoyZ/JoyRotZ in its XInput path. */
    { "LookUp", "JoyRotZ" },         { "LookDown", "JoyRotZ" },
    { "LookLeft", "JoyZ" },          { "LookRight", "JoyZ" },
    { "MoveForward_Alt", "DPadUp" }, { "MoveBackward_Alt", "DPadDown" },
    { "MoveLeft_Alt", "" },          { "MoveRight_Alt", "" },
    { "Jump_Alt", "" },              { "Attack_Alt", "" },
    { "Use_Alt", "" },               { "Cancel_Alt", "" },
    { "LockOn/Float_Alt", "" },      { "PsiPower1_Alt", "" },
    { "PsiPower2_Alt", "" },         { "PsiPower3_Alt", "" },
    { "InventoryMenu_Alt", "" },     { "PsiPowersMenu_Alt", "" },
    { "FirstPerson_Alt", "" },       { "Journal_Alt", "" },
    { "Stats_Alt", "" },             { "LookUp_Alt", "" },
    { "LookDown_Alt", "" },          { "LookLeft_Alt", "" },
    { "LookRight_Alt", "" },
};

int profile_fix_file(const char *ini_path)
{
    char jump[64], backup[MAX_PATH];
    size_t i;

    if (!GetPrivateProfileStringA(SECTION, "Jump", "", jump, sizeof(jump), ini_path))
        return 0;
    if (!strncmp(jump, "Joy", 3))
        return 0;
    if (_snprintf_s(backup, sizeof(backup), _TRUNCATE, "%s%s", ini_path, BACKUP_SUFFIX) < 0)
        return 0;
    if (!CopyFileA(ini_path, backup, TRUE) && GetLastError() != ERROR_FILE_EXISTS)
        return 0;
    for (i = 0; i < sizeof(BINDINGS) / sizeof(BINDINGS[0]); i++)
        WritePrivateProfileStringA(SECTION, BINDINGS[i][0], BINDINGS[i][1], ini_path);
    return 1;
}

int profile_fix_all(const char *game_dir)
{
    char pattern[MAX_PATH], sub[MAX_PATH], path[MAX_PATH];
    WIN32_FIND_DATAA dir, file;
    HANDLE dirs, files;
    int changed = 0;

    _snprintf_s(pattern, sizeof(pattern), _TRUNCATE, "%s\\Profiles\\*", game_dir);
    dirs = FindFirstFileA(pattern, &dir);
    if (dirs == INVALID_HANDLE_VALUE)
        return 0;
    do {
        if (!(dir.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || dir.cFileName[0] == '.')
            continue;
        _snprintf_s(sub, sizeof(sub), _TRUNCATE, "%s\\Profiles\\%s\\*.ini", game_dir, dir.cFileName);
        files = FindFirstFileA(sub, &file);
        if (files == INVALID_HANDLE_VALUE)
            continue;
        do {
            _snprintf_s(path, sizeof(path), _TRUNCATE, "%s\\Profiles\\%s\\%s",
                        game_dir, dir.cFileName, file.cFileName);
            changed += profile_fix_file(path);
        } while (FindNextFileA(files, &file));
        FindClose(files);
    } while (FindNextFileA(dirs, &dir));
    FindClose(dirs);
    return changed;
}

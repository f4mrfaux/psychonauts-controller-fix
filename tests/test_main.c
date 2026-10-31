/* usage: test_main.exe <path to xinput1_3.dll> [path to Psychonauts.exe] */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/padlogic.h"
#include "../src/gamepatch.h"
#include "../src/profiles.h"

static int g_failures;

#define CHECK(cond) do { \
    if (cond) { printf("ok    %s\n", #cond); } \
    else { printf("FAIL  %s (line %d)\n", #cond, __LINE__); g_failures++; } \
} while (0)

static void plug_in(PadSample *s, unsigned short buttons)
{
    s->connected = 1;
    s->state.buttons = buttons;
}

static void test_triggers(void)
{
    PadState s;
    ZeroMemory(&s, sizeof(s));
    s.left_trigger = 29;
    s.right_trigger = 30;
    pad_filter_triggers(&s);
    CHECK(s.left_trigger == 0);
    CHECK(s.right_trigger == 30);
}

static void test_in_use(void)
{
    PadState s;
    ZeroMemory(&s, sizeof(s));
    CHECK(!pad_in_use(&s));
    s.lx = 8000;
    CHECK(!pad_in_use(&s));
    s.lx = -20000;
    CHECK(pad_in_use(&s));
    s.lx = 0;
    s.left_trigger = 200;
    CHECK(pad_in_use(&s));
}

static void test_choose_slot(void)
{
    PadSample s[PAD_SLOTS];

    ZeroMemory(s, sizeof(s));
    CHECK(pad_choose_slot(0, s) == 0);               /* nothing connected */

    plug_in(&s[2], 0);
    CHECK(pad_choose_slot(0, s) == 2);               /* only pad is on slot 2 */

    plug_in(&s[0], 0);
    CHECK(pad_choose_slot(2, s) == 2);               /* idle pad doesn't steal */

    s[0].state.buttons = 0x1000;
    CHECK(pad_choose_slot(2, s) == 0);               /* pressed pad takes over */

    s[2].state.buttons = 0x1000;
    CHECK(pad_choose_slot(2, s) == 2);               /* current keeps it on a tie */

    s[2].connected = 0;
    CHECK(pad_choose_slot(2, s) == 0);               /* falls back when unplugged */
}

static void write_text(const char *path, const char *text)
{
    FILE *f;
    fopen_s(&f, path, "w");
    fputs(text, f);
    fclose(f);
}

static void read_binding(const char *ini, const char *key, char *out, DWORD size)
{
    GetPrivateProfileStringA("Input", key, "?", out, size, ini);
}

static void test_profiles(void)
{
    char root[MAX_PATH], dir[MAX_PATH], ini[MAX_PATH], other[MAX_PATH], bak[MAX_PATH], v[64];

    GetTempPathA(MAX_PATH, root);
    strcat_s(root, sizeof(root), "psychopadx_test");
    sprintf_s(dir, sizeof(dir), "%s\\Profiles", root);
    CreateDirectoryA(root, NULL);
    CreateDirectoryA(dir, NULL);
    strcat_s(dir, sizeof(dir), "\\profile 1");
    CreateDirectoryA(dir, NULL);

    sprintf_s(ini, sizeof(ini), "%s\\new.ini", dir);
    sprintf_s(other, sizeof(other), "%s\\custom.ini", dir);
    sprintf_s(bak, sizeof(bak), "%s.psychopadx.bak", ini);
    DeleteFileA(bak);
    write_text(ini, "[Global]\nEnableRumble=true\n\n[Input]\nJump=Space\nAttack=LeftMouse\nJump_Alt=Enter\n");
    write_text(other, "[Input]\nJump=Joy3\nAttack=Joy9\n");

    CHECK(profile_fix_all(root) == 1);

    read_binding(ini, "Jump", v, sizeof(v));
    CHECK(!strcmp(v, "Joy2"));
    read_binding(ini, "LockOn/Float", v, sizeof(v));
    CHECK(!strcmp(v, "Joy5"));
    read_binding(ini, "LookLeft", v, sizeof(v));
    CHECK(!strcmp(v, "JoyZ"));
    read_binding(ini, "Jump_Alt", v, sizeof(v));
    CHECK(!strcmp(v, ""));
    GetPrivateProfileStringA("Global", "EnableRumble", "?", v, sizeof(v), ini);
    CHECK(!strcmp(v, "true"));

    GetPrivateProfileStringA("Input", "Jump", "?", v, sizeof(v), bak);
    CHECK(!strcmp(v, "Space"));                      /* backup kept the original */

    read_binding(other, "Attack", v, sizeof(v));
    CHECK(!strcmp(v, "Joy9"));                       /* pad setups left alone */

    CHECK(profile_fix_all(root) == 0);               /* second run changes nothing */
    GetPrivateProfileStringA("Input", "Jump", "?", v, sizeof(v), bak);
    CHECK(!strcmp(v, "Space"));

    DeleteFileA(bak);
    DeleteFileA(ini);
    DeleteFileA(other);
    RemoveDirectoryA(dir);
    sprintf_s(dir, sizeof(dir), "%s\\Profiles", root);
    RemoveDirectoryA(dir);
    RemoveDirectoryA(root);
}

static int __cdecl fake_map_key(int code)
{
    return code;
}

/* Lays the exe out as the loader would, then patches that copy. */
static void test_patch_real_exe(const char *exe)
{
    HANDLE f = CreateFileA(exe, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    DWORD size, got;
    unsigned char *file, *image;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    WORD i;
    LONG rel;

    if (f == INVALID_HANDLE_VALUE) {
        printf("skip  real exe test: cannot open %s\n", exe);
        return;
    }
    size = GetFileSize(f, NULL);
    file = (unsigned char *)malloc(size);
    ReadFile(f, file, size, &got, NULL);
    CloseHandle(f);

    nt = (IMAGE_NT_HEADERS *)(file + ((IMAGE_DOS_HEADER *)file)->e_lfanew);
    image = (unsigned char *)VirtualAlloc(NULL, nt->OptionalHeader.SizeOfImage,
                                          MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    memcpy(image, file, nt->OptionalHeader.SizeOfHeaders);
    sec = IMAGE_FIRST_SECTION(nt);
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
        memcpy(image + sec->VirtualAddress, file + sec->PointerToRawData, sec->SizeOfRawData);

    CHECK(gamepatch_recognized(image));
    CHECK(image[0x331060] == 0 || image[0x331060] == 1);
    printf("info  stock XInput-allowed byte = %d, detected byte = %d\n", image[0x331060], image[0x337230]);

    CHECK(gamepatch_hook_key_lookup(image, fake_map_key));
    memcpy(&rel, image + 0x5b66, sizeof(rel));
    CHECK(image[0x5b65] == 0xE8);
    CHECK(image + 0x5b6a + rel == (unsigned char *)fake_map_key);

    gamepatch_enable_xinput(image);
    CHECK(image[0x337230] == 1 && image[0x331060] == 1);

    image[0x62bc] = 0x90;
    CHECK(!gamepatch_recognized(image));             /* other builds refused */

    VirtualFree(image, 0, MEM_RELEASE);
    free(file);
}

typedef DWORD (WINAPI *GetStateFn)(DWORD, PadState *);

static void test_dll(const char *dll)
{
    HMODULE m = LoadLibraryA(dll);
    GetStateFn by_ordinal, by_name;
    PadState s;
    DWORD r;

    CHECK(m != NULL);
    if (!m)
        return;
    by_ordinal = (GetStateFn)GetProcAddress(m, MAKEINTRESOURCEA(2));
    by_name = (GetStateFn)GetProcAddress(m, "XInputGetState");
    CHECK(by_ordinal != NULL && by_ordinal == by_name);
    CHECK(GetProcAddress(m, MAKEINTRESOURCEA(3)) != NULL);     /* the game imports 2 and 3 by ordinal */
    r = by_ordinal(0, &s);
    CHECK(r == ERROR_SUCCESS || r == ERROR_DEVICE_NOT_CONNECTED);
    printf("info  XInputGetState(0) = %lu\n", r);
}

int main(int argc, char **argv)
{
    test_triggers();
    test_in_use();
    test_choose_slot();
    test_profiles();
    if (argc > 2)
        test_patch_real_exe(argv[2]);
    if (argc > 1)
        test_dll(argv[1]);
    printf(g_failures ? "%d FAILED\n" : "all passed\n", g_failures);
    return g_failures ? 1 : 0;
}

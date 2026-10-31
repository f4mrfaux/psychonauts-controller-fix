/* Stands in for xinput1_3.dll next to Psychonauts.exe and forwards to the real
   XInput, fixing controller handling on the way through. */
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "padlogic.h"
#include "gamepatch.h"
#include "profiles.h"

#define MONITOR_INTERVAL_MS 100
#define DISCONNECTED_POLL_TICKS 10

typedef DWORD (WINAPI *GetStateFn)(DWORD, PadState *);
typedef DWORD (WINAPI *SetStateFn)(DWORD, void *);
typedef DWORD (WINAPI *GetCapabilitiesFn)(DWORD, DWORD, void *);
typedef void (WINAPI *EnableFn)(BOOL);

static INIT_ONCE g_real_once = INIT_ONCE_STATIC_INIT;
static GetStateFn g_get_state;
static SetStateFn g_set_state;
static GetCapabilitiesFn g_get_capabilities;
static EnableFn g_enable;
static volatile LONG g_active_slot;
static unsigned char *g_image;
static int g_game_recognized;
static char g_log_path[MAX_PATH];

static void log_line(const char *fmt, ...)
{
    FILE *f;
    va_list args;

    if (fopen_s(&f, g_log_path, "a"))
        return;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fputc('\n', f);
    fclose(f);
}

#ifdef PSX_TEST_PAD
#include "../tests/fake_pad.inc"
#endif

static BOOL CALLBACK load_real(PINIT_ONCE once, PVOID param, PVOID *context)
{
    static const char *const NAMES[] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
    char path[MAX_PATH];
    UINT len = GetSystemDirectoryA(path, MAX_PATH);
    HMODULE real = NULL;
    int i;

    (void)once; (void)param; (void)context;
#ifdef PSX_TEST_PAD
    {
        char game_dir[MAX_PATH];
        strcpy_s(game_dir, sizeof(game_dir), g_log_path);
        *strrchr(game_dir, '\\') = '\0';
        load_fake(game_dir);
        return TRUE;
    }
#endif
    for (i = 0; i < 3 && !real && len; i++) {
        char full[MAX_PATH];
        _snprintf_s(full, sizeof(full), _TRUNCATE, "%s\\%s", path, NAMES[i]);
        real = LoadLibraryA(full);
        if (real)
            log_line("real XInput: %s", full);
    }
    if (!real) {
        log_line("no system XInput found");
        return TRUE;
    }
    g_set_state = (SetStateFn)GetProcAddress(real, "XInputSetState");
    g_get_capabilities = (GetCapabilitiesFn)GetProcAddress(real, "XInputGetCapabilities");
    g_enable = (EnableFn)GetProcAddress(real, "XInputEnable");
    g_get_state = (GetStateFn)GetProcAddress(real, "XInputGetState");
    return TRUE;
}

static void ensure_real(void)
{
    InitOnceExecuteOnce(&g_real_once, load_real, NULL, NULL);
}

static DWORD map_user(DWORD user)
{
    return user == 0 ? (DWORD)g_active_slot : user;
}

DWORD WINAPI XInputGetState(DWORD user, PadState *state)
{
    DWORD result;

    ensure_real();
    if (!g_get_state)
        return ERROR_DEVICE_NOT_CONNECTED;
    result = g_get_state(map_user(user), state);
    if (result == ERROR_SUCCESS)
        pad_filter_triggers(state);
    return result;
}

DWORD WINAPI XInputSetState(DWORD user, void *vibration)
{
    ensure_real();
    return g_set_state ? g_set_state(map_user(user), vibration) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetCapabilities(DWORD user, DWORD flags, void *capabilities)
{
    ensure_real();
    return g_get_capabilities ? g_get_capabilities(map_user(user), flags, capabilities)
                              : ERROR_DEVICE_NOT_CONNECTED;
}

void WINAPI XInputEnable(BOOL enable)
{
    ensure_real();
    if (g_enable)
        g_enable(enable);
}

static int __cdecl map_key(int code)
{
    /* Gamepad codes start at 0x100. They aren't scancodes, and Proton's answer
       for them garbles button names, which is what breaks prompts and bindings. */
    if (code >= 0x100)
        return 0;
    return (int)MapVirtualKeyExA((UINT)code, MAPVK_VSC_TO_VK, GetKeyboardLayout(0));
}

static DWORD WINAPI monitor_thread(LPVOID unused)
{
    PadSample samples[PAD_SLOTS];
    unsigned tick = 0;
    int xinput_enabled = 0;

    (void)unused;
    ZeroMemory(samples, sizeof(samples));
    ensure_real();
    if (!g_get_state)
        return 0;
    for (;; tick++, Sleep(MONITOR_INTERVAL_MS)) {
        int i, any = 0, current = (int)g_active_slot, chosen;

        for (i = 0; i < PAD_SLOTS; i++) {
            /* Polling an empty slot is slow, so only look there now and then. */
            if (samples[i].connected || tick % DISCONNECTED_POLL_TICKS == 0)
                samples[i].connected = g_get_state(i, &samples[i].state) == ERROR_SUCCESS;
            any |= samples[i].connected;
        }
        if (any && g_game_recognized && !xinput_enabled) {
            gamepatch_enable_xinput(g_image);
            xinput_enabled = 1;
            log_line("controller found, game XInput path on");
        }
        chosen = pad_choose_slot(current, samples);
        if (chosen != current) {
            InterlockedExchange(&g_active_slot, chosen);
            log_line("reading XInput slot %d", chosen);
        }
    }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    char game_dir[MAX_PATH];
    char *slash;
    HANDLE thread;

    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;
    DisableThreadLibraryCalls(instance);

    GetModuleFileNameA(NULL, game_dir, MAX_PATH);
    slash = strrchr(game_dir, '\\');
    if (slash)
        *slash = '\0';
    _snprintf_s(g_log_path, sizeof(g_log_path), _TRUNCATE, "%s\\psychopadx.log", game_dir);
    DeleteFileA(g_log_path);

    g_image = (unsigned char *)GetModuleHandleA(NULL);
    g_game_recognized = gamepatch_recognized(g_image);
    if (g_game_recognized) {
        log_line("recognized GOG 1.1.500; key lookup hook %s",
                 gamepatch_hook_key_lookup(g_image, map_key) ? "on" : "FAILED");
        log_line("profiles updated: %d", profile_fix_all(game_dir));
    } else {
        log_line("exe not recognized: game code left alone, slot and trigger fixes only");
    }

    thread = CreateThread(NULL, 0, monitor_thread, NULL, 0, NULL);
    if (thread)
        CloseHandle(thread);
    return TRUE;
}

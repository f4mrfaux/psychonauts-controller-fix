#include <windows.h>
#include <string.h>
#include "gamepatch.h"

#define BUILD_TIMESTAMP 1337730827u

#define RVA_KEY_LOOKUP_CALL 0x5b65  /* call 0x405bb0 inside 0x405b50 */
#define RVA_XINPUT_GATE     0x62bc  /* movzx ecx, [0x737230] inside 0x4062a0 */
#define RVA_XINPUT_DETECTED 0x337230
#define RVA_XINPUT_ALLOWED  0x331060

static const unsigned char SIG_KEY_LOOKUP_CALL[] = { 0xE8, 0x46, 0x00, 0x00, 0x00 };
static const unsigned char SIG_XINPUT_GATE[] = { 0x0F, 0xB6, 0x0D, 0x30, 0x72, 0x73, 0x00 };

int gamepatch_recognized(const unsigned char *image)
{
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)image;
    const IMAGE_NT_HEADERS *nt;

    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;
    nt = (const IMAGE_NT_HEADERS *)(image + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.TimeDateStamp != BUILD_TIMESTAMP)
        return 0;
    return !memcmp(image + RVA_KEY_LOOKUP_CALL, SIG_KEY_LOOKUP_CALL, sizeof(SIG_KEY_LOOKUP_CALL))
        && !memcmp(image + RVA_XINPUT_GATE, SIG_XINPUT_GATE, sizeof(SIG_XINPUT_GATE));
}

int gamepatch_hook_key_lookup(unsigned char *image, int (*map_key)(int))
{
    unsigned char *call = image + RVA_KEY_LOOKUP_CALL;
    LONG rel = (LONG)((unsigned char *)map_key - (call + 5));
    DWORD old;

    if (!VirtualProtect(call, 5, PAGE_EXECUTE_READWRITE, &old))
        return 0;
    memcpy(call + 1, &rel, sizeof(rel));
    VirtualProtect(call, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), call, 5);
    return 1;
}

void gamepatch_enable_xinput(unsigned char *image)
{
    *(volatile unsigned char *)(image + RVA_XINPUT_DETECTED) = 1;
    *(volatile unsigned char *)(image + RVA_XINPUT_ALLOWED) = 1;
}

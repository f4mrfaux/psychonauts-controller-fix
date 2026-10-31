/* Test builds only: a pad on slot 2 scripted through a text file, so the game
   can be driven without a controller. Line format: buttons lt rt lx ly rx ry,
   all decimal except hex buttons. No file means no pad connected. */
static char g_fake_path[MAX_PATH];
static volatile LONG g_fake_polls;

static DWORD WINAPI fake_get_state(DWORD user, PadState *state)
{
    FILE *f;
    unsigned buttons = 0;
    int lt = 0, rt = 0, lx = 0, ly = 0, rx = 0, ry = 0;
    LONG polls = InterlockedIncrement(&g_fake_polls);

    if (polls == 2000)
        log_line("fake pad: 2000 polls seen");
    if (user != 2 || fopen_s(&f, g_fake_path, "r"))
        return ERROR_DEVICE_NOT_CONNECTED;
    fscanf_s(f, "%x %d %d %d %d %d %d", &buttons, &lt, &rt, &lx, &ly, &rx, &ry);
    fclose(f);
    ZeroMemory(state, sizeof(*state));
    state->buttons = (unsigned short)buttons;
    state->left_trigger = (unsigned char)lt;
    state->right_trigger = (unsigned char)rt;
    state->lx = (short)lx;
    state->ly = (short)ly;
    state->rx = (short)rx;
    state->ry = (short)ry;
    return ERROR_SUCCESS;
}

static DWORD WINAPI fake_set_state(DWORD user, void *vibration)
{
    (void)vibration;
    return user == 2 ? ERROR_SUCCESS : ERROR_DEVICE_NOT_CONNECTED;
}

static void load_fake(const char *game_dir)
{
    _snprintf_s(g_fake_path, sizeof(g_fake_path), _TRUNCATE, "%s\\psychopadx_testpad.txt", game_dir);
    g_get_state = fake_get_state;
    g_set_state = fake_set_state;
    log_line("TEST BUILD: fake pad on slot 2");
}

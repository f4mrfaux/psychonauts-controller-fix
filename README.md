# PsychoPad X: Psychonauts 1 Xbox Controller Fix for PC (GOG)

Fixes controller problems in the PC version of the original Psychonauts (2005) on Windows: turn on an Xbox-style controller, launch the game, and get Xbox button prompts and working controls. It's for the first game, not Psychonauts 2.

It's a small `xinput1_3.dll` that sits beside `Psychonauts.exe` and forwards to the system's XInput, fixing controller handling on the way through. It's a modern follow-up to the PsychoPad mod.

## Problems this addresses

- Psychonauts controller or gamepad not detected, or ignored because another device holds XInput slot 0
- Xbox controller not working if it's switched on after launching the game
- On-screen button prompts showing keyboard keys or mouse icons instead of Xbox buttons (with PsychoPad's icons installed)
- Wrong or identical button icons for L3 and R3, the stick clicks
- Triggers firing by themselves from trigger noise
- Controller bindings lost or garbled after quitting and restarting, the bug psycho_fix traced to `MapVirtualKeyEx` (this one is expected to help under Proton, but isn't tested there yet)

**Which version?** Built for the GOG release, exe 1.1.500. On other builds, such as Steam, only the slot and trigger fixes run, and that hasn't been tested. On Steam Deck or Linux with the Steam version, try [psycho_fix](https://github.com/jdaniellee/psycho_fix) first.

## Credit

This project builds directly on other people's work.

- **Voodooman (v00d00m4n), [PsychoPad](https://www.gog.com/forum/psychonauts/psychopad_xbox_360_controller_gui_unofficial_patch), 2011-12.** The hand-drawn Xbox icons, the icon remapping, the Xbox binding layout and the localized prompt text are all theirs. PsychoPad X's default bindings are the ones PsychoPad worked out. PsychoPad remains a requirement, and its files aren't redistributed here.
- **jdaniellee, [psycho_fix](https://github.com/jdaniellee/psycho_fix), 2026.** Identified that gamepad codes passing through `GetKeyboardLayout()`/`MapVirtualKeyEx()` get mangled under Proton, breaking prompts and saved bindings. Our key-lookup fix targets the code they found.
- **Horst Schaeffer, INIfile.** The tool PsychoPad uses to write profile settings, shipped there as `PsychoIni.dll`. Credited because it was part of understanding how PsychoPad works; PsychoPad X doesn't use it.
- **Double Fine.** Psychonauts, including the XInput support this relies on.

Neither project's code was copied. PsychoPad was studied by reading its data files and unpacking its installer, without running its packed patcher. PsychoPad X isn't affiliated with or endorsed by any of the above.

## What changed, and why

PsychoPad is a one-time patcher for game data. psycho_fix is a one-byte patch for Proton. The aim here was different: nothing to set up, on the machines and controllers we actually use.

| Change | Why |
|---|---|
| Reads whichever XInput slot has your pad | The game only ever polls slot 0. Virtual gamepad drivers and streaming tools can occupy that slot and leave your real pad ignored. A pad in use takes over from an idle one. |
| Turns on the game's XInput path once any pad appears | The game only checks for an XInput device at startup, so a pad switched on later would never be read. |
| Ignores small trigger values | The game counts any value above 0 as a press, so trigger noise registers as input. Values under the standard threshold of 30 now read as 0. |
| Skips `MapVirtualKeyEx` for gamepad codes only | psycho_fix's jump flip turns the lookup off for every key on any modern Windows, which can misread keys on non-English layouts. Filtering on code value (`>= 0x100`) fixes gamepads and leaves keyboards alone. |
| Applies Xbox bindings at startup to profiles that lack them | PsychoPad had to be re-run for each new profile. A profile whose Jump isn't on a Joy button gets the bindings, and a `.psychopadx.bak` of the original. Custom pad setups are left alone. |
| Separate L3 and R3 icons | PsychoPad maps both to one generic stick icon, even though its pack includes left and right stick art. `install.ps1` points them at that art. |
| A readable DLL and scripts instead of a packed patcher | Open source, no installer, no admin rights for the mod itself, and a clean uninstall. |
| Refuses unknown builds | The DLL checks the exe's timestamp and code bytes before touching game code. On anything else, only the slot and trigger fixes run, and the log says so. psycho_fix does nothing silently if its pattern isn't found. |

Kept as it was: PsychoPad's camera bindings. `JoyZ`/`JoyRotZ` look odd for an Xbox right stick, but the game's XInput path wires the right stick straight into those fields, so they're correct.

Not attempted: real DirectInput support, PlayStation glyphs, analog triggers.

## Status

Tested on Windows 11 with the GOG build, using a scripted fake pad on slot 2 (`tests/smoke.py`): buttons, triggers and D-pad reached the game's input state, trigger noise was ignored, and Xbox prompts showed with the character moving and jumping.

Not yet tested: a physical controller, the right stick, the L3/R3 icons on screen, bindings across a relaunch, and Proton. The mangling explanation comes from psycho_fix and hasn't been reproduced here.

## Requirements

- The GOG build of Psychonauts, exe 1.1.500
- PsychoPad Lite v1.1 applied, for its `XI_Icons` art and prompt text. That art has no license statement, so it isn't included here.

## Use

Build with Visual Studio 2022's C tools, then test and install:

    cmd /c .\build.cmd
    .\build\test_main.exe "$PWD\build\xinput1_3.dll" "<game folder>\Psychonauts.exe"
    .\install.ps1 -GameDir "<game folder>"

Remove it with `.\uninstall.ps1 -GameDir "<game folder>"`. Profiles the DLL changed keep their `.psychopadx.bak`.

Under Proton, set the launch option `WINEDLLOVERRIDES="xinput1_3=n,b" %command%`.

The log is `psychopadx.log` in the game folder.

`build\testpad\xinput1_3.dll` is a test build with a fake pad scripted through a text file. Don't play with it installed.

## How the game reads controllers

Addresses are for this exe, image base `0x400000`.

- `0x4062a0` polls `XInputGetState(0, ...)` and turns buttons into internal codes: X `0x118`, A `0x119`, B `0x11a`, Y `0x11b`, LT `0x11c`, RT `0x11d`, RB `0x11e`, LB `0x11f`, Start `0x120`, Back `0x121`, L3 `0x122`, R3 `0x123`, D-pad `0x114`-`0x117`.
- `0x4065b0` reads the sticks into a fixed struct.
- The XInput path only runs when `0x737230` (device found at startup) and `0x731060` (allowed) are set; otherwise the game uses DirectInput.
- `0x405b50` resolves a code to a name, calling `0x405bb0` (`MapVirtualKeyExA`) first. The flag it checks, `0x731061`, only means "NT-family Windows", so it's effectively always on.
- These codes line up with PsychoPad's `Joy1`-`Joy12` order: `Joy1` is X, `Joy2` is A, and so on.

## License

MIT for the code in this repository, see `LICENSE`. It doesn't cover PsychoPad, psycho_fix or Psychonauts.

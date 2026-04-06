# Breeze Integration with SwitchU

## Overview

This modification allows the Home button to toggle between a running game and **Breeze** (a cheat/memory editing tool by Tomvita) instead of the SwitchU menu. Breeze runs as an NRO in the Album (hbmenu) library applet slot.

## Architecture

### Constraints

- The Nintendo Switch OS only allows **one library applet** at a time.
- The SwitchU menu occupies the eShop library applet slot (`AppletId_LibraryAppletShop`).
- Breeze runs inside the Album library applet slot (`AppletId_LibraryAppletPhotoViewer`) via Atmosphere's hbmenu override.
- The SwitchU menu must be terminated before Breeze can launch, and vice versa.

### Communication Mechanism

Since Breeze (an NRO) has no IPC access to the SwitchU daemon, communication uses **flag files on the SD card**:

| Flag File | Purpose |
|---|---|
| `sdmc:/config/SwitchU/breeze_active` | When present, Home button launches Breeze instead of SwitchU menu |
| `sdmc:/config/SwitchU/breeze_goto_game` | When present after Breeze exits, daemon resumes the game instead of relaunching the menu |

Constants are defined in `projects/common/include/switchu/smi_protocol.hpp`:
```cpp
static constexpr const char* kBreezeActiveFlag   = "sdmc:/config/SwitchU/breeze_active";
static constexpr const char* kBreezeGotoGameFlag = "sdmc:/config/SwitchU/breeze_goto_game";
```

## Changes

### SwitchU Daemon (`projects/daemon/src/main.cpp`)

#### Startup Cleanup
Both flag files are removed in `__appInit()` after the SD card is mounted, preventing stale flags from a previous session from causing unexpected behavior.

#### Persistent Breeze Session (`g_breezeAlbumActive`)
A `g_breezeAlbumActive` flag tracks whether the Album applet (hosting Breeze) is currently alive. This enables **foreground toggling** — the Album is launched once and kept alive across Home presses, rather than being destroyed and recreated each time.

#### Home Button Handlers (SAMS msg=2 and AE msg=20)
When the game has foreground and the user presses Home:

1. **`breeze_active` flag exists + `g_breezeAlbumActive` is true:**
   The game is suspended and the daemon takes foreground. Since Album (Breeze) is already running in the background, it becomes visible immediately. No applet relaunch needed — instant toggle.

2. **`breeze_active` flag exists + `g_breezeAlbumActive` is false (first time):**
   The SwitchU menu is terminated via `daemon::menu_la::terminate()`. Album is launched via the blocking `launchLibraryApplet()` call. `g_breezeAlbumActive` is set to `true` for the duration.

3. **`breeze_active` flag does not exist:**
   Normal SwitchU menu behavior (wake suspended menu or launch fresh).

#### Spin Loop in `launchLibraryApplet()`
The blocking spin loop that runs while a library applet is active was modified with two new checks:

- **Home press (`g_pendingForegroundAppletHome`):** If `g_breezeAlbumActive` is true, `breeze_active` exists, and a game is running, the game is **resumed** instead of Album being killed. This keeps Breeze alive in the background for instant toggling on the next Home press.

- **`breeze_goto_game` flag polling:** The spin loop checks for this flag every ~10ms. When Breeze creates it (via "Goto Game") and exits to hbmenu, the daemon detects it almost immediately and auto-kills Album, bypassing hbmenu entirely. The post-exit logic then resumes the game.

#### Post-Album Exit Logic
After Album exits (either from the spin loop or `checkFinished` block), a 3-way decision determines what happens next:

1. `breeze_goto_game` exists → remove flag, resume game
2. `breeze_active` exists + game running → resume game (Home toggle, Breeze was killed by Exit or other means)
3. Neither → relaunch SwitchU menu (user pressed "Exit" in Breeze, which removed `breeze_active`)

#### Forward Declaration
`launchLibraryApplet()` is forward-declared before `handleGeneralChannel()` since the Breeze integration code calls it from within the Home handlers, which are defined earlier in the file.

#### Added Includes
`<cstdio>` and `<unistd.h>` were added for `access()`, `remove()`, `fopen()`, `fclose()`.

### Breeze (`C:\GitHub\Breeze\source\action.cpp`)

#### New Enum Value
```cpp
GotoGame = 48,
```
Added to `Main_menu_action_id_t` after `UnrealMenu = 47`.

#### New Button in Main Menu
```cpp
{"Goto Game", Main_menu_ID GotoGame, HidNpadButton_Verification, m_HasCheatProcess},
```
Added to `init_Main_menu()` after the "Exit" button. Only enabled when a cheat process is attached (`m_HasCheatProcess`), since toggling to a game only makes sense when one is running.

#### "Goto Game" Handler
```cpp
case Main_menu_ID GotoGame: {
    FILE* f1 = fopen("sdmc:/config/SwitchU/breeze_active", "w");
    if (f1) fclose(f1);
    FILE* f2 = fopen("sdmc:/config/SwitchU/breeze_goto_game", "w");
    if (f2) fclose(f2);
    request_exit();
    return;
}
```
Creates both flag files, then exits Breeze. The daemon detects `breeze_goto_game` in its spin loop and auto-kills Album, resuming the game. On subsequent Home presses, `breeze_active` causes the daemon to launch Breeze again.

#### Modified "Exit" Handler
```cpp
case 1:
    remove("sdmc:/config/SwitchU/breeze_active");
    request_exit();
    return;
```
Removes `breeze_active` before exiting, so the next Home press reverts to normal SwitchU menu behavior.

### SwitchU Common (`projects/common/include/switchu/smi_protocol.hpp`)
Added the two flag file path constants referenced above.

### Build System (`toolchain/devkitA64.lua`)
Added devkitPro portlibs include and library paths to resolve missing headers/libraries during compilation.

### NPDM Configs (`projects/daemon/daemon.json`, `projects/menu/menu.json`)
Added missing fields required by newer versions of `npdmtool` (hex string format for certain fields, `force_debug_prod` inside `debug_flags` kernel capability).

## User-Facing Behavior

### Activating Breeze Mode
1. From the SwitchU menu, open Album (which loads hbmenu, then Breeze)
2. In Breeze, press **"Goto Game"**
3. The game resumes. From this point on, Home toggles between the game and Breeze.

### Toggling (after activation)
- **Home from game** → Breeze appears instantly (no reload)
- **Home from Breeze** → game resumes instantly

### Deactivating Breeze Mode
- In Breeze, press **"Exit"**
- This removes the `breeze_active` flag
- Next Home press returns to the SwitchU menu

### Reboot
- Flag files are cleaned up on daemon startup, so Breeze mode does not persist across reboots.

## Files Modified

| File | Repository | Change |
|---|---|---|
| `projects/daemon/src/main.cpp` | SwitchU | Home handlers, spin loop, startup cleanup, forward decl, includes |
| `projects/common/include/switchu/smi_protocol.hpp` | SwitchU | Flag file constants |
| `projects/daemon/daemon.json` | SwitchU | NPDM fields |
| `projects/menu/menu.json` | SwitchU | NPDM fields |
| `projects/menu/src/core/WiiUMenuApp.cpp` | SwitchU | Home toggle logic |
| `toolchain/devkitA64.lua` | SwitchU | Portlibs paths |
| `source/action.cpp` | Breeze | GotoGame enum, button, handler; Exit handler |

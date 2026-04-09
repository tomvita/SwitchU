# SwitchU Daemon Logic

## Architecture

```
+------------------+     SMI protocol      +------------------+
|     Daemon       | <-------------------> |   SwitchU Menu   |
| (SystemApplet)   |   interactive storage |  (LibraryApplet  |
| title 0x...1000  |                       |   eShop slot     |
+------------------+                       |   0x...100B)     |
        |                                  +------------------+
        |  appletApplication*() APIs
        v
+------------------+
|   Game process   |
| (Application)    |
+------------------+
```

The daemon runs as `AppletType_SystemApplet` (replacing qlaunch, title ID `0100000000001000`). It owns all foreground orchestration. The menu runs in the eShop's library applet slot (`AppletId_LibraryAppletShop`, title ID `010000000000100B`). They communicate via applet interactive storage using the SMI (SwitchU Menu Interface) protocol.

Third-party apps like hbmenu can also occupy the eShop library applet slot. These don't speak SMI, so the daemon handles them differently (see `g_smiMenuReady`).

## Source Files

| File | Role |
|---|---|
| `src/main.cpp` | Main loop, Home handling, message dispatch, foreground applet management |
| `src/app_manager.hpp` | Game process lifecycle (launch, resume, suspend, terminate) |
| `src/menu_launcher.hpp` | Menu library applet lifecycle (launch, suspend, wake, terminate) |
| `src/ipc_server.hpp` | IPC server for external communication |

## State Variables

### `main.cpp` globals

| Variable | Type | Purpose |
|---|---|---|
| `g_pendingLaunch` | bool | Menu requested a game launch; waiting for menu to exit/suspend |
| `g_pendingTitleId` | u64 | Title ID for pending launch |
| `g_pendingUid` | AccountUid | User for pending launch |
| `g_pendingResume` | bool | Menu requested game resume; waiting for menu to exit/suspend |
| `g_pendingAlbum` | bool | Menu requested Album launch; waiting for menu to exit |
| `g_pendingMiiEditor` | bool | Menu requested Mii Editor; waiting for menu to exit |
| `g_pendingNetConnect` | bool | Menu requested NetConnect; waiting for menu to exit |
| `g_foregroundAppletActive` | bool | A foreground applet (Album/Breeze/etc) is running inside the `launchLibraryApplet()` blocking loop |
| `g_pendingForegroundAppletHome` | bool | Home was pressed; signal the blocking loop to close the foreground applet |
| `g_breezeAlbumActive` | bool | The current foreground applet is specifically Breeze/Album |
| `g_smiMenuReady` | bool | Current `menu_la` session sent `MenuReady` (cmd 40). True = SwitchU menu, false = hbmenu or other non-SMI app |

### `daemon::app` (app_manager.hpp)

| Variable | Purpose |
|---|---|
| `g_running` | A game process is alive |
| `g_hasForeground` | The game currently has foreground (false when suspended by Home) |
| `g_suspendedTitleId` | Title ID of the running game (set at launch, cleared at exit) |

### `daemon::menu_la` (menu_launcher.hpp)

| Variable | Purpose |
|---|---|
| `g_active` | A library applet was launched in the eShop slot and hasn't exited |
| `g_suspended` | The menu/applet is in keep-alive suspend (rendering off, polling for wake signal) |

---

## Use Cases

### 1. Normal Game Launch from SwitchU Menu

**Precondition:** SwitchU menu is active and showing (`menu_la::isActive()=true`, `g_smiMenuReady=true`).

1. User selects a game icon in the menu.
2. Menu sends `LaunchApplication` (cmd 1) with title_id + user_uid.
   - Daemon sets `g_pendingLaunch=true`, `g_pendingTitleId`, `g_pendingUid`.
3. Menu sends `MenuSuspending` (cmd 42).
   - Daemon sets `menu_la::setSuspended(true)`.
   - Daemon sees `g_pendingLaunch=true`.
   - Daemon calls `app::launch(titleId, uid)`:
     - Creates Application, pushes user param, starts it, requests foreground.
     - Sets `g_running=true`, `g_hasForeground=true`, `g_suspendedTitleId=titleId`.
4. Game runs with foreground. Menu is alive but suspended (keep-alive).

**State after:** `app: running=T, hasFG=T` / `menu_la: active=T, suspended=T` / `g_smiMenuReady=T`

---

### 2. Home Toggle (Game <-> SwitchU Menu)

#### 2a. Home: Game -> Menu

**Precondition:** Game running with foreground, SwitchU menu suspended in keep-alive.

Home press arrives via SAMS general channel (case 2) or applet message 20. Both follow the same logic:

1. `app::onHomeSuspend()` -> `g_hasForeground=false`
2. `appletRequestToGetForeground()` -> daemon takes foreground
3. Breeze check: no breeze flag -> skip
4. `menu_la::isActive()=true` AND `g_smiMenuReady=true`:
   - Push `ApplicationSuspended` notification with `suspendedTitleId` -> menu receives it, updates internal state (`m_appRunning=true`, `m_suspendedTitleId=tid`).
   - `menu_la::isSuspended()=true` -> push wake signal (reason=0, tid) -> menu wakes from suspend loop, reads reason, updates icons with play indicator. Daemon sets `setSuspended(false)`.
5. Menu is now showing with the game icon marked as suspended.

**State after:** `app: running=T, hasFG=F` / `menu_la: active=T, suspended=F` / `g_smiMenuReady=T`

#### 2b. Home: Menu -> Game (Resume)

**Precondition:** Game suspended, SwitchU menu active and showing.

1. Home press arrives.
2. `app::isRunning()=true` but `app::hasForeground()=false` -> skip first branch.
3. Falls to `else if (menu_la::isActive())`.
4. `g_smiMenuReady=true` -> push `HomeRequest` notification.
5. Menu receives `HomeRequest`:
   - Detects `isAppRunning() && suspendedTitleId()!=0`.
   - Calls `resumeApplication()`:
     - Sends `ResumeApplication` (cmd 2) -> daemon sets `g_pendingResume=true`.
     - Calls `suspendForApp()` -> stops audio, disables rendering.
     - Sends `MenuSuspending` (cmd 42) -> daemon sets `setSuspended(true)`.
       - Daemon sees `g_pendingResume=true`.
       - Calls `app::resume()`: `appletUnlockForeground()`, `appletApplicationRequestForApplicationToGetForeground()`, `g_hasForeground=true`.
6. Game resumes with foreground. Menu back to keep-alive suspend.

**State after:** Same as end of Use Case 1.

---

### 3. External Launch via msg 50 (sphaira / hbmenu)

**Trigger:** Another application calls `appletRequestLaunchApplication(app_id)`. AM sends AppletMessage 50 to the System Applet.

#### 3a. From Album (sphaira launched via Album)

**Precondition:** SwitchU menu exited (sent MenuClosing). Daemon is inside `launchLibraryApplet()` blocking loop running Album. `g_foregroundAppletActive=true`, `menu_la::isActive()=false`.

**Launch:**

1. msg 50 fires (processed by `handleAppletMessages()` inside the blocking loop).
2. `app::launchRequested()`:
   - Resolves user (`accountTrySelectUserWithoutInteraction`, fallback to first registered).
   - `appletPopLaunchRequestedApplication()` -> gets app handle + title_id.
   - Ensures save data, pushes user param, starts app, requests foreground.
   - `g_running=true`, `g_hasForeground=true`, `g_suspendedTitleId=titleId`.
3. `menu_la::isActive()=false` -> skip `setSuspended`.
4. Game launches. Album is still running in blocking loop.

**First Home press (game -> close Album -> relaunch menu):**

1. msg 20 fires inside the blocking loop.
2. `app::isRunning()=true`, `app::hasForeground()=true` -> `onHomeSuspend()`.
3. `g_foregroundAppletActive=true` -> sets `g_pendingForegroundAppletHome=true`.
4. Blocking loop sees the flag -> closes Album (`appletHolderRequestExitOrTerminate`).
5. Blocking loop exits, returns to the caller in mainLoop (the post-Album path).
6. Post-Album logic: no Breeze flags -> "relaunching menu after album".
7. `menu_la::launch(MainMenu, buildSystemStatus())`.
   - `buildSystemStatus()` returns `{suspended_app_id=titleId, app_running=true}`.
8. Menu starts, receives `SystemStatus` via `setStartupStatus(titleId, true)` -> knows the game is suspended, shows it with play indicator.

**Subsequent Home presses:** Follow Use Case 2 (normal toggle).

#### 3b. From hbmenu-as-eShop

**Precondition:** hbmenu is running as the library applet in eShop slot. `menu_la::isActive()=true`, `g_smiMenuReady=false` (hbmenu never sends MenuReady).

**Launch:**

1. msg 50 fires.
2. `app::launchRequested()` -> succeeds, game launches.
3. `menu_la::isActive()=true` -> `menu_la::setSuspended(true)`.
4. Game launches with foreground. hbmenu stays as suspended library applet.

**State:** `app: running=T, hasFG=T` / `menu_la: active=T, suspended=T` / `g_smiMenuReady=F`

**First Home press (game -> hbmenu):**

1. msg 20 / SAMS case 2.
2. `app::isRunning()=true`, `app::hasForeground()=true` -> `onHomeSuspend()`, `appletRequestToGetForeground()`.
3. `menu_la::isActive()=true`, `g_smiMenuReady=false`:
   - Skip `ApplicationSuspended` notification (hbmenu can't parse it).
   - `menu_la::isSuspended()=true` -> push wake signal (reason=0, tid). hbmenu wakes at OS level. `setSuspended(false)`.
4. hbmenu shows on screen.

**State:** `app: running=T, hasFG=F` / `menu_la: active=T, suspended=F` / `g_smiMenuReady=F`

**Second Home press (hbmenu -> game, daemon-side resume):**

1. msg 20 / SAMS case 2.
2. `app::isRunning()=true`, `app::hasForeground()=false` -> skip first branch.
3. `else if (menu_la::isActive())`:
   - `g_smiMenuReady=false` AND `app::isRunning()=true`:
   - **Daemon-side resume**: `menu_la::setSuspended(true)`, `app::resume()`.
   - Game gets foreground directly. hbmenu goes back to suspended state.

**State:** Same as after launch. Subsequent Home presses alternate between these two states.

---

### 4. Game Exit

#### 4a. While SwitchU menu is suspended (keep-alive)

1. `app::checkFinished()` -> true. `g_running=false`, `g_hasForeground=false`, `g_suspendedTitleId=0`.
2. `menu_la::isActive()=true`:
   - Push `ApplicationExited` notification.
   - `menu_la::isSuspended()=true` -> push wake signal (reason=1, tid=0).
3. Menu wakes, reads reason 1 -> `m_appRunning=false`, `m_suspendedTitleId=0`, clears suspended badges.
4. Menu shows normally.

#### 4b. While no menu is active

1. `app::checkFinished()` -> true.
2. `menu_la::isActive()=false` -> `menu_la::launch(MainMenu, buildSystemStatus())`.
3. `SystemStatus` has `app_running=false`, `suspended_app_id=0`.
4. Menu launches fresh, shows main menu.

#### 4c. While hbmenu is the menu (non-SMI)

1. `app::checkFinished()` -> true.
2. `menu_la::isActive()=true`:
   - Push `ApplicationExited` notification (hbmenu ignores).
   - `menu_la::isSuspended()=true` -> push wake signal (reason=1, tid=0). hbmenu wakes at OS level.
3. hbmenu shows. No suspended game state to worry about.

---

### 5. Album Launch from Menu

1. Menu sends `LaunchAlbum` (cmd 10) -> daemon sets `g_pendingAlbum=true`.
2. Menu sends `MenuClosing` (cmd 41) and exits.
3. mainLoop: `menu_la::checkFinished()` -> true, `g_smiMenuReady=false`.
4. `g_pendingAlbum=true`:
   - `launchLibraryApplet(PhotoViewer, "Album")` -> enters blocking loop.
5. Inside blocking loop:
   - Processes SAMS, applet messages, and SMI commands every 10ms.
   - Checks `g_pendingForegroundAppletHome` (Home pressed).
   - Checks `kBreezeGotoGameFlag` (Breeze wants to resume game).
6. When Album exits:
   - Blocking loop ends.
   - Post-Album logic checks Breeze flags.
   - No flags -> "relaunching menu after album" -> `menu_la::launch(MainMenu, buildSystemStatus())`.

---

### 6. Breeze (Overlay Homebrew via Album)

Breeze communicates with the daemon via flag files:
- `kBreezeActiveFlag`: Breeze is running.
- `kBreezeGotoGameFlag`: Breeze wants to launch/resume a game.

**Home press with Breeze active (game running with foreground):**

1. Daemon detects `kBreezeActiveFlag`.
2. If menu is active, terminates it (`g_smiMenuReady=false`).
3. Launches Album via `launchLibraryApplet()` -> enters blocking loop.
4. Inside loop, Breeze runs in Album.

**When Album exits:**
- `breeze_goto_game` flag set -> resume game.
- `breeze_active` still set + game running -> resume game (Home toggle through Breeze).
- Neither flag -> relaunch menu.

**Home toggle within Breeze (game running, Breeze Album in blocking loop):**

When Home is pressed while Breeze is active and a game has foreground:
1. `g_breezeAlbumActive=true` check in blocking loop.
2. `g_pendingForegroundAppletHome` -> if Breeze active + game running: `app::resume()` (game resumes, Breeze stays).
3. Otherwise: close Album, exit loop.

---

### 7. Wakeup from Sleep (msg 26)

1. If game running and no menu active: `app::resume()` (give game foreground).
2. Else: `appletRequestToGetForeground()` (daemon takes foreground).
3. If menu active: push `WakeUp` notification.
4. Else if no game running: launch menu fresh.

---

### 8. System Actions from Menu

The menu can request system actions via SMI:

| Command | Action |
|---|---|
| `EnterSleep` | `appletStartSleepSequence(true)` |
| `Shutdown` | `appletStartShutdownSequence()` |
| `Reboot` | `appletStartRebootSequence()` |
| `RequestForeground` | `appletRequestToGetForeground()` |
| `LaunchControllers` | Rejected while menu is active |
| `LaunchNetConnect` | Sets `g_pendingNetConnect`, executed after menu exits |
| `LaunchMiiEditor` | Sets `g_pendingMiiEditor`, executed after menu exits |

`NetConnect` and `MiiEditor` follow the same pattern: set a pending flag, menu exits, daemon launches the applet via `launchLibraryApplet()` (blocking loop), then relaunches the menu afterward.

---

## Key Mechanisms

### The Pending Flag Pattern

The menu can't directly trigger heavy operations (launch game, launch Album, etc.) because it's a library applet that needs to exit or suspend first. The pattern is:

1. Menu sends SMI command -> daemon sets `g_pending*` flag.
2. Menu sends `MenuSuspending` (keep-alive) or `MenuClosing` (exit).
3. Daemon detects menu suspended/exited.
4. Daemon executes the pending operation.
5. Daemon relaunches menu if needed.

**Via `MenuSuspending` (keep-alive):** Launch/resume happens immediately while menu stays alive in background. Used for game launch and resume.

**Via `MenuClosing` (exit):** `checkFinished()` fires in mainLoop. Pending flags are checked in priority order:
1. Album
2. NetConnect
3. MiiEditor
4. Launch (game)
5. Resume (game)
6. (relaunch menu if no game running)

### The Blocking Loop (`launchLibraryApplet`)

When the daemon launches a foreground applet (Album, NetConnect, MiiEditor), it enters a blocking loop:

1. Sets `g_foregroundAppletActive=true`.
2. Polls `handleGeneralChannel()`, `handleAppletMessages()`, `handleMenuCommand()` every 10ms.
3. Checks `g_pendingForegroundAppletHome` to close the applet on Home press.
4. Checks `kBreezeGotoGameFlag` to close Album when Breeze wants to launch a game.
5. On exit: sets `g_foregroundAppletActive=false`, joins/closes the holder.

This is important because **msg 50 can fire while inside this loop** (e.g., sphaira running in Album). The loop's `handleAppletMessages()` processes msg 50 and launches the game. When Home is later pressed (also inside the loop), `g_pendingForegroundAppletHome` closes the applet, the loop exits, and the post-applet path relaunches the menu with `buildSystemStatus()` containing the suspended game info.

### The `g_smiMenuReady` Distinction

This flag tells the daemon whether `menu_la` contains the SwitchU menu (which speaks SMI) or a third-party app like hbmenu (which doesn't):

- **Set true** when `MenuReady` (cmd 40) is received -- only SwitchU sends this.
- **Cleared** when the menu exits (`checkFinished`), is terminated (Breeze takeover), or the daemon shuts down.

| Scenario | SwitchU (`g_smiMenuReady=true`) | hbmenu (`g_smiMenuReady=false`) |
|---|---|---|
| Game suspend (Home from game) | Push `ApplicationSuspended` notification + wake signal | Wake signal only (skip notification) |
| Resume (Home from menu) | Push `HomeRequest` -> menu handles resume via SMI | Daemon-side: `setSuspended(true)` + `app::resume()` directly |
| Game exit | Push `ApplicationExited` + wake signal | Push notification (ignored) + wake signal (OS-level wake) |

### The Wake Signal Mechanism

When the menu self-suspends (keep-alive), it enters a tight poll loop checking `appletPopInteractiveInData` for a `WakeSignal` struct. The daemon pushes wake signals to bring the menu back:

| Reason | Meaning | Daemon sets |
|---|---|---|
| 0 | Game was suspended by Home (return to menu) | `suspended_tid = titleId` |
| 1 | Game exited while menu was suspended | `suspended_tid = 0` |

After pushing a wake signal, the daemon sets `setSuspended(false)` since the menu is now awake.

For non-SMI menus (hbmenu), the wake signal push into interactive storage is enough to wake the applet at the OS level, even though hbmenu doesn't parse the `WakeSignal` struct.

### `buildSystemStatus()`

Every time the daemon launches the menu, it passes a `SystemStatus` struct:

```cpp
struct SystemStatus {
    uint64_t  suspended_app_id;   // title ID of suspended game (0 if none)
    uint8_t   selected_user[16];
    bool      app_running;        // whether a game process is alive
    uint8_t   _pad[7];
};
```

The menu reads this at startup via `setStartupStatus()`, immediately knowing whether a game is suspended and which one. This is how the menu can show the correct state even when freshly launched (e.g., after Album exits with a game suspended).

---

## Startup Flow

1. `__appInit`: Initialize all services (sm, fs, applet, time, ns, account, etc.), mount SD card, open log file.
2. `main()`:
   1. `appletLoadAndApplyIdlePolicySettings()`
   2. `appletBeginToWatchShortHomeButtonMessage()`
   3. `appletSetHandlesRequestToDisplay(true)`
   4. `nxtcInitialize()`
   5. Snapshot initial app records + view flags (for delta detection later).
   6. Start IPC server.
   7. Start event manager thread (monitors app record changes and gamecard mount failures).
   8. Launch menu: `menu_la::launch(StartupBoot, buildSystemStatus())`.
   9. Enter main loop (10ms tick): `mainLoop()` forever.

---

## Home Button Dispatch Summary

The Home button arrives via two channels (SAMS general channel case 2 and applet message 20). Both follow the same decision tree:

```
Home pressed
|
+-- Game running with foreground?
|   |
|   +-- YES: onHomeSuspend(), requestToGetForeground()
|   |   |
|   |   +-- Breeze active flag?
|   |   |   +-- YES: Launch Album for Breeze (or resume into existing Breeze)
|   |   |
|   |   +-- menu_la active?
|   |   |   +-- YES, g_smiMenuReady:
|   |   |   |   Push ApplicationSuspended + wake signal
|   |   |   +-- YES, !g_smiMenuReady:
|   |   |       Wake signal only (non-SMI menu)
|   |   |
|   |   +-- Foreground applet active (blocking loop)?
|   |   |   +-- YES: Set g_pendingForegroundAppletHome (close applet)
|   |   |
|   |   +-- Nothing active:
|   |       Launch menu in Resume mode with buildSystemStatus()
|   |
|   +-- NO (game suspended or no game):
|       |
|       +-- menu_la active?
|       |   +-- YES, g_smiMenuReady:
|       |   |   Push HomeRequest (menu decides: resume game or normal Home)
|       |   +-- YES, !g_smiMenuReady, game running:
|       |       Daemon-side resume: setSuspended(true) + app::resume()
|       |
|       +-- Foreground applet active?
|           +-- YES: Set g_pendingForegroundAppletHome
```

---

## SMI Command Reference

### Menu -> Daemon (SystemMessage)

| Cmd | Name | Effect |
|---|---|---|
| 1 | LaunchApplication | Sets `g_pendingLaunch` with title_id and uid |
| 2 | ResumeApplication | Sets `g_pendingResume` |
| 3 | TerminateApplication | Calls `app::terminate()`, pushes `ApplicationExited` |
| 10 | LaunchAlbum | Sets `g_pendingAlbum` |
| 11 | LaunchMiiEditor | Sets `g_pendingMiiEditor` |
| 12 | LaunchNetConnect | Sets `g_pendingNetConnect` |
| 13 | LaunchControllers | Rejected while menu active |
| 20 | EnterSleep | `appletStartSleepSequence(true)` |
| 21 | Shutdown | `appletStartShutdownSequence()` |
| 22 | Reboot | `appletStartRebootSequence()` |
| 30 | RequestForeground | `appletRequestToGetForeground()` |
| 40 | MenuReady | Sets `g_smiMenuReady=true` |
| 41 | MenuClosing | Informational (logged) |
| 42 | MenuSuspending | Sets `setSuspended(true)`, executes pending launch/resume |

### Daemon -> Menu (MenuMessage, via notification)

| Name | When |
|---|---|
| ApplicationSuspended | Game suspended by Home (with title_id) |
| ApplicationExited | Game process ended |
| HomeRequest | Home pressed while menu is showing |
| WakeUp | System woke from sleep |
| AppRecordsChanged | Installed app list changed |
| AppViewFlagsUpdate | Individual app's view flags changed (with title_id + flags) |
| GameCardMountFailure | Gamecard mount failed (with error code) |

### Daemon -> Menu (WakeSignal, via interactive storage)

| Reason | Meaning |
|---|---|
| 0 | Game suspended, return to menu (with `suspended_tid`) |
| 1 | Game exited while menu was suspended (`suspended_tid=0`) |

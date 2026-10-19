# Splatoon 1 PC Port (experimental)

An unofficial, incomplete Windows/Linux x86-64 native port. The launcher and [RebrewU](https://github.com/ApfelTeeSaft/RebrewU) recompiler are distributed as **two prebuilt binaries in one release**. Game code and game assets are generated/imported from the player's own dump **only on the player's computer**. No game binaries, Nintendo assets, or encryption keys are included in releases.

## Download and start

1. Download the ZIP for your operating system from **Releases**. Extract the *entire* archive, keeping `SplatoonSetup`, `rebrewu`, `port/`, `profiles/`, `third_party/`, and `tools/` together.
2. **Run `SplatoonSetup.exe` on Windows or `./SplatoonSetup` on Linux.** A setup window opens by default. Select a raw game dump folder or a ZIP/WUD/WUX file with the browse buttons, choose your installation directory, and click **Validate dump** or **Install / Rebuild**. A live log and progress indicator remain visible during setup. Optional WUD/WUX extraction requires your own keys and separately provided JWUDTool/Java.
3. The installer checks the RPX hash, uses the **bundled RebrewU** to generate local C++, downloads a SHA-256-verified portable Zig 0.14.1 compiler, and builds the native game using the included open-source runtime sources. Subsequent launches reuse the installed game.

### GUI and command-line usage

The default invocation opens the **graphical installer** on Windows and Linux. Windows uses native Win32 dialogs. Linux uses the GTK 3 runtime (`libgtk-3.so.0`, generally available on desktop installations); no GTK development package is needed by end users. On Linux without a display or GTK 3, use `--cli` instead. The GUI includes dump file/folder selectors, installation output selection, optional WUD/WUX helper paths, a validation-only action, live install logs, cancellable setup, and launching an installed game. Setup runs in a subprocess so that the interface stays responsive. Cancelling terminates the setup process and its child tools; partially extracted/downloaded local files may still need to be cleaned manually.

Supplying CLI options without `--gui` uses the command-line workflow for scripts and CI. Pass `--gui` to prefill paths from command-line arguments but still open the graphical installer. The GUI does not recompile or distribute game code itself: it launches the **bundled native RebrewU** on the user's computer during setup.

### Input bindings

Click **Controls...** in the launcher, or start `SplatoonSetup --controls "/path/to/installed/Game"`. The SDL2 editor has a **Device** dropdown containing **Keyboard / Mouse** and every currently connected SDL game controller or generic joystick (named individually). Select one device and configure its Wii U GamePad layout by clicking an input, then pressing a key, mouse button, gamepad button, trigger, stick direction, or raw joystick hat/axis. Delete/Backspace clears the selected binding; Escape cancels input capture. Controller axes can be assigned individually; right-click a stick-axis row to invert it. Devices can be hot-plugged while the editor is open.

**The selected device is the game's active gameplay input.** Selecting a controller disables the keyboard and mouse button/movement overlays; choosing **Keyboard / Mouse** disables controller gameplay input. The existing relative **Mouse Camera** (right-stick camera) can be enabled or disabled per layout, with adjustable sensitivity. It defaults **on** for keyboard/mouse and **off** when creating a new controller layout. Explicitly enabling mouse camera on a controller layout combines *mouse motion only* with the selected controller's right stick. The mouse still controls GamePad touch simulation in GamePad view, and `Tab` / `F11` remain available as window shortcuts.

**Save and Close** persists the active device in `controls.ini`, and independent layouts in `input_profiles/<device-id>.ini` beside your installed game executable. Switching between devices does not discard other saved layouts. The game loads the active device and its matching profile at startup. The controller identity uses its SDL GUID plus a fingerprint of its serial or device path when exposed by the driver; if neither is available, identical controllers may share a GUID/profile. If the configured device is disconnected, its profile is retained and **the game does not silently switch to another pad or keyboard**. Reconnect it or select another device in the launcher. Input settings can be changed without recompiling; **restart the game** to apply them (a newly updated runtime itself must first be rebuilt once). Existing `controls.ini` configurations are migrated, and without any configuration the original keyboard/mouse bindings are used.

### Commands

```text
SplatoonSetup                             # Open graphical installer
SplatoonSetup --gui --rom "/path/to/dump"  # Open GUI with input preselected
SplatoonSetup --cli --rom "/path/to/dump" --no-launch
SplatoonSetup --rom "/path/to/dump.zip" --validate-only
SplatoonSetup --controls "/path/to/installed/Game"  # Configure keyboard, mouse and controllers
SplatoonSetup --toolchain-only
SplatoonSetup --help
```

For WUX/WUD, supply `tools/JWUDTool.jar` and `tools/java/bin/java` (`java.exe` on Windows), or pass `--jwud` and `--java`. Decryption keys must be provided by the owner, and the base disc may need the supported update installed separately.

## Credits

**Project supervisor:** ApfelTeeSaft. **Major contribution:** NEVES, who supplied a substantial amount of Claude Code usage/tokens to support development.

This project builds upon RebrewU, a Cemu-derived shader converter (MPL-2.0), SDL2, glslang, Vulkan-Headers, volk, zlib, miniaudio and nlohmann/json. Applicable notices are in `LICENSES/`, runtime sources, and the RebrewU upstream repository. Splatoon is a Nintendo trademark; this work is not affiliated with or endorsed by Nintendo.

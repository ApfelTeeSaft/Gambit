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

The **Controls...** button in the graphical launcher opens a cross-platform SDL2 controller/keyboard/mouse editor. Select your installed game's directory first; changes are saved as **controls.ini** beside Splatoon.exe (or the Linux Splatoon binary). Click a Wii U action under **Keyboard / Mouse** or **Controller**, then press the desired key, mouse button, SDL controller button, trigger, stick direction, or joystick input. **Delete/Backspace** clears a binding and **Esc** cancels capture. The editor detects hot-plugged controllers and lets you cycle between connected SDL devices; SDL-mapped gamepads (Xbox, PlayStation, Switch Pro, etc.) and generic SDL joysticks can both be configured. Generic joysticks may require initial manual raw button/axis/hat assignments, as there is no universal physical button arrangement.

Controller stick X/Y axes have separate selectable bindings. Move the desired physical axis when capturing, and **right-click** its row to invert. Digital movement and camera directions can also be assigned to keys or gamepad buttons. The **Mouse Camera** toggle preserves the existing mouse-to-right-stick emulation in TV view, with adjustable sensitivity, or can be disabled for controller-only aiming. In GamePad/touch view, the mouse remains available for the touch panel. Tab still toggles TV/GamePad view and F11 still toggles fullscreen.

The current keyboard/mouse and gamepad controls remain the default preset. **Restore Defaults** resets the editor's draft; **Save and Close** writes your choices. Changes take effect the **next time the installed game starts**; restarting is required, but **rebuilding/recompiling is not**. Bindings belong to the installation directory, not the game dump, and the file can be backed up or edited manually. If no controls.ini exists or some entries are invalid, the runtime uses the original defaults for those settings. The controller's SDL GUID is used to prefer a selected physical model, with a fallback to another connected controller if that model is unavailable.

### Commands

```text
SplatoonSetup                             # Open graphical installer
SplatoonSetup --gui --rom "/path/to/dump"  # Open GUI with input preselected
SplatoonSetup --cli --rom "/path/to/dump" --no-launch
SplatoonSetup --rom "/path/to/dump.zip" --validate-only
SplatoonSetup --toolchain-only
SplatoonSetup --help
```

For WUX/WUD, supply `tools/JWUDTool.jar` and `tools/java/bin/java` (`java.exe` on Windows), or pass `--jwud` and `--java`. Decryption keys must be provided by the owner, and the base disc may need the supported update installed separately.

## Credits

**Project supervisor:** ApfelTeeSaft. **Major contribution:** NEVES, who supplied a substantial amount of Claude Code usage/tokens to support development.

This project builds upon RebrewU, a Cemu-derived shader converter (MPL-2.0), SDL2, glslang, Vulkan-Headers, volk, zlib, miniaudio and nlohmann/json. Applicable notices are in `LICENSES/`, runtime sources, and the RebrewU upstream repository. Splatoon is a Nintendo trademark; this work is not affiliated with or endorsed by Nintendo.

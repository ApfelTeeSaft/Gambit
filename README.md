# Splatoon 1 PC Port (experimental)

An unofficial, incomplete Windows/Linux x86-64 native port. The launcher and [RebrewU](https://github.com/ApfelTeeSaft/RebrewU) recompiler are distributed as **two prebuilt binaries in one release**. Game code and game assets are generated/imported from the player's own dump **only on the player's computer**. No game binaries, Nintendo assets, or encryption keys are included in releases.

## Download and start

1. Download the ZIP for your operating system from **Releases**. Extract the *entire* archive, keeping `SplatoonSetup`, `rebrewu`, `port/`, `profiles/`, `third_party/`, and `tools/` together.
2. **Run `SplatoonSetup.exe` on Windows or `./SplatoonSetup` on Linux.** A setup window opens by default. Select a raw game dump folder or a ZIP/WUD/WUX file with the browse buttons, choose your installation directory, and click **Validate dump** or **Install / Rebuild**. A live log and progress indicator remain visible during setup. Optional WUD/WUX extraction requires your own keys and separately provided JWUDTool/Java.
3. The installer checks the RPX hash, uses the **bundled RebrewU** to generate local C++, downloads a SHA-256-verified portable Zig 0.14.1 compiler, and builds the native game using the included open-source runtime sources. Subsequent launches reuse the installed game.

Only Splatoon **EUR AGMP01, update v16 (288)** is accepted; other versions are rejected. Vulkan 1.3 hardware/drivers are required. This is a developer-preview workflow: complete game compilation/rendering and encrypted WUX import are **not yet proven end-to-end** on both platforms (see [status](docs/status.md)). The launcher still relies on host curl/archives unless you supply those utilities.

### Setup window: which path goes where?

The large **SPLATOON 1 / PC PORT** heading and the text **Select your game dump below** are informational labels, **not additional input fields**. Older Windows releases had a UI layout bug that placed these labels beside the wrong inputs; the actual paths belong to the controls listed here.

| Field in the setup window | What to choose |
| --- | --- |
| **Game dump (source)** | Your own game dump. Use **Choose folder** for an extracted directory containing `code/Gambit.rpx`, `content/` and `meta/meta.xml` (a wrapper folder is also accepted), or **Choose file** for a ZIP, WUD or WUX image. The launcher verifies the supported EUR v16 RPX hash. |
| **Installation directory (destination)** | The **separate writable folder** where the launcher copies `code/`, `content/`, `meta/` and places the locally compiled `Splatoon.exe` (Windows) or `Splatoon` (Linux). It is **not** the game dump and not the folder containing `SplatoonSetup`. It defaults to a `Game/` folder next to the setup executable; you can change it. Do not choose a folder inside your source dump. |
| **Java executable** | Only needed for **WUD/WUX** extraction. Select a Java executable (for example, `java.exe` on Windows), or use the supported bundled `tools/java/bin/` location. Leave empty for a raw directory or ZIP. |
| **JWUDTool.jar** | Only needed for **WUD/WUX** extraction. Select your locally supplied `JWUDTool.jar`, or place it at `tools/JWUDTool.jar` beside the launcher. Leave empty for a raw directory or ZIP. You must provide your own required decryption keys. |
| **Launch game after installation** | Automatically starts the newly compiled game when setup succeeds. |

**Windows example:** With your own dump at `D:\Games\SplatoonDump\`, set **Game dump** to that folder and **Installation directory** to `D:\Splatoon\`. Your extracted release ZIP (containing `SplatoonSetup.exe` and `rebrewu.exe`) can stay somewhere else. You do not need to put the dump inside the release ZIP.

**Setup actions:** **Validate dump** checks the selected dump/version without building or installing the game (archives may be extracted for validation). **Install / Rebuild** imports the selected files and generates/builds the native executable locally. **Launch installed game** starts an existing compiled game from the installation directory. **Cancel** stops an active setup operation. The log panel below the buttons shows detailed progress and errors.

### GUI and command-line usage

The default invocation opens the **graphical installer** on Windows and Linux. Windows uses native Win32 dialogs. Linux uses the GTK 3 runtime (`libgtk-3.so.0`, generally available on desktop installations); no GTK development package is needed by end users. On Linux without a display or GTK 3, use `--cli` instead. The GUI includes dump file/folder selectors, installation output selection, optional WUD/WUX helper paths, a validation-only action, live install logs, cancellable setup, and launching an installed game. Setup runs in a subprocess so that the interface stays responsive. Cancelling terminates the setup process and its child tools; partially extracted/downloaded local files may still need to be cleaned manually.

Supplying CLI options without `--gui` uses the command-line workflow for scripts and CI. Pass `--gui` to prefill paths from command-line arguments but still open the graphical installer. The GUI does not recompile or distribute game code itself: it launches the **bundled native RebrewU** on the user's computer during setup.

During setup, the progress bar displays real percentages when the underlying tool reports measurable progress: curl transfers, tar/unzip archive entries, copied game files and Ninja build steps. Long operations without a trustworthy completion estimate (including RebrewU code generation, initial archive scanning and CMake configuration) use an animated activity bar rather than a fabricated percentage. Child tool diagnostics appear in the in-window log, and on Windows curl/tar/CMake run without opening separate command windows. Listing archive entries first adds an initial scan before extraction begins.

### Setup log and troubleshooting

The setup window uses a **native RichEdit log viewer on Windows**, and automatically moves its viewport to the newest output after each append. Both Windows and Linux keep a bounded eight-million-character GUI history to stay responsive. The full, untruncated session output is simultaneously saved as **`build/logs/setup-latest.log`** relative to the extracted release directory. The **Open full log** button opens this file while setup is running or after it finishes. A new setup run replaces `setup-latest.log`, so copy it elsewhere if you want to keep an earlier run's diagnostic log.

The log on disk is independent of the GUI text control's size limit. On Windows, curl/tar/CMake child command output is still captured without spawning additional console windows.

If a previous release failed with `port_volk` and HTTP 404, it referenced a nonexistent `1.3.296` tag in the volk repository. Updated releases use volk's verified Vulkan 1.3.296 source commit. **Publish a new versioned release and download its Windows ZIP** after merging fixes; already-downloaded release archives do not update automatically. Re-extract the new ZIP (including `port/`), and retry **Install / Rebuild**. If a failed FetchContent download is cached, delete only the release's `build/game/` build-cache directory before retrying; keep your source dump and installation directory.

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

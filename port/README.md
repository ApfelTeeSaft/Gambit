# Splatoon PC runtime

This project-specific host/runtime implementation is maintained in the parent monorepo independently of the `recompiler/` Git submodule. The native runtime translates the console-facing interfaces and graphics workload onto host APIs. It is built locally after a user imports a compatible dump and the shipped RebrewU CLI generates C++ for that dump.

Do not include dumped game data or generated C++ in public source trees, portable releases, or GitHub Actions artifacts. Building the runtime end-to-end still requires validation on supported private game dumps and standard Vulkan/SDL dependencies.

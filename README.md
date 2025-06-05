# DoD ECS Godot Demo
* USE `git clone --recursive`
* If on Windows, use `scons -jN use_llvm=yes` to build, where `N` is the number of threads you want to use.
  - or use `use_mingw=yes` if you prefer that (it's usually fater than clang).
* run `scons compile_commands.json` to generate a compilation database if you want that.

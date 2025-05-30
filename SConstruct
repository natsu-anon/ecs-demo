#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags
env.Append(CCFLAGS=["-fpermissive"])
env['CXXFLAGS'].remove("-fno-gnu-unique")

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["src/"])
sources = [
    Glob("src/*.cpp"),
    Glob("src/**/*.cpp"),
    Glob("src/*.c"),
    Glob("src/**/*.c")
]

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "bin/libecsdemo.{}.{}.framework/libecsdemo.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "bin/libecsdemo{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)

# ezpz lemon squeezy
env.Tool('compilation_db')
env.CompilationDatabase('compile_commands.json')

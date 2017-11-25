
# What is OSCAR?

Oscar is a set of programs to efficiently search in openstreetmap data

It consists of the following parts:

#### liboscar
This is the main library to provide access to serialized data-types.

#### oscar-create
This is a program to create files needed by the search applications

#### oscar-cmd
This is a simple command line program to search with the files created by oscar-create

#### oscar-gui
This is a qt-based gui application to search the files created by oscar-create

# Setup
Initialize all submodules and their submodules or clone with --recursive

# Building

## LTO and ultra builds
mkdir build && cd build
CMAKE_GCC_VERSION_FOR_LTO=4.9 cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/setup-lto.cmake -DCMAKE_BUILD_TYPE=ultra ../

Note that changing build types afterwards is not supported as this does not set the correct compiler flags


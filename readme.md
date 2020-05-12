
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

# Clone
```
git clone --recursive https://github.com/dbahrdt/oscar
```

# Prerequisites
In order to compile and run OSCAR you at least need the following libraries:

- ragel
- CGAL
- Google Protobuf
- zlib
- Cairo
- libmarble
- Qt5


Note that on some distributions these libraries are split into multiple packages with extra packages for the development files.
These are usually denoted with a "dev" at the end.

# Building

## Debug builds
```
mkdir build && cd build
cmake ../
make
```

## LTO and ultra builds
This currently only works with gcc. You first have to determine your version of gcc and its full path.
On Debian Buster this is GCC 8.2 with the full path beeing `/usr/bin/gcc-8`.
We only need the version numbers at the end:

```
mkdir build && cd build
CMAKE_GCC_VERSION_FOR_LTO=8 cmake -DCMAKE_BUILD_TYPE=ultra ../
```
On Arch this won't work due to different naming of dirs and gcc version. Instead you have to explicitly set the lto plugin:
```
CMAKE_GCC_VERSION_FOR_LTO=disable cmake -DCMAKE_LTO_PLUGIN_PATH=/usr/lib/gcc/x86_64-pc-linux-gnu/9.3.0/liblto_plugin.so -DCMAKE_BUILD_TYPE=ultra ../
```

Note that changing build types afterwards is not supported as this does not set the correct compiler flags

## AutoFDO
 * Get a not too recent perf (should work with 4.12)



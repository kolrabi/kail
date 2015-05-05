# What is it?

Developer's Image Library was previously called OpenIL, but due to trademark issues,
OpenIL is now known as DevIL. The Resiliant Image Library (ResIL) is a fork of DevIL.
From that kaIL was created.

kaIL is an Open Source programming library for programmers to incorporate into their own programs.
kaIL loads and saves a large variety of images for use in a software developer's program.
This library is capable of manipulating images in various ways and passing image information to display APIs, such as OpenGL and Direct3D.

# Features

kaIL offers the same features of loading and manipulating images its predecessors, 
plus a few improvements (see the ChangeLog for a complete list):

## Thread safety

Global state was reduced and each thread now has its own bound image context. 
That way separate threads can work on separate images concurrently.

## Metadata 

Get your EXIF data from your images.

## CMake

The build system was moved to CMake.

## Unit tests

CMake made it easy to write unit tests, so import, export and conversion functions
are now all tested.

# MinGW troubleshooting

## CMake can't find my libraries!

If you have an architecture specific subfolder, try adding 

    -DCMAKE_SYSTEM_PREFIX_PATH=<that folder> 

to the cmake command line.  For zlib there exist also the `ZLIB_ROOT` variable 
that indicates where to look.

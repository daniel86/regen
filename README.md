regen {#mainpage}
============

`regen` -- Real-time Graphics Engine -- is a portable OpenGL library written in C++.
The purpose of this library is to help creating
real-time rendering software.

A graphics card supporting the OpenGL 3.3 API is required for `regen`.
Some features from the 4.0 API are also supported but optional for backwards compatibility.
The engine was tested with NVIDIA drivers on Arch Linux and ATI drivers on
Ubuntu 11.10 (ATI dropped support for my notebook
graphics adapter so i had to use an old Ubuntu version).
Other Unix based Operating-Systems should work from the scratch.
Windows 8 with latest ATI drivers was also tested. You will find some special
notes for compiling the engine under Win in this document.

Downloading
=========================
Clone the code from [github](https://github.com/daniel86/regen).
The master branch contains the most up to date source code.
For each release you will find a custom branch.

Compiling
=========================
`regen` builds with [CMake](http://www.cmake.org/).
Run `cmake .` in the root directory to generate files needed for compiling.
On Unix systems CMake will generate Makefile's with usual targets.
Running `make` invokes the compiler and linker and
if `make` was successfull a static library `libregen.a`
was created in the root directory.
On Windows CMake generates Visual Studio files, just open them and compile from the IDE.

`regen` defines following build targets:

| Target   | Description                    |
|----------|--------------------------------|
| all      | Compile and link the engine.   |
| install  | Install compiled library.      |
| doc      | Generate doxygen documentation.|

CMake supports some default parameters for compiler and build path setup, consider the CMake documentations
for those arguments.
Following you can find a list of `cmake` arguments with special handling in the `regen` build files:

| Argument                | Default | Description                                                                                         |
|-------------------------|---------|-----------------------------------------------------------------------------------------------------|
| -DCMAKE_BUILD_TYPE      | Release | One of Release or Debug. With Debug mode the engine is compiled with additional debugging symbols.  |
| -DBUILD_TESTS           | 0       | If set to 1 the test applications will be compiled by all target                                    |
| -DBUILD_VIDEO_PLAYER    | 0       | If set to 1 the video player application will be compiled by all target                             |
| -DBUILD_TEXTURE_UPDATER | 0       | If set to 1 the texture updater application will be compiled by all target                          |
| -DBoost_DIR             | -       | Base path for Boost library. You should set this on Windows.                                        |
| -DASSIMP_DIR            | -       | Base path for Assimp library. You should set this on Windows.                                       |
| -DFREETYPE_DIR          | -       | Base path for Freetype library. You should set this on Windows.                                     |
| -DDEVIL_DIR             | -       | Base path for DevIL library. You should set this on Windows.                                        |
| -DFFMPEG_DIR            | -       | Base path for FFmpeg library. You should set this on Windows.                                       |
| -DALUT_DIR              | -       | Base path for ALut library. You should set this on Windows.                                         |


Feature List
=========================
Here you find a brief list of supported features in this library.

- `Portability`: Tested with Windows8, Ubuntu11.10 and ArchLinux
- `Augmented GLSL`:
    - input modification (constant, uniform, attribute, instanced attribute)
    - support for 'include' and 'for' directive
- `Render State`: encapsulates GL states and avoids redundant state switches.
- `Audio/Video`: Streaming from file resources, 3D Sound
- `Image loading`: Support for common image formats (png, jpg, hdr, ...)
- `Text rendering`: Loading of Freetype fonts, rendering with texture mapped text
- `Model loading`: Support for common model formats (3ds, ply, obj, ...), support for bone animations
- `Shading`: Deferred and Direct shading is supported
- `Picking`: Supports to distinguish between objects and instances
- `Scene post processing`: FXAA, Ambient Occlusion, Volumetric Fog, Tonemap ...
- `Sky Rendering`: Dynamic sky with realistic scattering
- `Particles`: Simple implementations of smoke,rain,snow particles
- `Volume Rendering`: Simple raycasting volume renderer

Dependency List
=========================
Following you can find a list of libraries that must be installed in order
to compile `regen`.
- [OpenGL](http://www.opengl.org/) >=3.3
- [OpenAL Soft](http://kcat.strangesoft.net/openal.html) >=1.1
- [Assimp](http://assimp.sourceforge.net/) >= 2.0
- [DevIL](http://openil.sourceforge.net/) >= 1.4.0
- [FreeType](http://www.freetype.org/) >= 2.4.0
- [Boost](http://www.boost.org/) (thread system date_time filesystem regex)
- [FFmpeg](http://www.ffmpeg.org/)

In order to compile the test applications you will also need to install
the following list of libraries:
- [Qt](http://qt-project.org/) >=4.0 (QtCore, QtGui, QtOpenGL)

Documentation
=========================
The documentation is hosted using the [gh-pages branch](http://daniel86.github.com/regen/)

Contact
=========================
If you find any bugs or if you have any feature requests
please report them to the [github tracker](https://github.com/daniel86/regen/issues).

You also can contact me directly via mail or Jabber if you like (address: daniel(at)orgizm.net).


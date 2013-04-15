regen {#mainpage}
============

regen -- Realtime Graphics Engine -- is a portable OpenGL library written in C++.
The purpose of this library is to help creating
real-time rendering software.

You need a graphics card supporting the OpenGL 3.3 API.
Some features from the 4.0 API are also supported but optional for backwards compatibility.
The engine is tested with NVIDIA and ATI drivers and on the
Arch Linux and Ubuntu 11.10 (ATI dropped support for my notebook
graphics adapter so i had to use an old Ubuntu version).
Other Unix based Operating-Systems should work from the scratch.
Windows support might require some work to be done but should be generally
possible too because the engine was written with portability in mind.

Compiling
=========================
regen builds with CMake. Just clone the code from [github](https://github.com/daniel86/regen) and run `cmake .` in the root directory.
You can pass a set of arguments to `cmake` to toggle some features on or off. Following you can find a list of those arguments:

Argument                | Default | Description
----------------------- | ------- | ------------
-DBUILD_TESTS           | 0       | Compile test application 
-DBUILD_VIDEO_PLAYER    | 0       | Compile video player application
-DBUILD_TEXTURE_UPDATER | 0       | Compile texture updater application

CMake should complain about any missing dependencies. On Unix Makefiles should be generated after running `cmake` and you should be able to run `make` in the root directory to build `libregen`.
On Windows visual studio project files are generated. You can open those files with visual studio 
and compile the library from the GUI of visual studio.

Feature List
=========================
Here you find a brief list of supported features in this library.

- 3D audio using OpenAL
- Video/Audio streaming using ffmpeg
- Image loading using DevIL
- Noise textures using libnoise
- Font loading using FreeType
    - displayed using texture mapped text
- Shader pre-processing
    - input modification (constant, uniform, attribute, instanced attribute)
    - support for 'include' and 'for' directive
- Assimp model loading
    - bone animation loading
- Particle engine using transform feedback
- Dynamic sky scattering
- Instancing
    - using instanced attributes
    - instanced bone animation using bone TBO
- GPU mesh animation using transform feedback
- Deferred and Direct shading
- Ambient Occlusion
- Volumetric fog
- Simple raycasting volume renderer
- Geometry picking

Dependency List
=========================
Following you can find a list of libraries that must be installed in order
to compile regen.
- OpenGL >=3.3
- OpenAL Soft >=1.1
- Boost
- assimp
- DevIL
- FreeType
- libav

In order to compile the test applications you will also need to install
the following list of libraries:
- Qt >=4.0 (QtCore, QtGui, QtOpenGL)

Documentation
=========================
The documentation is hosted using the [gh-pages branch](http://daniel86.github.com/regen/)

Contact
=========================
daniel@orgizm.net


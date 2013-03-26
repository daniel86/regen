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
- OpenGL 3.3
- OpenAL
- Boost
- assimp
- DevIL
- FreeType
- libav

In order to compile the test applications you will also need to install
the following list of libraries:
- Qt >=4.0 (QtCore, QtGui, QtOpenGL)

Contact
=========================
daniel@orgizm.net


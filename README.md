<p align="center">
    <img src="img/fluid-text-white.gif" width="320" height="90" />
</p>

<p align="center">
    <img src="img/physics.gif" width="160" height="100" />
    <img src="img/instancing.gif" width="160" height="100" />
    <img src="img/transparency.gif" width="160" height="100" />
    <img src="img/gallery.gif" width="160" height="100" />
</p>

`regen` -- Real-time Graphics Engine -- is a portable OpenGL library written in C++.
The purpose of this library is to help creating real-time rendering software.
Some example renderings created by regen are shown in the GIFs above.
For each of them you can find an example configuration for regen [here](https://github.com/daniel86/regen/tree/master/applications/scene-display/examples).

A graphics card supporting the OpenGL 3.3 API is required for `regen`.
Some features from the 4.0 API are also supported but optional for backwards compatibility.
The engine was tested with proprietary NVIDIA and ATI drivers and should work with
Unix based operating systems and Windows.

Development
=========================
The development is at  the moment mostly in maintanance mode. This includes:
- Making sure it runs with latest stable Ubuntu
- Merging pull requests

Additional features are currently only be worked on occasionally to create
specific scenes, or to explore specific algorithmic challanges in computer graphics.


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

| Target     | Description                    |
|------------|--------------------------------|
| `all`      | Compile and link the engine.   |
| `install`  | Install compiled library.      |
| `doc`      | Generate doxygen documentation.|
| `package`  | Generate a regen package (archive file on Linux).|

CMake supports some default parameters for compiler and build path setup, consider the CMake documentations
for a complete socumentation of these arguments. Some important argument are listed below.

| CMake argument          | Description                       |
|-------------------------|-----------------------------------|
| CMAKE_INSTALL_PREFIX    | Path were the engine should get installed when the `install` target is used. |
| CMAKE_INCLUDE_PATH      | Path were you installed the dependency library headers. |
| CMAKE_LIBRARY_PATH      | Path were you installed the dependency libraries. |
| -G $GENERATOR_NAME      | Specifies custom build file generator. You might need to set this to "Visual Studio 11 Win64" when you intend to build the library for Win 64 Bit. |

Following you can find a list of `cmake` arguments with special handling in the `regen` build files:

| CMake argument          | Default | Description                                                                                             |
|-------------------------|---------|---------------------------------------------------------------------------------------------------------|
| -DCMAKE_BUILD_TYPE      | Release | One of `Release` or `Debug`. With Debug mode the engine is compiled with additional debugging symbols.  |
| -DBUILD_TESTS           | 0       | If set to 1 the test applications will be compiled by `all` target                                      |
| -DBUILD_VIDEO_PLAYER    | 0       | If set to 1 the video player application will be compiled by `all` target                               |

On Windows you might have to set environment variables for the dependency libraries.
Following you can find a list of those variables:

| Environment variable    | Description                       |
|-------------------------|-----------------------------------|
| GLEW_DIR                | Base path for GLEW library.       |
| Boost_DIR               | Base path for Boost library.      |
| ASSIMP_DIR              | Base path for Assimp library.     |
| FREETYPE_DIR            | Base path for Freetype library.   |
| DEVIL_DIR               | Base path for DevIL library.      |
| FFMPEG_DIR              | Base path for FFmpeg library.     |
| OPENAL_DIR              | Base path for OpenAL library.     |
| ALUT_DIR                | Base path for ALut library.       |
| BULLET_DIR              | Base path for Bullet library.     |

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
- `Text rendering`: Loading of Freetype fonts, rendering of texture mapped text
- `Model loading`: Support for common model formats (3ds, ply, obj, ...), support for bone animations
- `Shading`: Deferred and Direct shading is supported
- `Picking`: Supports to distinguish between objects and instances
- `Scene post processing`: FXAA, Ambient Occlusion, Volumetric Fog, Tonemap ...
- `Sky Rendering`: Dynamic sky with realistic scattering
- `Particles`: Simple implementations of smoke,rain,snow particles
- `Volume Rendering`: Simple raycasting volume renderer
- `Scene Graph`: Loading scenes from XML resources

Dependency List
=========================
Following you can find a list of libraries that must be installed in order
to compile `regen`.
- [OpenGL](http://www.opengl.org/) >=3.3
- [OpenAL Soft](http://kcat.strangesoft.net/openal.html) >=1.1 and [ALUT](http://connect.creativelabs.com/openal/Documentation/The%20OpenAL%20Utility%20Toolkit.htm)
    - You might have to compile ALUT manually on Win. I had troubles linking against the binary distribution using VisualStudio, probably incompatible compilers.
- [Assimp](http://assimp.sourceforge.net/) >= 2.0
    - You have to copy the dll manually to binary dir on Win (.dll is not named like .lib)
- [DevIL](http://openil.sourceforge.net/) >= 1.4.0
- [FreeType](http://www.freetype.org/) >= 2.4.0
    - You might have to compile FreeType manually on Win. I had troubles linking against the binary distribution using VisualStudio, probably incompatible compilers.
- [Boost](http://www.boost.org/)
    - components: thread system date_time filesystem regex
    - compile options: --build-type=complete
- [FFmpeg](http://www.ffmpeg.org/)
    - download `Dev` and `Shared` package from [here](http://ffmpeg.zeranoe.com/builds/) on Win. You have to copy the dll manually to binary dir (.dll is not named like .lib).
- [Bullet](http://bulletphysics.org)

In order to compile the test applications you will also need to install
the following list of libraries:
- [Qt](http://qt-project.org/) >=5.0 (QtCore, QtGui, QtOpenGL)
    - Note: use Qt5 installer for Win

Documentation
=========================
The documentation is hosted using the [gh-pages branch](http://daniel86.github.com/regen/)

Videos of regen in action
=========================

<p align="center">
    <a href="http://www.youtube.com/watch?v=waBatPmNW9Y">
        <img src="http://img.youtube.com/vi/waBatPmNW9Y/0.jpg"  width="160" height="120" />
    </a>
    <a href="http://www.youtube.com/watch?v=nNI9y0Ey_m8">
        <img src="http://img.youtube.com/vi/nNI9y0Ey_m8/0.jpg"  width="160" height="120" />
    </a>
    <a href="http://www.youtube.com/watch?v=VYKM_JCpGYk">
        <img src="http://img.youtube.com/vi/VYKM_JCpGYk/0.jpg"  width="160" height="120" />
    </a>
    <a href="http://www.youtube.com/watch?v=7nJSMTlSmjo">
        <img src="http://img.youtube.com/vi/7nJSMTlSmjo/0.jpg"  width="160" height="120" />
    </a>
    <a href="http://www.youtube.com/watch?v=kzlMzHjgKpg">
        <img src="http://img.youtube.com/vi/kzlMzHjgKpg/0.jpg"  width="160" height="120" />
    </a>
    <a href="http://www.youtube.com/watch?v=Msn6Teot7ZU">
        <img src="http://img.youtube.com/vi/Msn6Teot7ZU/0.jpg"  width="160" height="120" />
    </a>
    <a href="http://www.youtube.com/watch?v=jaFgTxK6WjU">
        <img src="http://img.youtube.com/vi/jaFgTxK6WjU/0.jpg"  width="160" height="120" />
    </a>
    <a href="http://www.youtube.com/watch?v=Mk0Nc-UMAMM">
        <img src="http://img.youtube.com/vi/Mk0Nc-UMAMM/0.jpg"  width="160" height="120" />
    </a>
</p>

Contact
=========================
If you find any bugs or if you have any feature requests
please report them to the [github tracker](https://github.com/daniel86/regen/issues).

You also can contact me directly via mail or Jabber if you like (address: daniel(at)orgizm.net).


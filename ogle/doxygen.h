
#ifndef __DOXYGEN_H_
#define __DOXYGEN_H_

/**
@mainpage OGLE

OGLE is a portable OpenGL library written in C++.
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

@section features Feature List
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

@section deps Dependency List
Following you can find a list of libraries that must be installed in order
to compile OGLE.
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

@section contact Contact
daniel@orgizm.net

@page Tutorials Tutorials

@section tut_win Creating a window

First create the root node of the render tree for your application.
The root node will be traversed each frame.
@code
  ref_ptr<RootNode> rootNode = ref_ptr<RootNode>::manage(new RootNode);
@endcode

Then instantiate the application and show the window.
@code
  ref_ptr<QtApplication> app = ref_ptr<QtApplication>::manage(
      new QtApplication(rootNode,argc,argv));
  app->set_windowTitle("My Application");
  app->show();
@endcode

Finally enter the main loop. This call will block until the engine exits.
@code
app->mainLoop();
@endcode

@section tut_fbo Creating a render target

Instantiate the FBO. Specify texture dimensions and depth texture parameters.
If you set them to anything but GL_NONE the depth texture is automatically attached.
@code
  FrameBufferObject *fbo = new FrameBufferObject(
      width, height, 1,
      GL_TEXTURE_2D,
      GL_DEPTH_COMPONENT24,
      GL_UNSIGNED_BYTE);
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(
      new FBOState(ref_ptr<FrameBufferObject>::manage(fbo)));
@endcode

Add a color texture to the render target.
@code
  ref_ptr<Texture> colorAttachment = fbo->addTexture(1,
      GL_TEXTURE_2D,
      GL_RGBA, GL_RGBA,
      GL_UNSIGNED_BYTE);
@endcode

Call glDrawBuffer when the FBOState is traversed.
@code
  fboState->addDrawBuffer(GL_COLOR_ATTACHMENT0);
@endcode

Clear depth and color attachments when the tree is traversed.
@code
  ClearColorState::Data clearData;
  clearData.clearColor = Vec4f(0.0f);
  clearData.colorBuffers.push_back(GL_COLOR_ATTACHMENT0);
  fboState->setClearColor(clearData);
  fboState->setClearDepth();
@endcode

Attach the FBO to the render tree.
@code
  ref_ptr<StateNode> fboNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(fboState)));
  rootNode->addChild(fboNode);
@endcode

Display the color attachment by adding a blit state to the render tree.
@code
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitToScreen(fbo, screenSize, GL_COLOR_ATTACHMENT0));
  rootNode->addChild(ref_ptr<StateNode>::manage(new StateNode(blitState)));
@endcode

@section tut_cube Rendering a Cube

Instantiate the Box mesh, the box is centered at origin.
@code
    Box::Config cubeConfig;
    cubeConfig.posScale = Vec3f(1.0f);
    ref_ptr<Mesh> cube = ref_ptr<Mesh>::manage(new Box(cubeConfig));
@endcode

Move the Box center to another position using the model matrix.
@code
    ref_ptr<ModelTransformation> modelMat = ref_ptr<ModelTransformation>::manage(new ModelTransformation);
    modelMat->translate(Vec3f(-2.0f, 0.75f, 0.0f), 0.0f);
    cube->joinStates(ref_ptr<State>::cast(modelMat));
@endcode

Attach a material state to the cube.
@code
    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_copper();
    cube->joinStates(ref_ptr<State>::cast(material));
@endcode

Attach a shader state to the cube. Note that the shader is not yet loaded.
@code
    ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
    cube->joinStates(ref_ptr<State>::cast(shaderState));
@endcode

Add the cube to the render tree. Use the FBO state as parent, so that all rendering
is going to the offscreen render target.
@code
    ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(cube)));
    fboState->addChild(meshNode);
@endcode

Configure the shader using the hierarchical tree structure.
@code
    ShaderConfigurer shaderConfigurer;
    shaderConfigurer.addNode(meshNode.get());
@endcode

Finally compile the shader that is used to render the cube. Note that the 'mesh' shader is used
here. It can be considered as some kind of Uber shader that blanks out a lot of unused functionality using
the shader configuration generated above.
@code
    shaderState->createShader(shaderConfigurer.cfg(), "mesh");
@endcode

@section tut_shading Deferred Shading

Deferred and direct shading is supported but you should use deferred shading
whenever possible.

The geometry is processed separated from the shading calculation.
The geometry pass renders to G-buffer, the G-buffer must have a set of attachment for
the shading to work. In the current implementation attachments for color, specular
and for the world space normal are used. Positions are reconstructed from depth values.
For the color 2 attachments are used for ping pong rendering.
Here is how You can setup the FBO to be a G-buffer.
@code
  static const GLenum count[] = { 2, 1, 1 };
  static const GLenum formats[] = { GL_RGBA, GL_RGBA, GL_RGBA };
  static const GLenum internalFormats[] = { colorBufferFormat, GL_RGBA, GL_RGBA };
  static const GLenum clearBuffers[] = {
      GL_COLOR_ATTACHMENT2, // spec
      GL_COLOR_ATTACHMENT3  // norWorld
  };
  for(GLuint i=0; i<sizeof(count)/sizeof(GLenum); ++i) {
    fbo->addTexture(count[i], GL_TEXTURE_2D,
        formats[i], internalFormats[i], GL_UNSIGNED_BYTE);
    // call glDrawBuffer
    fboState->addDrawBuffer(GL_COLOR_ATTACHMENT0+i+1);
  }
  ref_ptr<Texture> gDiffuseTexture = fbo->colorBuffer()[0];
  ref_ptr<Texture> gSpecularTexture = fbo->colorBuffer()[2];
  ref_ptr<Texture> gNorWorldTexture = fbo->colorBuffer()[3];
  ref_ptr<Texture> gDepthTexture = fbo->depthTexture();
@endcode

Create the shading state and set G-buffer textures.
@code
  ref_ptr<DeferredShading> shading =
      ref_ptr<DeferredShading>::manage(new DeferredShading);
  shading->set_gBuffer(
      gDepthTexture, gNorWorldTexture,
      gDiffuseTexture, gSpecularTexture);
@endcode

Setup the render target for the deferred shading pass.
@code
  ref_ptr<FBOState> fboState =
      ref_ptr<FBOState>::manage(new FBOState(gBuffer));
  // Ping-Pong rendering
  fboState->setDrawBufferUpdate(gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  shading->joinStatesFront(ref_ptr<State>::manage(new FramebufferClear));
  shading->joinStatesFront(ref_ptr<State>::cast(fboState));
@endcode

No depth test/write needed during deferred shading.
@code
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);
  shading->joinStatesFront(ref_ptr<State>::cast(depthState));
@endcode

Finally add the state to the tree, use the tree to generate a shader configuration
and compile shaders used by the deferred shading pipeline.
@code
  ref_ptr<StateNode> shadingNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(shading)));
  rootNode->addChild(shadingNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(shadingNode.get());
  shading->createShader(shaderConfigurer.cfg());
@endcode

 */

/// @dir algebra
/// \brief Frequently used algorithms for 3D geometry.
/// @dir animations
/// \brief Collection of Animation implementations.
/// @dir av
/// \brief Interface for OpenAL and libav.
/// @dir gl-types
/// \brief Interface to OpenGL API.
/// @dir meshes
/// \brief Mesh implementations.
/// @dir shading
/// \brief Lights and shadows.
/// @dir states
/// \brief Collection of State implementations.
/// @dir textures
/// \brief Texture loading.
/// @dir utility
/// \brief Miscellaneous types, defines and algorithms.

#endif /* __DOXYGEN_H_ */

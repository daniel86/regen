
#ifndef __DOXYGEN_H_
#define __DOXYGEN_H_

/**
@page Shader Shader Loading
When a shader is loaded the code is pre-processed on the CPU before it is
send to the GL. Regular GLSL code should work fine but the actual
code send to the GL maybe different from the code passed in.

The pre-processors are implemented in regen::GLSLInputOutputProcessor and
regen::GLSLDirectiveProcessor.

@section directives Directive handling
- \#define can be used as usual
- \#ifdef,\#if,\#elif,\#else,\#endif directives are evaluated
  and undefined code is not send to the GL.
- \#line directives are ignored and not send to the GL as they are confusing in
  combination with the pre-processors.
- \#version directives are not send to GL as passed in. The directive processor look
  for the maximum version and at last the \#version directive with
  maximum version is prepended to shader code (ATI is really strict about having
  the version directive the very first statement in the code).

@section macro_replace Macro name replacing
Macro names are only replaced by the defined value when explicitly requested
using a special notation. The notation is a leading dollar sign followed by the macro
name surrounded by curly brackets.
For example:
@code
#define2 FOO 2
int i = ${FOO};
@endcode

@section include_directive \#include directive
\#include directives are supported using GLSW. They are evaluated recursively.
You can add custom include paths with ogle::Application::addShaderPath.
The include key is build from the shader filename and a named section in the shader.
Nodes in the path are separated by dots.
For example to load the subsection 'bar' from the section 'foo' in the
file 'baz' the include key would be 'baz.foo.bar'.
The actual GLSL code would look like this:
@code
#include baz.foo.bar
@endcode

regen::ShaderState scans for implemented shader stages by appending the shader stage
prefix (regen::GLEnum::glslStagePrefix) to the include key and compiles all
defined stages into the shader program.

@section define2_directive \#define2 directive
Practically the same as \#define with the exception that the define is not
included in the code send to GL. In other words this define can only be used
by CPU pre-processors.
For example this would work as intended:
@code
#define2 HAS_FOO
#ifdef HAS_FOO
do_something();
#endif
@endcode

@section for_directive \#for directive
A \#for directive is supported that allows to define code n-times.
Inside the for loop the current index in the for loop can be accessed.
The index is actually defined using the \#define2 directive.
For example following would be repeated 8 times:
@code
#for INDEX to 8
  {
    int i = ${INDEX};
  }
#endfor
@endcode

@section io_names IO name matching
The pre-processor handles name matching between shader stages by redefining the
IO names.
It is good practice to use 'in_' and 'out_' prefix for all inputs and outputs
in each shader stage and let the pre-processor decide about the naming.
You could also bypass this suggestion and give the IO data arbitrary names.

For attributes a stage prefix (regen::GLEnum::glslStagePrefix) is prepended,
uniforms get a 'u_' prefix and constants get a 'c_' prefix.

For example this code is valid:
@code
-- vs
in vec3 in_pos;
out vec3 out_pos;
void main() {
  out_pos = in_pos;
}
-- fs
in vec3 in_pos;
void main() {
  do_something(in_pos);
}
@endcode

@section io_gen IO generator
Often you don't do anything special with attributes in a shader stage and only pass
them through to the stage that actually operates on the data.
The pre-processor can generate such pass through code for you. This actually
allows defining shader stages without knowing the input attributes of the following
stage allowing to define more generic shaders.

When a stage is processed the pre-processor iterates over the inputs declared
in the following state and makes sure that there is matching input.
If not a pass through output is generated.

The generator does only operate when you explicitly request it.
The IO code is generated only if you define HANDLE_IO(i) somewhere outside the main
function.
Additionally you have to call this macro somewhere inside the main function.

For example this code would pass an attribute named 'foo' to the fragment shader:
@code
-- vs
#define HANDLE_IO(i)
void main() {
  HANDLE_IO(glVertexID);
}
-- fs
in vec3 in_foo;
void main() {
  do_something(in_foo);
}
@endcode

@section io_trans IO transformation
Shader input in this engine is defined as constant input (never changing for a
compiled program), uniform input (not changing during shader invocation),
vertex attribute input (changing per vertex) and instanced attribute input
(changing per instance).
On the CPU all types of input are represented by regen::ShaderInput.

Pre-processors allow transformation from one input type to another.
This allows defining code without knowing the actual type of the input.
For example you can define a simple material shader declaring all
material inputs as constant inputs and this shader could also be used
as instanced material shader or none constant material shader using uniforms.
*/

/**
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

/**
@dir algebra
\brief Frequently used algorithms for 3D geometry.

@dir animations
\brief Collection of Animation implementations.

@dir av
\brief Interface for OpenAL and libav.

@dir gl-types
\brief Interface to OpenGL API.

@dir meshes
\brief Mesh implementations.

@dir shading
\brief Lights and shadows.

@dir states
\brief Collection of State implementations.

@dir textures
\brief Texture loading.

@dir utility
\brief Miscellaneous types, defines and algorithms.
 */

#endif /* __DOXYGEN_H_ */

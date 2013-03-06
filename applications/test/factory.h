/*
 * factory.h
 *
 *  Created on: 17.02.2013
 *      Author: daniel
 */

#ifndef FACTORY_H_
#define FACTORY_H_

#include <applications/fltk-ogle-application.h>
#include <applications/application-config.h>
#include <ogle/config.h>

#include <ogle/utility/font-manager.h>

#include <ogle/states/shader-configurer.h>

#include <ogle/meshes/rectangle.h>
#include <ogle/meshes/sky.h>
#include <ogle/meshes/box.h>
#include <ogle/meshes/sphere.h>
#include <ogle/meshes/texture-mapped-text.h>
#include <ogle/meshes/precipitation-particles.h>

#include <ogle/states/fbo-state.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/blit-state.h>
#include <ogle/states/material-state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/atomic-states.h>
#include <ogle/states/depth-of-field.h>
#include <ogle/states/tonemap.h>
#include <ogle/states/anti-aliasing.h>
#include <ogle/states/transparency-state.h>
#include <ogle/states/assimp-importer.h>
#include <ogle/states/tesselation-state.h>
#include <ogle/states/picking.h>
#include <ogle/states/volumetric-fog.h>
#include <ogle/states/distance-fog.h>

#include <ogle/shading/deferred.h>
#include <ogle/shading/post-processing.h>
#include <ogle/shading/light-shafts.h>
#include <ogle/shading/directional-shadow-map.h>
#include <ogle/shading/point-shadow-map.h>
#include <ogle/shading/spot-shadow-map.h>

#include <ogle/filter/filter.h>

#include <ogle/textures/texture-loader.h>

#include <ogle/animations/animation-manager.h>
#include <ogle/animations/mesh-animation.h>
#include <ogle/animations/camera-manipulator.h>

namespace ogle {

struct BoneAnimRange {
  string name;
  Vec2d range;
};
struct MeshData {
  ref_ptr<MeshState> mesh_;
  ref_ptr<ShaderState> shader_;
  ref_ptr<StateNode> node_;
};

class SortByModelMatrix : public State
{
public:
  SortByModelMatrix(ref_ptr<StateNode> &n, ref_ptr<PerspectiveCamera> &cam, GLboolean frontToBack)
  : State(), n_(n), comparator_(cam,frontToBack) {}

  virtual void enable(RenderState *state) {
    n_->childs().sort(comparator_);
  }
protected:
  ref_ptr<StateNode> n_;
  NodeEyeDepthComparator comparator_;
};

// create application window and set up OpenGL
ref_ptr<OGLEFltkApplication> initApplication(
    int argc, char** argv, const string &windowTitle);

// Blits fbo attachment to screen
void setBlitToScreen(
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &texture,
    GLenum attachment);
void setBlitToScreen(
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    GLenum attachment);

ref_ptr<TextureCube> createStaticReflectionMap(
    OGLEFltkApplication *app,
    const string &file,
    const GLboolean flipBackFace,
    const GLenum textureFormat,
    const GLfloat aniso=2.0f);

ref_ptr<PickingGeom> createPicker(
    GLdouble interval=50.0, GLuint maxPickedObjects=999);

/////////////////////////////////////
//// Camera
/////////////////////////////////////

ref_ptr<LookAtCameraManipulator> createLookAtCameraManipulator(
    OGLEApplication *app,
    const ref_ptr<PerspectiveCamera> &cam,
    const GLfloat &scrollStep=2.0f,
    const GLfloat &stepX=0.02f,
    const GLfloat &stepY=0.001f,
    const GLuint &interval=10);

ref_ptr<PerspectiveCamera> createPerspectiveCamera(
    OGLEApplication *app,
    GLfloat fov=45.0f,
    GLfloat near=0.1f,
    GLfloat far=200.0f);

/////////////////////////////////////
//// Instancing
/////////////////////////////////////

ref_ptr<ModelTransformation> createInstancedModelMat(
    GLuint numInstancesX, GLuint numInstancesY, GLfloat instanceDistance);

/////////////////////////////////////
//// GBuffer / TBuffer
/////////////////////////////////////

// Creates render target for deferred shading.
ref_ptr<FBOState> createGBuffer(
    OGLEApplication *app,
    GLfloat gBufferScaleW=1.0,
    GLfloat gBufferScaleH=1.0,
    GLenum colorBufferFormat=GL_RGBA,
    GLenum depthFormat=GL_DEPTH_COMPONENT24);

ref_ptr<TransparencyState> createTBuffer(
    OGLEApplication *app,
    const ref_ptr<PerspectiveCamera> &cam,
    const ref_ptr<Texture> &depthTexture,
    TransparencyMode mode=TRANSPARENCY_MODE_FRONT_TO_BACK,
    GLfloat tBufferScaleW=1.0,
    GLfloat tBufferScaleH=1.0);

ref_ptr<State> resolveTransparency(
    OGLEApplication *app,
    const ref_ptr<TransparencyState> &transparency,
    const ref_ptr<StateNode> &root);

/////////////////////////////////////
//// Post Passes
/////////////////////////////////////

// Creates root node for states rendering the background of the scene
ref_ptr<StateNode> createPostPassNode(
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment);

ref_ptr<FilterSequence> createBlurState(
    OGLEFltkApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<StateNode> &root,
    GLuint size, GLfloat sigma,
    GLboolean downsampleTwice=GL_FALSE);

ref_ptr<DepthOfField> createDoFState(
    OGLEFltkApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput,
    const ref_ptr<Texture> &depthInput,
    const ref_ptr<StateNode> &root);

ref_ptr<Tonemap> createTonemapState(
    OGLEFltkApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput,
    const ref_ptr<StateNode> &root);

ref_ptr<AntiAliasing> createAAState(
    OGLEFltkApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<StateNode> &root);

/////////////////////////////////////
//// Background/Foreground States
/////////////////////////////////////

// Creates root node for states rendering the background of the scene
ref_ptr<StateNode> createBackground(
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment);

// Creates sky box mesh
ref_ptr<DynamicSky> createSky(OGLEApplication *app, const ref_ptr<StateNode> &root);

ref_ptr<SkyBox> createSkyCube(
    OGLEFltkApplication *app,
    const ref_ptr<TextureCube> &reflectionMap,
    const ref_ptr<StateNode> &root);

ref_ptr<SnowParticles> createSnow(
    OGLEFltkApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root,
    GLuint numSnowFlakes = 5000);

ref_ptr<RainParticles> createRain(
    OGLEFltkApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root,
    GLuint numParticles=5000);

ref_ptr<VolumetricFog> createVolumeFog(
    OGLEFltkApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<Texture> &tBufferColor,
    const ref_ptr<Texture> &tBufferDepth,
    const ref_ptr<StateNode> &root);
ref_ptr<VolumetricFog> createVolumeFog(
    OGLEFltkApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root);

ref_ptr<DistanceFog> createDistanceFog(
    OGLEFltkApplication *app,
    const Vec3f &fogColor,
    const ref_ptr<TextureCube> &skyColor,
    const ref_ptr<Texture> &gDepth,
    const ref_ptr<Texture> &tBufferColor,
    const ref_ptr<Texture> &tBufferDepth,
    const ref_ptr<StateNode> &root);
ref_ptr<DistanceFog> createDistanceFog(
    OGLEFltkApplication *app,
    const Vec3f &fogColor,
    const ref_ptr<TextureCube> &skyColor,
    const ref_ptr<Texture> &gDepth,
    const ref_ptr<StateNode> &root);

/////////////////////////////////////
//// Shading States
/////////////////////////////////////

// Creates deferred shading state and add to render tree
ref_ptr<DeferredShading> createShadingPass(
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &gBuffer,
    const ref_ptr<StateNode> &root,
    ShadowMap::FilterMode shadowFiltering=ShadowMap::FILTERING_NONE,
    GLboolean useAmbientLight=GL_TRUE);

ref_ptr<ShadingPostProcessing> createShadingPostProcessing(
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &gBuffer,
    const ref_ptr<StateNode> &root,
    GLboolean useAmbientOcclusion=GL_TRUE);

ref_ptr<PointLight> createPointLight(OGLEFltkApplication *app,
    const Vec3f &pos=Vec3f(-4.0f, 1.0f, 0.0f),
    const Vec3f &diffuse=Vec3f(0.1f, 0.2f, 0.95f),
    const Vec2f &radius=Vec2f(7.5,8.0));

ref_ptr<SpotLight> createSpotLight(OGLEFltkApplication *app,
    const Vec3f &pos=Vec3f(0.0f,6.5f,0.0f),
    const Vec3f &dir=Vec3f(0.0001f,-1.0f,0.0001f),
    const Vec3f &diffuse=Vec3f(0.95f,0.0f,0.0f),
    const Vec2f &radius=Vec2f(8.5f,10.5f),
    const Vec2f &coneAngles=Vec2f(34.0f,35.0f));

ref_ptr<DirectionalShadowMap> createSunShadow(
    const ref_ptr<DynamicSky> &sky,
    const ref_ptr<PerspectiveCamera> &cam,
    const ref_ptr<Frustum> &frustum,
    const GLuint shadowMapSize=1024,
    const GLuint numLayer=3,
    const GLfloat shadowSplitWeight=0.5,
    const GLenum internalFormat=GL_DEPTH_COMPONENT24,
    const GLenum pixelType=GL_FLOAT);

ref_ptr<PointShadowMap> createPointShadow(
    OGLEApplication *app,
    const ref_ptr<PointLight> &l,
    const ref_ptr<PerspectiveCamera> &cam,
    const GLuint shadowMapSize=512,
    const GLenum internalFormat=GL_DEPTH_COMPONENT24,
    const GLenum pixelType=GL_FLOAT);

ref_ptr<SpotShadowMap> createSpotShadow(
    OGLEApplication *app,
    const ref_ptr<SpotLight> &l,
    const ref_ptr<PerspectiveCamera> &cam,
    const GLuint shadowMapSize=512,
    const GLenum internalFormat=GL_DEPTH_COMPONENT24,
    const GLenum pixelType=GL_FLOAT);

ref_ptr<SkyLightShaft> createSkyLightShaft(
    OGLEFltkApplication *app,
    const ref_ptr<DirectionalLight> &sun,
    const ref_ptr<Texture> &colorTexture,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root);

/////////////////////////////////////
//// Mesh Factory
/////////////////////////////////////

class AnimationRangeUpdater : public EventCallable
{
public:
  AnimationRangeUpdater(const BoneAnimRange *animRanges, GLuint numAnimationRanges)
  : EventCallable(), animRanges_(animRanges), numAnimationRanges_(numAnimationRanges) {}

  void call(EventObject *ev, void *data)
  {
    NodeAnimation *anim = (NodeAnimation*)ev;
    Vec2d newRange = animRanges_[rand()%numAnimationRanges_].range;
    anim->setAnimationIndexActive(0, newRange + Vec2d(-1.0, -1.0) );
  }

protected:
  const BoneAnimRange *animRanges_;
  GLuint numAnimationRanges_;
};

// Loads Meshes from File using Assimp. Optionally Bone animations are loaded.
list<MeshData> createAssimpMesh(
    OGLEApplication *app,
    const ref_ptr<StateNode> &root,
    const string &modelFile,
    const string &texturePath,
    const Mat4f &meshRotation,
    const Vec3f &meshTranslation,
    const BoneAnimRange *animRanges=NULL,
    GLuint numAnimationRanges=0,
    GLdouble ticksPerSecond=20.0);

void createConeMesh(OGLEApplication *app, const ref_ptr<StateNode> &root);

MeshData createBox(OGLEApplication *app, const ref_ptr<StateNode> &root);

ref_ptr<MeshState> createSphere(OGLEApplication *app, const ref_ptr<StateNode> &root);

ref_ptr<MeshState> createQuad(OGLEApplication *app, const ref_ptr<StateNode> &root);

MeshData createFloorMesh(OGLEFltkApplication *app,
    const ref_ptr<StateNode> &root,
    const GLfloat &height=-2.0,
    const Vec3f &posScale=Vec3f(20.0f),
    const Vec2f &texcoScale=Vec2f(10.0f),
    TransferTexco transferMode=TRANSFER_TEXCO_PARALLAX,
    GLboolean useTess=GL_FALSE);

ref_ptr<MeshState> createReflectionSphere(
    OGLEFltkApplication *app,
    const ref_ptr<TextureCube> &reflectionMap,
    const ref_ptr<StateNode> &root);

/////////////////////////////////////
//// GUI Factory
/////////////////////////////////////

// Creates GUI widgets displaying the current FPS
void createFPSWidget(OGLEApplication *app, const ref_ptr<StateNode> &root);

void createTextureWidget(
    OGLEApplication *app,
    const ref_ptr<StateNode> &root,
    const ref_ptr<Texture> &tex,
    const Vec2ui &pos=Vec2ui(0u),
    const GLfloat &size=100.0f);

// Creates root node for states rendering the HUD
ref_ptr<StateNode> createHUD(OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment);

// Creates root node for states rendering the HUD
ref_ptr<StateNode> createHUD(OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    GLenum baseAttachment);

}

#endif /* FACTORY_H_ */

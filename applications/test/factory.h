/*
 * factory.h
 *
 *  Created on: 17.02.2013
 *      Author: daniel
 */

#ifndef FACTORY_H_
#define FACTORY_H_

#include <applications/qt/qt-application.h>
#include <regen/config.h>

#include <regen/utility/font.h>

#include <regen/states/state-configurer.h>

#include <regen/meshes/mesh-state.h>
#include <regen/meshes/rectangle.h>
#include <regen/meshes/sky.h>
#include <regen/meshes/box.h>
#include <regen/meshes/sphere.h>
#include <regen/meshes/texture-mapped-text.h>
#include <regen/meshes/particle-cloud.h>
#include <regen/meshes/assimp-importer.h>

#include <regen/states/fbo-state.h>
#include <regen/states/blend-state.h>
#include <regen/states/blit-state.h>
#include <regen/states/material-state.h>
#include <regen/states/texture-state.h>
#include <regen/states/shader-state.h>
#include <regen/states/depth-state.h>
#include <regen/states/blend-state.h>
#include <regen/states/atomic-states.h>
#include <regen/states/depth-of-field.h>
#include <regen/states/tonemap.h>
#include <regen/states/tesselation-state.h>
#include <regen/states/picking.h>

#include <regen/shading/volumetric-fog.h>
#include <regen/shading/distance-fog.h>
#include <regen/shading/shading-deferred.h>
#include <regen/shading/shadow-map.h>
#include <regen/shading/t-buffer.h>

#include <regen/states/filter.h>

#include <regen/textures/texture-loader.h>

#include <regen/animations/animation-manager.h>
#include <regen/animations/mesh-animation.h>
#include <regen/animations/camera-manipulator.h>

namespace regen {

struct BoneAnimRange {
  BoneAnimRange(const string &n, const Vec2d &r)
  : name(n), range(r) {}
  string name;
  Vec2d range;
};
struct MeshData {
  ref_ptr<Mesh> mesh_;
  ref_ptr<ShaderState> shader_;
  ref_ptr<StateNode> node_;
  ref_ptr<Material> material_;
};

class SortByModelMatrix : public State
{
public:
  SortByModelMatrix(const ref_ptr<StateNode> &n, const ref_ptr<Camera> &cam, GLboolean frontToBack)
  : State(), n_(n), comparator_(cam,frontToBack) {}

  virtual void enable(RenderState *state) {
    n_->childs().sort(comparator_);
  }
protected:
  ref_ptr<StateNode> n_;
  NodeEyeDepthComparator comparator_;
};

// create application window and set up OpenGL
ref_ptr<QtApplication> initApplication(
    int argc, char** argv, const string &windowTitle="regen -- real-time graphics engine");

// Blits fbo attachment to screen
void setBlitToScreen(
    QtApplication *app,
    const ref_ptr<FBO> &fbo,
    const ref_ptr<Texture> &texture,
    GLenum attachment);
void setBlitToScreen(
    QtApplication *app,
    const ref_ptr<FBO> &fbo,
    GLenum attachment);

ref_ptr<TextureCube> createStaticReflectionMap(
    QtApplication *app,
    const string &file,
    const GLboolean flipBackFace,
    const GLenum textureFormat,
    const GLfloat aniso=2.0f);

PickingGeom* createPicker(QtApplication *app,
    const ref_ptr<Camera> &camera,
    GLdouble interval=50.0);

/////////////////////////////////////
//// Camera
/////////////////////////////////////

ref_ptr<LookAtCameraManipulator> createLookAtCameraManipulator(
    QtApplication *app,
    const ref_ptr<Camera> &cam,
    const GLfloat &scrollStep=2.0f,
    const GLfloat &stepX=0.02f,
    const GLfloat &stepY=0.001f,
    const GLuint &interval=10);

ref_ptr<EgoCameraManipulator> createEgoCameraManipulator(
    QtApplication *app, const ref_ptr<Camera> &cam,
    GLfloat moveSpeed=0.05f, GLfloat mouseSensitivity=0.005f);

ref_ptr<Camera> createPerspectiveCamera(
    QtApplication *app,
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
    QtApplication *app,
    GLfloat gBufferScaleW=1.0,
    GLfloat gBufferScaleH=1.0,
    GLenum colorBufferFormat=GL_RGBA,
    GLenum depthFormat=GL_DEPTH_COMPONENT24);

ref_ptr<TBuffer> createTBuffer(
    QtApplication *app,
    const ref_ptr<Camera> &cam,
    const ref_ptr<Texture> &depthTexture,
    TBuffer::Mode mode=TBuffer::MODE_FRONT_TO_BACK,
    GLfloat tBufferScaleW=1.0,
    GLfloat tBufferScaleH=1.0);

/////////////////////////////////////
//// Post Passes
/////////////////////////////////////

// Creates root node for states rendering the background of the scene
ref_ptr<StateNode> createPostPassNode(
    QtApplication *app,
    const ref_ptr<FBO> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment);

ref_ptr<FilterSequence> createBlurState(
    QtApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<StateNode> &root,
    GLint size, GLfloat sigma,
    GLboolean downsampleTwice=GL_FALSE,
    const string &treePath="");

ref_ptr<DepthOfField> createDoFState(
    QtApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput,
    const ref_ptr<Texture> &depthInput,
    const ref_ptr<StateNode> &root);

ref_ptr<Tonemap> createTonemapState(
    QtApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput,
    const ref_ptr<StateNode> &root);

ref_ptr<FullscreenPass> createAAState(
    QtApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<StateNode> &root);

/////////////////////////////////////
//// Background/Foreground States
/////////////////////////////////////

// Creates root node for states rendering the background of the scene
ref_ptr<StateNode> createBackground(
    QtApplication *app,
    const ref_ptr<FBO> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment);

// Creates sky box mesh
ref_ptr<SkyScattering> createSky(QtApplication *app, const ref_ptr<StateNode> &root);

ref_ptr<SkyBox> createSkyCube(
    QtApplication *app,
    const ref_ptr<TextureCube> &reflectionMap,
    const ref_ptr<StateNode> &root);

ref_ptr<ParticleSnow> createParticleFog(
    QtApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root,
    GLuint numSnowFlakes = 5000);

ref_ptr<ParticleRain> createRain(
    QtApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root,
    GLuint numParticles=5000);

ref_ptr<VolumetricFog> createVolumeFog(
    QtApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<Texture> &tBufferColor,
    const ref_ptr<Texture> &tBufferDepth,
    const ref_ptr<StateNode> &root,
    GLboolean useShadowMapping=GL_FALSE);
ref_ptr<VolumetricFog> createVolumeFog(
    QtApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root,
    GLboolean useShadowMapping=GL_FALSE);

ref_ptr<DistanceFog> createDistanceFog(
    QtApplication *app,
    const Vec3f &fogColor,
    const ref_ptr<TextureCube> &skyColor,
    const ref_ptr<Texture> &gDepth,
    const ref_ptr<Texture> &tBufferColor,
    const ref_ptr<Texture> &tBufferDepth,
    const ref_ptr<StateNode> &root);
ref_ptr<DistanceFog> createDistanceFog(
    QtApplication *app,
    const Vec3f &fogColor,
    const ref_ptr<TextureCube> &skyColor,
    const ref_ptr<Texture> &gDepth,
    const ref_ptr<StateNode> &root);

/////////////////////////////////////
//// Shading States
/////////////////////////////////////

// Creates deferred shading state and add to render tree
ref_ptr<DeferredShading> createShadingPass(
    QtApplication *app,
    const ref_ptr<FBO> &gBuffer,
    const ref_ptr<StateNode> &root,
    ShadowMap::FilterMode shadowFiltering=ShadowMap::FILTERING_NONE,
    GLboolean useAmbientLight=GL_TRUE);

ref_ptr<Light> createPointLight(QtApplication *app,
    const Vec3f &pos=Vec3f(-4.0f, 1.0f, 0.0f),
    const Vec3f &diffuse=Vec3f(0.1f, 0.2f, 0.95f),
    const Vec2f &radius=Vec2f(7.5,8.0));

ref_ptr<Light> createSpotLight(QtApplication *app,
    const Vec3f &pos=Vec3f(0.0f,6.5f,0.0f),
    const Vec3f &dir=Vec3f(0.0001f,-1.0f,0.0001f),
    const Vec3f &diffuse=Vec3f(0.95f,0.0f,0.0f),
    const Vec2f &radius=Vec2f(8.5f,10.5f),
    const Vec2f &coneAngles=Vec2f(34.0f,35.0f));

ref_ptr<ShadowMap> createShadow(
    QtApplication *app,
    const ref_ptr<Light> &light,
    const ref_ptr<Camera> &cam,
    ShadowMap::Config cfg);

/////////////////////////////////////
//// Mesh Factory
/////////////////////////////////////

class AnimationRangeUpdater : public EventHandler
{
public:
  AnimationRangeUpdater(const ref_ptr<NodeAnimation> &anim,
      const BoneAnimRange *animRanges, GLuint numAnimationRanges)
  : EventHandler(), anim_(anim), animRanges_(animRanges), numAnimationRanges_(numAnimationRanges) {}

  void call(EventObject *ev, EventData *data)
  {
    NodeAnimation *anim = (NodeAnimation*)ev;
    Vec2d newRange = animRanges_[rand()%numAnimationRanges_].range;
    anim->setAnimationIndexActive(0, newRange + Vec2d(-1.0, -1.0) );
  }

protected:
  ref_ptr<NodeAnimation> anim_;
  const BoneAnimRange *animRanges_;
  GLuint numAnimationRanges_;
};

// Loads Meshes from File using Assimp. Optionally Bone animations are loaded.
list<MeshData> createAssimpMesh(
    QtApplication *app,
    const ref_ptr<StateNode> &root,
    const string &modelFile,
    const string &texturePath,
    const Mat4f &meshRotation,
    const Vec3f &meshTranslation,
    const Mat4f &meshScaling,
    const BoneAnimRange *animRanges=NULL,
    GLuint numAnimationRanges=0,
    GLdouble ticksPerSecond=20.0,
    const string &shaderKey="mesh");

void createConeMesh(QtApplication *app, const ref_ptr<StateNode> &root);

MeshData createBox(QtApplication *app, const ref_ptr<StateNode> &root);

ref_ptr<Mesh> createSphere(QtApplication *app, const ref_ptr<StateNode> &root);

ref_ptr<Mesh> createQuad(QtApplication *app, const ref_ptr<StateNode> &root);

MeshData createFloorMesh(QtApplication *app,
    const ref_ptr<StateNode> &root,
    const GLfloat &height=-2.0,
    const Vec3f &posScale=Vec3f(20.0f),
    const Vec2f &texcoScale=Vec2f(10.0f),
    TextureState::TransferTexco transferMode=TextureState::TRANSFER_TEXCO_PARALLAX,
    GLboolean useTess=GL_FALSE);

ref_ptr<Mesh> createReflectionSphere(
    QtApplication *app,
    const ref_ptr<TextureCube> &reflectionMap,
    const ref_ptr<StateNode> &root);

/////////////////////////////////////
//// GUI Factory
/////////////////////////////////////

// Creates GUI widgets displaying the current FPS
Animation* createFPSWidget(QtApplication *app, const ref_ptr<StateNode> &root);

void createLogoWidget(QtApplication *app, const ref_ptr<StateNode> &root);

void createTextureWidget(
    QtApplication *app,
    const ref_ptr<StateNode> &root,
    const ref_ptr<Texture> &tex,
    const Vec2ui &pos=Vec2ui(0u),
    const GLfloat &size=100.0f);

// Creates root node for states rendering the HUD
ref_ptr<StateNode> createHUD(QtApplication *app,
    const ref_ptr<FBO> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment);

// Creates root node for states rendering the HUD
ref_ptr<StateNode> createHUD(QtApplication *app,
    const ref_ptr<FBO> &fbo,
    GLenum baseAttachment);

}

#endif /* FACTORY_H_ */

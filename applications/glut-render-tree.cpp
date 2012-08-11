/*
 * glut-render-tree.cpp
 *
 *  Created on: 10.08.2012
 *      Author: daniel
 */

#ifndef GLUT_RENDER_TREE_CPP_
#define GLUT_RENDER_TREE_CPP_

#include "glut-render-tree.h"

#include <ogle/states/blit-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/font/font-manager.h>
#include <ogle/animations/animation-manager.h>

class UpdateFPS : public Animation
{
public:
  UpdateFPS(ref_ptr<Text> fpsText)
  : Animation(),
    fpsText_(fpsText),
    numFrames_(0),
    sumDtMiliseconds_(0.0f)
  {

  }
  virtual void animate(GLdouble milliSeconds)
  {
  }
  virtual void updateGraphics(GLdouble dt)
  {
    numFrames_ += 1;
    sumDtMiliseconds_ += dt*1000.0;

    if (sumDtMiliseconds_ > 1000.0) {
      fps_ = (int) (numFrames_*1000.0/sumDtMiliseconds_);
      sumDtMiliseconds_ = 0;
      numFrames_ = 0;

      wstringstream ss;
      ss << fps_ << " FPS";
      fpsText_->set_value(ss.str());
    }
  }
private:
  ref_ptr<Text> fpsText_;
  unsigned int numFrames_;
  int fps_;
  double sumDtMiliseconds_;
};

GlutRenderTree::GlutRenderTree(
    int argc, char** argv,
    const string &windowTitle,
    GLuint windowWidth,
    GLuint windowHeight,
    ref_ptr<RenderTree> renderTree,
    ref_ptr<RenderState> renderState)
: GlutApplication(argc, argv, windowTitle, windowWidth, windowHeight),
  renderTree_(renderTree),
  renderState_(renderState)
{
  if(renderState_.get()==NULL) {
    renderState_ = ref_ptr<RenderState>::manage(new RenderState);
  }
  if(renderTree_.get()==NULL) {
    renderTree_ = ref_ptr<RenderTree>::manage(new RenderTree);
  }
  globalStates_ = renderTree_->rootNode();
  timeDelta_ = ref_ptr<UniformFloat>::manage(new UniformFloat("deltaT"));
  timeDelta_->set_value(0.0f);
  globalStates_->state()->joinUniform(ref_ptr<Uniform>::cast(timeDelta_));

  perspectiveCamera_ = ref_ptr<PerspectiveCamera>::manage(new PerspectiveCamera);
  perspectivePass_ = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(perspectiveCamera_)));

  orthogonalCamera_ = ref_ptr<OrthoCamera>::manage(new OrthoCamera);
  orthogonalPass_ = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(orthogonalCamera_)));
}

ref_ptr<StateNode>& GlutRenderTree::globalStates()
{
  return globalStates_;
}
ref_ptr<StateNode>& GlutRenderTree::perspectivePass()
{
  return perspectivePass_;
}
ref_ptr<StateNode>& GlutRenderTree::orthogonalPass()
{
  return orthogonalPass_;
}
ref_ptr<PerspectiveCamera>& GlutRenderTree::perspectiveCamera()
{
  return perspectiveCamera_;
}
ref_ptr<OrthoCamera>& GlutRenderTree::orthogonalCamera()
{
  return orthogonalCamera_;
}
ref_ptr<RenderTree>& GlutRenderTree::renderTree()
{
  return renderTree_;
}
ref_ptr<RenderState>& GlutRenderTree::renderState()
{
  return renderState_;
}

void GlutRenderTree::addRootNodeVBO(GLuint sizeMB)
{
  renderTree_->addVBONode(globalStates_, sizeMB);
}
void GlutRenderTree::addPerspectiveVBO(GLuint sizeMB)
{
  if(perspectivePass_->parent().get() == NULL) {
    usePerspectivePass();
  }
  renderTree_->addVBONode(perspectivePass_, sizeMB);
}
void GlutRenderTree::addOrthoVBO(GLuint sizeMB)
{
  if(orthogonalPass_->parent().get() == NULL) {
    useOrthogonalPass();
  }
  renderTree_->addVBONode(orthogonalPass_, sizeMB);
}

void GlutRenderTree::usePerspectivePass()
{
  renderTree_->addChild(globalStates_, perspectivePass_, false);
}
void GlutRenderTree::useOrthogonalPass()
{
  renderTree_->addChild(globalStates_, orthogonalPass_, false);
}
void GlutRenderTree::setBlitToScreen(
    ref_ptr<FrameBufferObject> fbo,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitToScreen(fbo, attachment));
  ref_ptr<StateNode> blitNode = ref_ptr<StateNode>::manage(
      new StateNode(blitState));
  globalStates_->addChild(blitNode);
}
void GlutRenderTree::setClearScreenColor(const Vec4f &clearColor)
{
  ref_ptr<ClearColorState> clearScreenColor =
      ref_ptr<ClearColorState>::manage(new ClearColorState);
  ClearColorData clearData;
  clearData.clearColor = clearColor;
  clearData.colorAttachment = GL_FRONT;
  clearScreenColor->data.push_back(clearData);
  globalStates_->state()->addEnabler(ref_ptr<Callable>::cast(clearScreenColor));
}
void GlutRenderTree::setClearScreenDepth()
{
  ref_ptr<Callable> clearScreenDepth =
      ref_ptr<Callable>::manage(new ClearDepthState);
  globalStates_->state()->addEnabler(clearScreenDepth);
}

void GlutRenderTree::setLight()
{
  setLight(ref_ptr<Light>::manage(new Light));
}
void GlutRenderTree::setLight(ref_ptr<Light> light)
{
  perspectivePass_->state()->joinStates(ref_ptr<State>::cast(light));
}

ref_ptr<Texture> GlutRenderTree::setRenderToTexture(
    GLuint width,
    GLuint height,
    GLenum colorAttachmentFormat,
    GLenum depthAttachmentFormat,
    GLboolean clearDepthBuffer,
    GLboolean clearColorBuffer,
    const Vec4f &clearColor)
{
  ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(
          width, height,
          colorAttachmentFormat,
          depthAttachmentFormat));
  ref_ptr<Texture> colorBuffer = fbo->addTexture(1);
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  GLenum colorAttachment = GL_COLOR_ATTACHMENT0;
  if(clearDepthBuffer) {
    fboState->setClearDepth();
  }
  if(clearColorBuffer) {
    ClearColorData colorData;
    colorData.clearColor = clearColor;
    colorData.colorAttachment = colorAttachment;
    fboState->setClearColor(colorData);
  }

  // call glDrawBuffer for GL_COLOR_ATTACHMENT0
  ref_ptr<ShaderFragmentOutput> output =
      ref_ptr<ShaderFragmentOutput>::manage(new DefaultFragmentOutput);
  output->set_colorAttachment(colorAttachment);
  fboState->addDrawBuffer(output);

  perspectivePass_->state()->joinStates(ref_ptr<State>::cast(fboState));
  return colorBuffer;
}
ref_ptr<FBOState> GlutRenderTree::setRenderToTexture(
    ref_ptr<FrameBufferObject> fbo,
    GLboolean clearDepthBuffer,
    GLboolean clearColorBuffer,
    const Vec4f &clearColor)
{
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  GLenum colorAttachment = GL_COLOR_ATTACHMENT0;
  if(clearDepthBuffer) {
    fboState->setClearDepth();
  }
  if(clearColorBuffer) {
    ClearColorData colorData;
    colorData.clearColor = clearColor;
    colorData.colorAttachment = colorAttachment;
    fboState->setClearColor(colorData);
  }

  // call glDrawBuffer for GL_COLOR_ATTACHMENT0
  ref_ptr<ShaderFragmentOutput> output =
      ref_ptr<ShaderFragmentOutput>::manage(new DefaultFragmentOutput);
  output->set_colorAttachment(colorAttachment);
  fboState->addDrawBuffer(output);

  perspectivePass_->state()->joinStates(ref_ptr<State>::cast(fboState));

  return fboState;
}

ref_ptr<StateNode> GlutRenderTree::addMesh(
    ref_ptr<AttributeState> mesh,
    ref_ptr<ModelTransformationState> modelTransformation,
    ref_ptr<Material> material,
    GLboolean generateShader,
    GLboolean generateVBO)
{
  if(perspectivePass_->parent().get() == NULL) {
    usePerspectivePass();
  }
  return addMesh(
      perspectivePass_,
      mesh,
      modelTransformation,
      material,
      generateShader,
      generateVBO);
}

ref_ptr<StateNode> GlutRenderTree::addOrthoMesh(
    ref_ptr<AttributeState> mesh,
    ref_ptr<ModelTransformationState> modelTransformation,
    ref_ptr<Material> material,
    GLboolean generateShader,
    GLboolean generateVBO)
{
  if(orthogonalPass_->parent().get() == NULL) {
    useOrthogonalPass();
  }
  return addMesh(
      orthogonalPass_,
      mesh,
      modelTransformation,
      material,
      generateShader,
      generateVBO);
}

ref_ptr<StateNode> GlutRenderTree::addMesh(
    ref_ptr<StateNode> parent,
    ref_ptr<AttributeState> mesh,
    ref_ptr<ModelTransformationState> modelTransformation,
    ref_ptr<Material> material,
    GLboolean generateShader,
    GLboolean generateVBO)
{
  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  ref_ptr<ShaderState> shaderState =
      ref_ptr<ShaderState>::manage(new ShaderState);
  ref_ptr<StateNode> materialNode, shaderNode, modeltransformNode;

  ref_ptr<StateNode> *root = &meshNode;
  if(generateShader) {
    shaderNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(shaderState)));
    shaderNode->addChild(*root);
    (*root)->set_parent(shaderNode);
    root = &shaderNode;
  }
  if(modelTransformation.get()!=NULL) {
    modeltransformNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(modelTransformation)));
    modeltransformNode->addChild(*root);
    (*root)->set_parent(modeltransformNode);
    root = &modeltransformNode;
  }
  if(material.get()!=NULL) {
    materialNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(material)));
    materialNode->addChild(*root);
    (*root)->set_parent(materialNode);
    root = &materialNode;
  }

  renderTree_->addChild(parent, *root, generateVBO);

  if(generateShader) {
    ref_ptr<Shader> shader = renderTree_->generateShader(*meshNode.get());
    shaderState->set_shader(shader);
  }

  return *root;
}

void GlutRenderTree::setShowFPS()
{
  const Vec4f fpsColor = Vec4f(1.0f);
  const Vec4f fpsBackgroundColor = Vec4f(0.0f, 0.0f, 0.0f, 0.5f);
  FreeTypeFont& font = FontManager::get().getFont(
                  "demos/arial.ttf",
                  12, // font size in pixel
                  fpsColor, //
                  fpsBackgroundColor,
                  0, // glyph rotation in degree
                  GL_LINEAR, // filter mode
                  false, // use mipmaps?
                  96); // dpi

  fpsText_ = ref_ptr<Text>::manage(new Text(font, 12.0, true, true));
  fpsText_->set_value(L"0 FPS");

  // TODO: modelMat needed for ortho objects ??
  ref_ptr<ModelTransformationState> modelTransformation =
      ref_ptr<ModelTransformationState>::manage(new ModelTransformationState);
  modelTransformation->translate( Vec3f( 2.0, 18.0, 0.0 ), 0.0f );
  addOrthoMesh(ref_ptr<AttributeState>::cast(fpsText_), modelTransformation);

  updateFPS_ = ref_ptr<Animation>::manage(new UpdateFPS(fpsText_));
  AnimationManager::get().addAnimation(updateFPS_);
}

void GlutRenderTree::render(GLdouble dt)
{
  renderTree_->traverse(renderState_.get());
}
void GlutRenderTree::postRender(GLdouble dt)
{
  renderTree_->updateStates(dt);
}

#endif /* GLUT_RENDER_TREE_CPP_ */

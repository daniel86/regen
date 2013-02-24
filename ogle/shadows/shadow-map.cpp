/*
 * shadow-map.cpp
 *
 *  Created on: 24.11.2012
 *      Author: daniel
 */

#include "shadow-map.h"

#include <ogle/states/polygon-offset-state.h>
#include <ogle/states/cull-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/states/fbo-state.h>
#include <ogle/states/shader-configurer.h>
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/shadows/directional-shadow-map.h>
#include <ogle/filter/blur.h>

static void traverseTree(RenderState *rs, StateNode *node)
{
  if(rs->isNodeHidden(node)) { return; }

  node->enable(rs);
  for(list< ref_ptr<StateNode> >::iterator
      it=node->childs().begin(); it!=node->childs().end(); ++it)
  {
    traverseTree(rs, it->get());
  }
  node->disable(rs);
}

GLboolean DepthRenderState::isStateHidden(State *state)
{
  return (
      dynamic_cast<DepthState*>(state)!=NULL ||
      dynamic_cast<CullDisableState*>(state)!=NULL ||
      dynamic_cast<CullEnableState*>(state)!=NULL ||
      state->isHidden());
}

///////////
//////////

// TODO: SHADOW: direct shading shadows

ShadowMap::ShadowMap(
    const ref_ptr<Light> &light,
    GLenum shadowMapTarget,
    GLuint shadowMapSize,
    GLuint shadowMapDepth,
    GLenum depthFormat,
    GLenum depthType)
: Animation(), State(), light_(light)
{
  // TODO: SHADOW: no inverse matrices provided
  // TODO: SHADOW: viewport uniform not right during traversal
  depthFBO_ = ref_ptr<FrameBufferObject>::manage( new FrameBufferObject(
      shadowMapSize,shadowMapSize,shadowMapDepth,
      shadowMapTarget,depthFormat,depthType));
  depthTexture_ = depthFBO_->depthTexture();
  depthTexture_->set_wrapping(GL_CLAMP_TO_EDGE);
  depthTexture_->set_filter(GL_NEAREST,GL_NEAREST);
  depthTexture_->set_compare(GL_COMPARE_R_TO_TEXTURE, GL_LEQUAL);

  depthTextureState_ = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(depthTexture_)));
  depthTextureState_->set_name("inputTexture");
  depthTextureState_->set_mapping(MAPPING_CUSTOM);
  depthTextureState_->setMapTo(MAP_TO_CUSTOM);

  textureQuad_ = ref_ptr<MeshState>::cast(Rectangle::getUnitQuad());

  shadowMapSizeUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("shadowMapSize"));
  shadowMapSizeUniform_->setUniformData((GLfloat)shadowMapSize);
  joinShaderInput(ref_ptr<ShaderInput>::cast(shadowMapSizeUniform_));

  depthTextureSize_ = shadowMapSize;
  depthTextureDepth_ = shadowMapDepth;

  // avoid shadow acne
  setCullFrontFaces(GL_TRUE);
}

///////////
///////////

void ShadowMap::setPolygonOffset(GLfloat factor, GLfloat units)
{
  if(polygonOffsetState_.get()) {
    disjoinStates(polygonOffsetState_);
  }
  polygonOffsetState_ = ref_ptr<State>::manage(new PolygonOffsetState(factor,units));
  joinStatesFront(polygonOffsetState_);
}

void ShadowMap::setCullFrontFaces(GLboolean v)
{
  if(cullState_.get()) {
    disjoinStates(cullState_);
  }
  if(v) {
    cullState_ = ref_ptr<State>::manage(new CullFrontFaceState);
    joinStatesFront(cullState_);
  } else {
    cullState_ = ref_ptr<State>();
  }
}

///////////
///////////

const ref_ptr<Texture>& ShadowMap::shadowDepth() const
{
  return depthTexture_;
}
void ShadowMap::set_depthFormat(GLenum f)
{
  depthTexture_->bind();
  depthTexture_->set_internalFormat(f);
  depthTexture_->texImage();
}
void ShadowMap::set_depthType(GLenum t)
{
  depthTexture_->bind();
  depthTexture_->set_pixelType(t);
  depthTexture_->texImage();
}
void ShadowMap::set_depthSize(GLuint shadowMapSize)
{
  depthTextureSize_ = shadowMapSize;

  depthFBO_->resize(depthTextureSize_,depthTextureSize_,depthTextureDepth_);
  if(momentsTexture_.get()) {
    momentsFBO_->resize(depthTextureSize_,depthTextureSize_,depthTextureDepth_);
  }
  if(momentsFilter_.get()) {
    momentsFilter_->resize();
  }
}

///////////
///////////

const ref_ptr<Texture>& ShadowMap::shadowMomentsUnfiltered() const
{
  return momentsTexture_;
}
const ref_ptr<Texture>& ShadowMap::shadowMoments() const
{
  // return filtered moments texture
  return momentsFilter_->output();
}
const ref_ptr<FilterSequence>& ShadowMap::momentsFilter() const
{
  return momentsFilter_;
}
void ShadowMap::setComputeMoments()
{
  if(momentsCompute_.get()) { return; }

  momentsFBO_ = ref_ptr<FrameBufferObject>::manage(new FrameBufferObject(
      depthTextureSize_,depthTextureSize_,depthTextureDepth_,
      GL_NONE,GL_NONE,GL_NONE));
  momentsFBO_->bind();
  momentsTexture_ = momentsFBO_->addTexture(1,
      depthTexture_->targetType(),
      GL_RGBA, GL_RGBA, GL_BYTE);
      //GL_RGBA, GL_RGBA32F, GL_FLOAT);
  momentsTexture_->set_wrapping(GL_REPEAT);
  momentsTexture_->set_filter(GL_LINEAR,GL_LINEAR);

  momentsCompute_ = ref_ptr<ShaderState>::manage(new ShaderState);
  ShaderConfigurer cfg;
  cfg.addState(depthTextureState_.get());
  cfg.addState(textureQuad_.get());
  switch(samplerType()) {
  case GL_TEXTURE_CUBE_MAP:
    cfg.define("IS_CUBE_SHADOW", "TRUE");
    break;
  case GL_TEXTURE_2D_ARRAY: {
    Texture3D *depth = (Texture3D*) depthTexture_.get();
    cfg.define("IS_ARRAY_SHADOW", "TRUE");
    cfg.define("NUM_SHADOW_LAYER", FORMAT_STRING(depth->depth()));
    break;
  }
  default:
    cfg.define("IS_2D_SHADOW", "TRUE");
    break;
  }
  momentsCompute_->createShader(cfg.cfg(), "shadow_mapping.moments");
  momentsLayer_ = momentsCompute_->shader()->uniformLocation("shadowLayer");
  momentsNear_ = momentsCompute_->shader()->uniformLocation("shadowNear");
  momentsFar_ = momentsCompute_->shader()->uniformLocation("shadowFar");

  momentsFilter_ = ref_ptr<FilterSequence>::manage(new FilterSequence(momentsTexture_));
}
void ShadowMap::createBlurFilter(
    GLuint size, GLfloat sigma,
    GLboolean downsampleTwice)
{
  // first downsample the moments texture
  momentsFilter_->addFilter(
      ref_ptr<Filter>::manage(new Filter("downsample", 0.5)));
  if(downsampleTwice) {
    momentsFilter_->addFilter(
        ref_ptr<Filter>::manage(new Filter("downsample", 0.5)));
  }
  // then blur the downsampled texture
  ref_ptr<BlurSeparableFilter> blurX = ref_ptr<BlurSeparableFilter>::manage(
      new BlurSeparableFilter(BlurSeparableFilter::HORITONTAL));
  ref_ptr<BlurSeparableFilter> blurY = ref_ptr<BlurSeparableFilter>::manage(
      new BlurSeparableFilter(BlurSeparableFilter::VERTICAL));
  blurX->numPixels()->setVertex1f(0, (GLfloat)size);
  blurY->numPixels()->setVertex1f(0, (GLfloat)size);
  blurX->sigma()->setVertex1f(0, sigma);
  blurY->sigma()->setVertex1f(0, sigma);
  momentsFilter_->addFilter(ref_ptr<Filter>::cast(blurX));
  momentsFilter_->addFilter(ref_ptr<Filter>::cast(blurY));
}

///////////
///////////

void ShadowMap::addCaster(const ref_ptr<StateNode> &caster)
{
  caster_.push_back(caster);
}
void ShadowMap::removeCaster(StateNode *caster)
{
  for(list< ref_ptr<StateNode> >::iterator
      it=caster_.begin(); it!=caster_.end(); ++it)
  {
    ref_ptr<StateNode> &n = *it;
    if(n.get()==caster) {
      caster_.erase(it);
      break;
    }
  }
}

void ShadowMap::traverse(RenderState *rs)
{
  for(list< ref_ptr<StateNode> >::iterator
      it=caster_.begin(); it!=caster_.end(); ++it)
  {
    traverseTree(rs, it->get());
  }
}

///////////
///////////

void ShadowMap::glAnimate(GLdouble dt)
{
  update();

  {
    depthFBO_->bind();
    depthFBO_->set_viewport();

    glDrawBuffer(GL_NONE);
    enable(&depthRenderState_);
    computeDepth();
    disable(&depthRenderState_);
  }

  // compute moments from depth texture
  if(momentsTexture_.get()) {
    momentsFBO_->bind();
    momentsFBO_->set_viewport();

    GLint *channel = depthTextureState_->channelPtr();
    *channel = filteringRenderState_.nextTexChannel();
    depthTexture_->activateBind(*channel);
    depthTexture_->set_compare(GL_NONE, GL_LEQUAL);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    // update moments texture
    computeMoment();
    // and pre filter the result
    momentsFilter_->enable(&filteringRenderState_);
    momentsFilter_->disable(&filteringRenderState_);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    depthTexture_->bind();
    depthTexture_->set_compare(GL_COMPARE_R_TO_TEXTURE, GL_LEQUAL);
    filteringRenderState_.releaseTexChannel();
  }
}
void ShadowMap::animate(GLdouble dt)
{
}
GLboolean ShadowMap::useGLAnimation() const
{
  return GL_TRUE;
}
GLboolean ShadowMap::useAnimation() const
{
  return GL_FALSE;
}

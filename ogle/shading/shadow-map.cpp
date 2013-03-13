/*
 * shadow-map.cpp
 *
 *  Created on: 24.11.2012
 *      Author: daniel
 */

#include <ogle/states/atomic-states.h>
#include <ogle/states/depth-state.h>
#include <ogle/states/fbo-state.h>
#include <ogle/states/shader-configurer.h>
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-util.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/shading/directional-shadow-map.h>

#include "shadow-map.h"
using namespace ogle;

///////////
//////////

ShadowMap::ShadowMap(
    const ref_ptr<Light> &light,
    GLenum shadowMapTarget,
    GLuint shadowMapSize,
    GLuint shadowMapDepth,
    GLenum depthFormat,
    GLenum depthType)
: ShaderInputState(), light_(light)
{
  // TODO: SHADOW: no inverse matrices provided
  depthFBO_ = ref_ptr<FrameBufferObject>::manage( new FrameBufferObject(
      shadowMapSize,shadowMapSize,shadowMapDepth,
      shadowMapTarget,depthFormat,depthType));
  depthTexture_ = depthFBO_->depthTexture();
  depthTexture_->set_wrapping(GL_CLAMP_TO_EDGE);
  depthTexture_->set_filter(GL_NEAREST,GL_NEAREST);
  depthTexture_->set_compare(GL_COMPARE_R_TO_TEXTURE, GL_LEQUAL);

  depthTextureState_ = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(depthTexture_), "inputTexture"));
  depthTextureState_->set_mapping(TextureState::MAPPING_CUSTOM);
  depthTextureState_->setMapTo(TextureState::MAP_TO_CUSTOM);

  textureQuad_ = ref_ptr<MeshState>::cast(Rectangle::getUnitQuad());

  shadowMapSizeUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("shadowMapSize"));
  shadowMapSizeUniform_->setUniformData((GLfloat)shadowMapSize);
  setInput(ref_ptr<ShaderInput>::cast(shadowMapSizeUniform_));

  depthTextureSize_ = shadowMapSize;
  depthTextureDepth_ = shadowMapDepth;

  // avoid shadow acne
  setCullFrontFaces(GL_TRUE);
}

string ShadowMap::shadowFilterMode(ShadowMap::FilterMode f)
{
  switch(f) {
  case ShadowMap::FILTERING_NONE: return "Single";
  case ShadowMap::FILTERING_PCF_GAUSSIAN: return "Gaussian";
  case ShadowMap::FILTERING_VSM: return "VSM";
  }
  return "Single";
}
GLboolean ShadowMap::useShadowMoments(ShadowMap::FilterMode f)
{
  switch(f) {
  case ShadowMap::FILTERING_VSM:
    return GL_TRUE;
  default:
    return GL_FALSE;
  }
}
GLboolean ShadowMap::useShadowSampler(ShadowMap::FilterMode f)
{
  switch(f) {
  case ShadowMap::FILTERING_VSM:
    return GL_FALSE;
  default:
    return GL_TRUE;
  }
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
    cullState_ = ref_ptr<State>::manage(new CullFaceState(GL_FRONT));
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

  momentsBlurSize_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("numBlurPixels"));
  momentsBlurSize_->setUniformData(4.0);
  momentsFilter_->joinShaderInput(ref_ptr<ShaderInput>::cast(momentsBlurSize_));

  momentsBlurSigma_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurSigma"));
  momentsBlurSigma_->setUniformData(2.0);
  momentsFilter_->joinShaderInput(ref_ptr<ShaderInput>::cast(momentsBlurSigma_));

}
void ShadowMap::createBlurFilter(
    GLuint size, GLfloat sigma,
    GLboolean downsampleTwice)
{
  // first downsample the moments texture
  momentsFilter_->addFilter(ref_ptr<Filter>::manage(new Filter("downsample", 0.5)));
  if(downsampleTwice) {
    momentsFilter_->addFilter(ref_ptr<Filter>::manage(new Filter("downsample", 0.5)));
  }
  momentsFilter_->addFilter(ref_ptr<Filter>::manage(new Filter("blur.horizontal")));
  momentsFilter_->addFilter(ref_ptr<Filter>::manage(new Filter("blur.vertical")));
  momentsBlurSize_->setVertex1f(0, size);
  momentsBlurSigma_->setVertex1f(0, sigma);
}

const ref_ptr<ShaderInput1f>& ShadowMap::momentsBlurSize() const
{
  return momentsBlurSize_;
}
const ref_ptr<ShaderInput1f>& ShadowMap::momentsBlurSigma() const
{
  return momentsBlurSigma_;
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
    RootNode::traverse(rs, it->get());
  }
}

///////////
///////////

void ShadowMap::update(RenderState *rs, GLdouble dt)
{
  update();

  {
    rs->fbo().push(depthFBO_.get());
    glDrawBuffer(GL_NONE);

    enable(rs);
    {
      // we use unmodified tree passed in. but it is not
      // allowed to push some states during shadow traversal.
      // we lock them here so pushes will not change server
      // side state.
      rs->fbo().lock();
      rs->cullFace().lock();
      rs->frontFace().lock();
      rs->colorMask().lock();
      rs->depthFunc().lock();
      rs->depthMask().lock();
      rs->depthRange().lock();
    }
    computeDepth(rs);
    {
      rs->fbo().unlock();
      rs->cullFace().unlock();
      rs->frontFace().unlock();
      rs->colorMask().unlock();
      rs->depthFunc().unlock();
      rs->depthMask().unlock();
      rs->depthRange().unlock();
    }
    disable(rs);

    rs->fbo().pop();
  }

  // compute depth moments
  if(momentsTexture_.get()) {
    rs->fbo().push(momentsFBO_.get());

    GLint *channel = depthTextureState_->channel();
    *channel = rs->reserveTextureChannel();
    depthTexture_->activate(*channel);
    depthTexture_->set_compare(GL_NONE, GL_LEQUAL);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    rs->toggles().push(RenderState::DEPTH_TEST, GL_FALSE);
    rs->depthMask().push(GL_FALSE);

    // update moments texture
    computeMoment(rs);
    // and filter the result
    momentsFilter_->enable(rs);
    momentsFilter_->disable(rs);

    depthTexture_->bind();
    depthTexture_->set_compare(GL_COMPARE_R_TO_TEXTURE, GL_LEQUAL);

    rs->releaseTextureChannel();
    rs->toggles().pop(RenderState::DEPTH_TEST);
    rs->depthMask().pop();
    rs->fbo().pop();
  }
}

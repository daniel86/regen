/*
 * deferred-shading.cpp
 *
 *  Created on: 09.11.2012
 *      Author: daniel
 */

#include <ogle/utility/gl-error.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/states/depth-state.h>
#include <ogle/render-tree/shader-configurer.h>
#include "shading-deferred.h"

class GBufferUpdate : public StateNode
{
public:
  GBufferUpdate()
  : StateNode()
  {
    state_->shaderDefine("USE_DEFERRED_SHADING", "TRUE");
  }
};

#ifdef USE_AMBIENT_OCCLUSION
class AmbientOcclusion : public StateNode
{
public:
  AmbientOcclusion(
      ref_ptr<State> orthoQuad,
      ref_ptr<FrameBufferObject> &gbuffer,
      list<GBufferTarget> &outputTargets)
  : StateNode(),
    occlusionShader_( ref_ptr<ShaderState>::manage(new ShaderState) ),
    outputTargets_(outputTargets),
    gbuffer_(gbuffer)
  {
    state_ = ref_ptr<State>::cast(occlusionShader_);
    state_->joinStates(orthoQuad);

    ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(new FrameBufferObject(
            gbuffer->width()/2,gbuffer->height()/2,GL_NONE));
    framebuffer_ = ref_ptr<FBOState>::manage(new FBOState(fbo));
    framebuffer_->addDrawBuffer(GL_COLOR_ATTACHMENT0);
    //state_->joinStates(ref_ptr<State>::cast(framebuffer_));

    // not sure why nvidia refuses to use GL_R as format
    aoTexture_ = framebuffer_->fbo()->addTexture(1, GL_RGB, GL_R8);

    handleFBOError("AmbientOcclusion");

    randNor_ = ref_ptr<Texture>::manage(
        new ImageTexture("res/textures/random_normals.png"));
  }

  ref_ptr<Texture>& aoTexture() { return aoTexture_; }

  virtual void enable(RenderState *rs)
  {
    if(occlusionShader_->shader().get() == NULL) {
      ShaderConfigurer shaderConfig = ShaderConfigurer::config(occlusionShader_);
      occlusionShader_->createShader(shaderConfig, "deferred-shading.ssao");
      // add textures to shader
      occlusionShader_->shader()->setTexture(randNor_, "randomNormalTexture");
      list< ref_ptr<Texture> > &outputs = gbuffer_->colorBuffer();
      list< GBufferTarget >::iterator it1=outputTargets_.begin();
      list< ref_ptr<Texture> >::iterator it2=outputs.begin();
      while(it1!=outputTargets_.end()) {
        GBufferTarget &target = *it1;
        occlusionShader_->shader()->setTexture(*it2, target.name + "Texture");
        ++it1;
        ++it2;
      }
    }

    GLuint channel = rs->nextTexChannel();
    glActiveTexture(GL_TEXTURE0 + channel);
    randNor_->bind();
    randNor_->set_channel(channel);

    // switch to downscaled fbo
    framebuffer_->enable(rs);
    StateNode::enable(rs);
  }
  virtual void disable(RenderState *rs)
  {
    StateNode::disable(rs);
    framebuffer_->disable(rs);
    rs->releaseTexChannel();
  }
  ref_ptr<FrameBufferObject> &gbuffer_;
  list<GBufferTarget> &outputTargets_;
  ref_ptr<FBOState> framebuffer_;
  ref_ptr<ShaderState> occlusionShader_;
  ref_ptr<Texture> randNor_;
  ref_ptr<Texture> aoTexture_;
};
#endif

class AccumulateLight : public StateNode
{
public:
  AccumulateLight(
      const ref_ptr<State> &orthoQuad,
      const ref_ptr<FrameBufferObject> &framebuffer,
      const ref_ptr<Texture> &colorTexture,
      list<GBufferTarget> &outputTargets)
  : StateNode(),
    accumulationShader_( ref_ptr<ShaderState>::manage(new ShaderState) ),
    colorTexture_(colorTexture),
    framebuffer_(framebuffer),
    outputTargets_(outputTargets)
  {
    state_ = ref_ptr<State>::cast(accumulationShader_);

#ifdef USE_AMBIENT_OCCLUSION
    ref_ptr<AmbientOcclusion> aoStage = ref_ptr<AmbientOcclusion>::manage(
        new AmbientOcclusion(orthoQuad, framebuffer_, outputTargets_));
    aoTexture_ = aoStage->aoTexture();
    aoStage_ = ref_ptr<StateNode>::cast(aoStage);
    numInputTextures += 1;
#endif

    ref_ptr<DepthState> depthState_ = ref_ptr<DepthState>::manage(new DepthState);
    depthState_->set_useDepthTest(GL_FALSE);
    depthState_->set_useDepthWrite(GL_FALSE);
    state_->joinStates(ref_ptr<State>::cast(depthState_));

    state_->joinStates(orthoQuad);

    GLint numChannels = outputTargets.size()+1;
#ifdef USE_AMBIENT_OCCLUSION
    numChannels += 1;
#endif
    outputChannels_ = new GLint[numChannels];
  }
  ~AccumulateLight() {
    delete []outputChannels_;
  }
  virtual void enable(RenderState *rs)
  {
    if(accumulationShader_->shader().get() == NULL) {
      GLint outputIndex = 0;
      ShaderConfig shaderConfig = ShaderConfigurer::configure(this);
      accumulationShader_->createShader(shaderConfig, "deferred-shading");
      // add textures to shader
      list< ref_ptr<Texture> > &outputs = framebuffer_->colorBuffer();
      list< GBufferTarget >::iterator it1=outputTargets_.begin();
      list< ref_ptr<Texture> >::iterator it2=outputs.begin();
      while(it1!=outputTargets_.end()) {
        GBufferTarget &target = *it1;
        accumulationShader_->shader()->setTexture(
            &outputChannels_[++outputIndex], target.name + "Texture");
        ++it1;
        ++it2;
      }
#ifdef USE_AMBIENT_OCCLUSION
      accumulationShader_->shader()->setTexture(
          &outputChannels_[++outputIndex], "aoTexture");
#endif
      accumulationShader_->shader()->setTexture(
          &outputChannels_[++outputIndex], "depthTexture");
    }
    GLuint channel;

    GLint outputIndex = 0;
    colorTexture_->set_bufferIndex(1);
    list< ref_ptr<Texture> > &outputs = framebuffer_->colorBuffer();
    list< ref_ptr<Texture> >::iterator it;
    for(it=outputs.begin();it!=outputs.end();++it) {
      ref_ptr<Texture> &tex = *it;
      channel = rs->nextTexChannel();
      glActiveTexture(GL_TEXTURE0 + channel);
      tex->bind();
      outputChannels_[++outputIndex] = channel;
    }

#ifdef USE_AMBIENT_OCCLUSION
    // calculate ambient occlusion term
    channel = rs->nextTexChannel();
    aoStage_->traverse(rs, dt_);
    // and bind the ao texture for the accumulation pass
    glActiveTexture(GL_TEXTURE0 + channel);
    aoTexture_->bind();
    aoTexture_->set_channel(channel);
    outputChannels_[++outputIndex] = channel;
#endif

    channel = rs->nextTexChannel();
    glActiveTexture(GL_TEXTURE0 + channel);
    framebuffer_->depthTexture()->bind();
    outputChannels_[++outputIndex] = channel;

    // accumulate shading in GL_COLOR_ATTACHMENT0
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    StateNode::enable(rs);
  }
  virtual void disable(RenderState *rs)
  {
    list< ref_ptr<Texture> > &outputs = framebuffer_->colorBuffer();
    list< ref_ptr<Texture> >::iterator it;
    StateNode::disable(rs);
    colorTexture_->set_bufferIndex(0);
    for(it=outputs.begin();it!=outputs.end();++it) { rs->releaseTexChannel(); }
#ifdef USE_AMBIENT_OCCLUSION
    rs->releaseTexChannel();
#endif
    rs->releaseTexChannel();
  }
  ref_ptr<ShaderState> accumulationShader_;
  const ref_ptr<Texture> &colorTexture_;
  const ref_ptr<FrameBufferObject> &framebuffer_;
  list<GBufferTarget> &outputTargets_;
#ifdef USE_AMBIENT_OCCLUSION
  ref_ptr<StateNode> aoStage_;
  ref_ptr<Texture> aoTexture_;
#endif
  GLint *outputChannels_;
};

DeferredShading::DeferredShading(
    GLuint width, GLuint height,
    GLenum depthAttachmentFormat,
    list<GBufferTarget> outputTargets)
: StateNode(),
  outputTargets_(outputTargets)
{
  Rectangle::Config quadCfg;
  quadCfg.isNormalRequired = GL_FALSE;
  quadCfg.isTangentRequired = GL_FALSE;
  quadCfg.isTexcoRequired = GL_FALSE;
  quadCfg.levelOfDetail = 0;
  quadCfg.rotation = Vec3f(0.5*M_PI, 0.0f, 0.0f);
  quadCfg.posScale = Vec3f(2.0f);
  quadCfg.translation = Vec3f(-1.0f,-1.0f,0.0f);
  ref_ptr<State> orthoQuad = ref_ptr<State>::manage(new Rectangle(quadCfg));

  ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(width,height,depthAttachmentFormat));
  framebuffer_ = ref_ptr<FBOState>::manage(new FBOState(fbo));
  framebuffer_->fbo()->bind();

  depthTexture_ = ref_ptr<Texture>::cast(framebuffer_->fbo()->depthTexture());

  GLuint i=0;
  vector<GLenum> clearBuffers;
  for(list<GBufferTarget>::iterator
      it=outputTargets.begin(); it!=outputTargets.end(); ++it)
  {
    GBufferTarget &target = *it;
    GLenum buffer = GL_COLOR_ATTACHMENT0+i+1;
    if(it==outputTargets.begin()) {
      // first output target must be color buffer
      colorTexture_ = framebuffer_->fbo()->addTexture(2, target.format, target.internalFormat);
    }
    else {
      framebuffer_->fbo()->addTexture(1, target.format, target.internalFormat);
      clearBuffers.push_back(buffer);
    }
    // call glDrawBuffer for buffer
    framebuffer_->addDrawBuffer(buffer);
    ++i;
  }

  ClearColorData clearData;
  clearData.clearColor = Vec4f(0.0f);
  clearData.colorBuffers = clearBuffers;
  framebuffer_->setClearColor(clearData);

  // first render geometry and material info to GBuffer
  geometryStage_ = ref_ptr<StateNode>::manage(new GBufferUpdate);
  addChild(geometryStage_);

  // next accumulate lights
  ref_ptr<AccumulateLight> accumulationStage = ref_ptr<AccumulateLight>::manage(
      new AccumulateLight(orthoQuad, framebuffer_->fbo(), colorTexture_, outputTargets_));
  accumulationStage_ = ref_ptr<StateNode>::cast(accumulationStage);
  addChild(accumulationStage_);

  handleGLError("DeferredShading");
}

void DeferredShading::resize(GLuint w, GLuint h)
{
  framebuffer_->resize(w,h);
#ifdef USE_AMBIENT_OCCLUSION
  aoStage_->resize(w,h);
#endif
}

void DeferredShading::enable(RenderState *rs)
{
  framebuffer_->enable(rs);
  StateNode::enable(rs);
}
void DeferredShading::disable(RenderState *rs)
{
  StateNode::disable(rs);
  framebuffer_->disable(rs);
}

GLuint DeferredShading::numOutputs() const
{
  return outputTargets_.size();
}
const ref_ptr<FBOState>& DeferredShading::framebuffer() const
{
  return framebuffer_;
}
const ref_ptr<Texture>& DeferredShading::depthTexture() const
{
  return depthTexture_;
}
const ref_ptr<Texture>& DeferredShading::colorTexture() const
{
  return colorTexture_;
}
const ref_ptr<StateNode>& DeferredShading::accumulationStage() const
{
  return accumulationStage_;
}
const ref_ptr<StateNode>& DeferredShading::geometryStage() const
{
  return geometryStage_;
}

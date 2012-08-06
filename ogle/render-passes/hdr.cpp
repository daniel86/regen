/*
 * hdr.cpp
 *
 *  Created on: 23.04.2012
 *      Author: daniel
 */

#include <hdr.h>

class DownsampleHDRPass : public UnitOrthoRenderPass {
public:
  DownsampleHDRPass(
      HDRRenderer *hdr,
      ShaderFunctions &f,
      ref_ptr<FrameBufferObject> downsampledBuffer);
  virtual void render();
  virtual bool usesSceneBuffer() { return false; }
  HDRRenderer *hdr_;
  ref_ptr<FrameBufferObject> downsampledBuffer_;
};
class BlurHDRPass : public UnitOrthoRenderPass {
public:
  BlurHDRPass(HDRRenderer *hdr, ShaderFunctions &f);
  virtual bool usesSceneBuffer() { return false; }
  GLint hdrTextureUniform_;
  HDRRenderer *hdr_;
};
class BlurHDRHorizontalPass : public BlurHDRPass {
public:
  BlurHDRHorizontalPass(HDRRenderer *hdr, ShaderFunctions &f);
  virtual void render();
};
class BlurHDRVerticalPass : public BlurHDRPass {
public:
  BlurHDRVerticalPass(HDRRenderer *hdr, ShaderFunctions &f);
  virtual void render();
};
class TonemapHDRPass : public UnitOrthoRenderPass {
public:
  TonemapHDRPass(HDRRenderer *hdr, ShaderFunctions &f);
  virtual void render();
  virtual bool rendersOnTop() { return false; }
  virtual bool usesDepthTest() { return false; }
  virtual bool usesSceneBuffer() { return true; }
  GLint blurTextureUniform_;
  HDRRenderer *hdr_;
};
class ProjectionChangedHDR : public EventCallable {
public:
  ProjectionChangedHDR(HDRRenderer *g) : EventCallable(), hdr_(g) {}
  virtual void call(EventObject*,void*) { hdr_->resize(); }
  HDRRenderer *hdr_;
};

HDRRenderer::HDRRenderer(
    ref_ptr<Scene> scene,
    const HDRConfig &cfg,
    GLenum format)
: scene_(scene),
  cfg_(cfg),
  format_(format),
  projectionChangedCB_(ref_ptr<EventCallable>::manage(new ProjectionChangedHDR(this)))
{
  Vec2ui size = Vec2ui(scene->width(), scene->height());
  { // ...
    downsampledBuffer2_ = ref_ptr<FrameBufferObject>::manage(new FrameBufferObject);
    downsampledBuffer2_->bind();
    downsampledBuffer2_->set_size(size.x/2, size.y/2);
    downsampledTex2_ = ref_ptr<Texture2D>::manage(new TextureRectangle);
    downsampledTex2_->set_size(size.x/2, size.y/2);
    downsampledTex2_->set_internalFormat(format_);
    downsampledTex2_->bind();
    downsampledTex2_->set_filter(GL_LINEAR,GL_LINEAR);
    downsampledTex2_->texImage();
    downsampledBuffer2_->addColorAttachment(*downsampledTex2_.get());
  }
  {
    downsampledBuffer4_ = ref_ptr<FrameBufferObject>::manage(new FrameBufferObject);
    downsampledBuffer4_->bind();
    downsampledBuffer4_->set_size(size.x/4, size.y/4);
    downsampledTex4_ = ref_ptr<Texture2D>::manage(new TextureRectangle(2));
    downsampledTex4_->set_size(size.x/4, size.y/4);
    downsampledTex4_->set_internalFormat(format_);
    for(int i=0; i<2; ++i) {
      downsampledTex4_->bind();
      downsampledTex4_->texImage();
      downsampledTex4_->set_filter(GL_LINEAR,GL_LINEAR);
      downsampledBuffer4_->addColorAttachment(*downsampledTex4_.get());
      downsampledTex4_->nextBuffer();
    }
  }

  scene_->connect(Scene::PROJECTION_CHANGED, projectionChangedCB_);

  makeDownsamplePass(downsampledTex2_, downsampledBuffer2_);
  makeBlurPass(true, cfg_.blurCfg);
  makeBlurPass(false, cfg_.blurCfg);
  makeTonemapPass();
}

HDRRenderer::~HDRRenderer()
{
  scene_->removePostPass(downsamplePass_);
  scene_->removePostPass(blurHorizontalPass_);
  scene_->removePostPass(blurVerticalPass_);
  scene_->removePostPass(tonemapPass_);
  scene_->disconnect(projectionChangedCB_);
}

void HDRRenderer::resize()
{
  Vec2ui size = Vec2ui(scene_->width(), scene_->height());

  downsampledBuffer2_->set_size(size.x/2, size.y/2);
  downsampledTex2_->set_size(size.x/2, size.y/2);
  downsampledTex2_->bind();
  downsampledTex2_->texImage();

  downsampledBuffer4_->set_size(size.x/4, size.y/4);
  downsampledTex4_->set_size(size.x/4, size.y/4);
  for(int j=0; j<2; ++j) {
    downsampledTex4_->bind();
    downsampledTex4_->texImage();
    downsampledTex4_->nextBuffer();
  }
}

void HDRRenderer::makeDownsamplePass(
    ref_ptr<Texture2D> downsampledTex,
    ref_ptr<FrameBufferObject> downsampledBuffer)
{
  Texture *tex = downsampledTex.get();
  vector<string> args;
  args.push_back(FORMAT_STRING("gl_FragCoord.xy"));
  args.push_back("sceneTexture");
  args.push_back("fragmentColor_");
  Downsample f(args,*tex);
  downsamplePass_ = ref_ptr<RenderPass>::manage(
        new DownsampleHDRPass(this, f, downsampledBuffer));
  scene_->addPostPass(downsamplePass_);
}

void HDRRenderer::makeBlurPass(
    bool isHorizontal, const BlurConfig &blurCfg)
{
  Texture *tex = downsampledTex4_.get();
  vector<string> args;
  args.push_back(FORMAT_STRING("gl_FragCoord.xy"));
  args.push_back("hdrTexture");
  args.push_back("fragmentColor_");

  ConvolutionKernel kernel = (isHorizontal ?
      blurHorizontalKernel(*tex,blurCfg) :
      blurVerticalKernel(*tex,blurCfg));
  ConvolutionShader shader("blurHDR",args,kernel,*tex);

  shader.addUniform(GLSLUniform(tex->samplerType(), "hdrTexture"));

  ref_ptr<RenderPass> pass;
  if(isHorizontal) {
    pass = ref_ptr<RenderPass>::manage(
        new BlurHDRHorizontalPass(this, shader));
    blurHorizontalPass_ = pass;
  } else {
    pass = ref_ptr<RenderPass>::manage(
        new BlurHDRVerticalPass(this, shader));
    blurVerticalPass_ = pass;
  }

  scene_->addPostPass(pass);

  delete[] kernel.offsets_;
  delete[] kernel.weights_;
}

void HDRRenderer::makeTonemapPass()
{
  Texture *tex = downsampledTex4_.get();
  vector<string> args;
  args.push_back(FORMAT_STRING("gl_FragCoord.xy"));
  args.push_back("fragmentColor_");
  TonemapShader f(args,*tex);
  tonemapPass_ = ref_ptr<RenderPass>::manage(new TonemapHDRPass(this, f));
  scene_->addPostPass(tonemapPass_);
}

/////////////////

DownsampleHDRPass::DownsampleHDRPass(
    HDRRenderer *hdr,
    ShaderFunctions &f,
    ref_ptr<FrameBufferObject> downsampledBuffer)
: UnitOrthoRenderPass(hdr->scene_.get(), f),
  hdr_(hdr),
  downsampledBuffer_(downsampledBuffer)
{
}
void DownsampleHDRPass::render()
{
  hdr_->downsampledBuffer2_->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glViewport(0, 0,
      hdr_->downsampledBuffer2_->width(),
      hdr_->downsampledBuffer2_->height());
  UnitOrthoRenderPass::render(true);
}

BlurHDRPass::BlurHDRPass(HDRRenderer *hdr, ShaderFunctions &f)
: UnitOrthoRenderPass(hdr->scene_.get(), f),
  hdr_(hdr)
{
  hdrTextureUniform_ = glGetUniformLocation(shader_->id(), "hdrTexture");
}
BlurHDRHorizontalPass::BlurHDRHorizontalPass(HDRRenderer *hdr, ShaderFunctions &f)
: BlurHDRPass(hdr, f)
{
}
BlurHDRVerticalPass::BlurHDRVerticalPass(HDRRenderer *hdr, ShaderFunctions &f)
: BlurHDRPass(hdr, f)
{
}
void BlurHDRHorizontalPass::render()
{
  hdr_->downsampledBuffer4_->bind();
  glViewport(0, 0,
      hdr_->downsampledTex4_->width(),
      hdr_->downsampledTex4_->height());

  glDrawBuffer(GL_COLOR_ATTACHMENT0);

  shader_->enableShader();

  glActiveTexture(GL_TEXTURE5);
  hdr_->downsampledTex2_->bind();
  glUniform1i(hdrTextureUniform_, 5);

  UnitOrthoRenderPass::render(false);

  hdr_->downsampledTex4_->set_bufferIndex(0);
}
void BlurHDRVerticalPass::render()
{
  glDrawBuffer(GL_COLOR_ATTACHMENT1);

  shader_->enableShader();

  glActiveTexture(GL_TEXTURE5);
  hdr_->downsampledTex4_->bind();
  glUniform1i(hdrTextureUniform_, 5);

  UnitOrthoRenderPass::render(false);

  hdr_->downsampledTex4_->set_bufferIndex(1);
}

TonemapHDRPass::TonemapHDRPass(HDRRenderer *hdr, ShaderFunctions &f)
: UnitOrthoRenderPass(hdr->scene_.get(), f),
  hdr_(hdr)
{
  blurTextureUniform_ = glGetUniformLocation(shader_->id(), "blurTexture");
}
void TonemapHDRPass::render()
{
  shader_->enableShader();

  // FIXME: black downsampled texture on pc
  glActiveTexture(GL_TEXTURE5);
  hdr_->downsampledTex4_->bind();
  glUniform1i(blurTextureUniform_, 5);

  UnitOrthoRenderPass::render(false);
}

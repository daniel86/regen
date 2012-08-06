/*
 * blur.cpp
 *
 *  Created on: 31.07.2012
 *      Author: daniel
 */

#include <blur.h>

class ProjectionChangedBlur : public EventCallable {
public:
  ProjectionChangedBlur(BlurSeparablePass *g) : EventCallable(), pass_(g) {}
  virtual void call(EventObject*,void*) { pass_->resize(); }
  BlurSeparablePass *pass_;
};

BlurSeparablePass::BlurSeparablePass(
    Scene *scene,
    const BlurConfig &blurCfg,
    const float &sizeScale,
    GLenum textureFormat)
: UnitOrthoRenderPass(scene),
  sizeScale_(sizeScale)
{
  bufferSize_ = scene_->viewportUniform()->value() * sizeScale_;
  blurBuffer_ = ref_ptr<FrameBufferObject>::manage(new FrameBufferObject);
  blurBuffer_->bind();
  blurBuffer_->set_size(bufferSize_.x, bufferSize_.y);
  blurTex_ = ref_ptr<Texture2D>::manage(new TextureRectangle(2));
  blurTex_->set_size(bufferSize_.x, bufferSize_.y);
  blurTex_->set_internalFormat(textureFormat);
  for(int i=0; i<2; ++i) {
    blurTex_->bind();
    blurTex_->texImage();
    blurTex_->set_filter(GL_LINEAR,GL_LINEAR);
    blurBuffer_->addColorAttachment(*blurTex_.get());
    blurTex_->nextBuffer();
  }

  initDownsampleShader();
  initBlurShader(blurCfg);

  // resize glow buffer if projection changed
  projectionChangedCB_ = ref_ptr<EventCallable>::manage(
      new ProjectionChangedBlur(this));
  scene_->connect(Scene::PROJECTION_CHANGED, projectionChangedCB_);
}
BlurSeparablePass::~BlurSeparablePass()
{
  scene_->disconnect(projectionChangedCB_);
}

void BlurSeparablePass::resize()
{
  bufferSize_ = scene_->viewportUniform()->value() * sizeScale_;
  blurBuffer_->set_size(bufferSize_.x, bufferSize_.y);
  blurTex_->set_size(bufferSize_.x, bufferSize_.y);
  for(int j=0; j<2; ++j) {
    blurTex_->bind();
    blurTex_->texImage();
    blurTex_->nextBuffer();
  }
}

void BlurSeparablePass::initDownsampleShader()
{
  Texture *tex = blurTex_.get();
  vector<string> args;
  args.push_back("gl_FragCoord.xy");
  args.push_back("sceneTexture");
  args.push_back("fragmentColor_");
  Downsample f(args,*tex);
  downsampleShader_ = initShader(f);
}

void BlurSeparablePass::initBlurShader(
    const BlurConfig &blurCfg)
{
  Texture *tex = blurTex_.get();

  vector<string> args;
  args.push_back("gl_FragCoord.xy");
  args.push_back("inputTex");
  args.push_back("fragmentColor_");

  for(int i=0; i<2; ++i) {
    bool isHorizontal = (i==0);

    ConvolutionKernel kernel = (isHorizontal ?
        blurHorizontalKernel(*tex,blurCfg) :
        blurVerticalKernel(*tex,blurCfg));
    ConvolutionShader shader("blurSeparable",args,kernel,*tex);

    shader.addUniform(GLSLUniform(tex->samplerType(), "inputTex"));

    if(isHorizontal) {
      horizontalBlur_ = initShader(shader);
      texLocHorizontal_ = glGetUniformLocation(
          horizontalBlur_->id(), "inputTex");
    } else {
      verticalBlur_ = initShader(shader);
      texLocVertical_ = glGetUniformLocation(
          horizontalBlur_->id(), "inputTex");
    }

    delete[] kernel.offsets_;
    delete[] kernel.weights_;
  }
}

void BlurSeparablePass::render()
{
  // bind vertex data
  glBindBuffer(GL_ARRAY_BUFFER, unitOrthoQuad_->buffer());
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitOrthoQuad_->indexBuffer());
  // bind fbo
  blurBuffer_->bind();
  blurTex_->set_viewport();

  glActiveTexture(GL_TEXTURE5);
  { // down sample scene texture without blur
    // first blur pass can then be evaluated at lower resolution
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    downsampleShader_->enableShader();
    unitOrthoQuad_->drawMesh();
    // result is in GL_COLOR_ATTACHMENT0
    // next bind should bind this texture
    blurTex_->set_bufferIndex(0);
  }

  /*
  { // horizontal blur
    glDrawBuffer(GL_COLOR_ATTACHMENT1);

    horizontalBlur_->enableShader();

    blurTex_->bind();
    glUniform1i(texLocHorizontal_, 5);

    unitOrthoQuad_->drawMesh();
    // result is in GL_COLOR_ATTACHMENT1
    // next bind should bind this texture
    blurTex_->set_bufferIndex(1);
  }

  { // vertical blur
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    verticalBlur_->enableShader();

    blurTex_->bind();
    glUniform1i(texLocVertical_, 5);

    unitOrthoQuad_->drawMesh();
    // result is in GL_COLOR_ATTACHMENT0
    // next bind should bind this texture
    blurTex_->set_bufferIndex(0);
  }
  */
}

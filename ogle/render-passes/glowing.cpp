/*
 * glowing.cpp
 *
 *  Created on: 23.04.2012
 *      Author: daniel
 */

#include <glowing.h>

struct GlowSourceData {
  Mesh *mesh;
  ref_ptr<Texture> tex;
  GlowSourceData(Mesh *mesh_, ref_ptr<Texture> tex_)
  : mesh(mesh_), tex(tex_)
  {
  }
};
class GlowFragmentOutput : public ShaderFragmentOutput {
public:
  GlowFragmentOutput(GlowRenderer *glow);
  virtual string variableName() const {
    return "glowOutput";
  }
  virtual void addOutput(Mesh *mesh,
      ShaderFunctions &fragmentShader);
  GlowRenderer *glow_;
};
class BlurGlowPass : public UnitOrthoRenderPass {
public:
  BlurGlowPass(GlowRenderer *glow, ShaderFunctions &f);
  virtual bool rendersOnTop() { return false; }
  virtual bool usesDepthTest() { return false; }
  GLuint glowTextureUniform_;
  GlowRenderer *glow_;
};
class BlurGlowHorizontalPass : public BlurGlowPass {
public:
  BlurGlowHorizontalPass(GlowRenderer *glow, ShaderFunctions &f);
  virtual void render();
  virtual bool usesSceneBuffer() { return false; }
};
class BlurGlowVerticalPass : public BlurGlowPass {
public:
  BlurGlowVerticalPass(GlowRenderer *glow, ShaderFunctions &f);
  virtual void render();
  virtual bool rendersOnTop() { return true; }
  virtual bool usesDepthTest() { return false; }
  virtual bool usesSceneBuffer() { return true; }
};
class ProjectionChanged : public EventCallable {
public:
  ProjectionChanged(GlowRenderer *g) : EventCallable(), glow_(g) {}
  virtual void call(EventObject*,void*) { glow_->resize(); }
  GlowRenderer *glow_;
};

static Vec2ui glowSize(ref_ptr<Scene> scene, float glowToScreenScale_)
{
  return Vec2ui(
      scene->width()/glowToScreenScale_,
      scene->height()/glowToScreenScale_);
}

GlowRenderer::GlowRenderer(
    ref_ptr<Scene> scene,
    const GlowConfig &cfg)
: scene_(scene),
  cfg_(cfg),
  projectionChangedCB_(ref_ptr<EventCallable>::manage(new ProjectionChanged(this))),
  glowColorUniform_(ref_ptr<UniformVec4>::manage(new UniformVec4("glowColorUniform")))
{
  Vec2ui size = glowSize(scene_, cfg_.glowToScreenScale);

  glowColorUniform_->set_value(Vec4f(1.0f,0.0f,0.0f,1.0f));

  glowTex_ = ref_ptr<Texture2D>::manage(new TextureRectangle);
  glowTex_->set_size(scene_->width(), scene_->height());
  glowTex_->set_internalFormat(GL_RGBA);
  glowTex_->bind();
  glowTex_->texImage();

  { // setup the blur buffer
    blurGlowBuffer_ = ref_ptr<FrameBufferObject>::manage(new FrameBufferObject);
    blurGlowBuffer_->bind();
    blurGlowBuffer_->set_size(size.x, size.y);
    blurGlowTex_ = ref_ptr<Texture2D>::manage(new TextureRectangle);
    blurGlowTex_->set_size(size.x, size.y);
    blurGlowTex_->set_internalFormat(GL_RGBA);
    blurGlowTex_->bind();
    blurGlowTex_->texImage();
    blurGlowBuffer_->addColorAttachment(*blurGlowTex_.get());
  }

  // resize glow buffer if projection changed
  scene_->connect(Scene::PROJECTION_CHANGED, projectionChangedCB_);

  { // apply blur to glow buffer pre pass
    makeGlowBlurPass(true, cfg_.blurCfg);
    makeGlowBlurPass(false, cfg_.blurCfg);
  }

  { // render to glowTex_ in main pass
    glowFragmentOutput_ = ref_ptr<ShaderFragmentOutput>::manage(
        new GlowFragmentOutput(this));
    glowFragmentOutput_->set_clearColor(Vec4f(0.0f,0.0f,0.0f,0.0f));
    scene_->addFragmentOutput(glowFragmentOutput_, glowTex_);
  }

  handleGLError("after GlowRenderer::GlowRenderer");
}

GlowRenderer::~GlowRenderer()
{
  scene_->removePrePass(blurGlowHorizontalPass_);
  scene_->removePrePass(blurGlowVerticalPass_);
  scene_->removeFragmentOutput(glowFragmentOutput_);
  scene_->disconnect(projectionChangedCB_);
}

void GlowRenderer::resize()
{
  Vec2ui size = glowSize(scene_, cfg_.glowToScreenScale);

  // Note: glowTex_ resized automatically in Scene

  blurGlowBuffer_->set_size(size.x, size.y);
  blurGlowTex_->set_size(size.x, size.y);
  for(int j=0; j<2; ++j) {
    blurGlowTex_->bind();
    blurGlowTex_->texImage();
    blurGlowTex_->nextBuffer();
  }
}

void GlowRenderer::addGlowSource(Mesh *mesh, ref_ptr<Texture> tex)
{
  if(isGlowSource(mesh)) return;
  glowSources_.insert(pair< Mesh*, GlowSourceData >(
      mesh, GlowSourceData(mesh,tex)));
  // join the glow map so that we can use it in main pass shader
  if(tex.get()) mesh->joinStates(tex);
  mesh->joinStates(glowColorUniform_);
}
void GlowRenderer::removeGlowSource(Mesh *mesh)
{
  GlowSourceData *data = glowSourceData(mesh);
  if(!data) return;
  if(data->tex.get()) mesh->disjoinStates(data->tex);
  mesh->disjoinStates(glowColorUniform_);
  glowSources_.erase(mesh);
}
bool GlowRenderer::isGlowSource(Mesh *mesh) const
{
  return glowSources_.count(mesh)>0;
}
GlowSourceData* GlowRenderer::glowSourceData(Mesh *mesh)
{
  map<Mesh*,GlowSourceData>::iterator it = glowSources_.find(mesh);
  if(it == glowSources_.end()) {
    return NULL;
  } else {
    return &it->second;
  }
}

void GlowRenderer::makeGlowBlurPass(
    bool isHorizontal, const BlurConfig &blurCfg)
{
  Texture *tex = (isHorizontal ? glowTex_.get() : blurGlowTex_.get());
  vector<string> args;
  if(isHorizontal) {
    args.push_back(FORMAT_STRING("gl_FragCoord.xy*"<<cfg_.glowToScreenScale));
  } else {
    args.push_back(FORMAT_STRING("gl_FragCoord.xy/"<<cfg_.glowToScreenScale));
  }
  args.push_back("glowTexture");
  args.push_back("fragmentColor_");

  ConvolutionKernel kernel = (isHorizontal ?
      blurHorizontalKernel(*tex,blurCfg) :
      blurVerticalKernel(*tex,blurCfg));
  ConvolutionShader shader("blurGlow",args,kernel,*tex);

  shader.addUniform(GLSLUniform(tex->samplerType(), "glowTexture"));

  ref_ptr<RenderPass> pass;
  if(isHorizontal) {
    pass = ref_ptr<RenderPass>::manage(
        new BlurGlowHorizontalPass(this, shader));
    blurGlowHorizontalPass_ = pass;
  } else {
    pass = ref_ptr<RenderPass>::manage(
        new BlurGlowVerticalPass(this, shader));
    blurGlowVerticalPass_ = pass;
  }

  scene_->addPostPass(pass);

  delete[] kernel.offsets_;
  delete[] kernel.weights_;
}

/////////////////

GlowFragmentOutput::GlowFragmentOutput(GlowRenderer *glow)
: ShaderFragmentOutput(),
  glow_(glow)
{
}
static string getGlowColorTerm(const GlowConfig &cfg)
{
  string color = "";

  if(cfg.useConstantGlowColor) {
    const Vec4f &c = cfg.constantGlowColor;
    color = FORMAT_STRING("vec4("<<c.x<<","<<c.y<<","<<c.z<<","<<c.w<<")");
  }

  if(cfg.useUniformGlowColor) {
    if(color.size()==0) {
      color = "glowColorUniform";
    } else {
      color += "*glowColorUniform";
    }
  }

  return color;
}
void GlowFragmentOutput::addOutput(Mesh *mesh,
      ShaderFunctions &fragmentShader)
{
  fragmentShader.addFragmentOutput(
      GLSLFragmentOutput("vec4", variableName(), colorAttachment_ ));

  GlowSourceData *glowSource = glow_->glowSourceData(mesh);
  if(glowSource != NULL) {
    string glowColor = getGlowColorTerm(glow_->cfg_);

    if(glow_->cfg_.useUniformGlowColor) {
      fragmentShader.addUniform(GLSLUniform("vec4","glowColorUniform"));
    }

    if(glowSource->tex.get())
    { // use a glow map
      string mapColor = FORMAT_STRING("texture("<<
          glowSource->tex->name()<<","<<
          "f_uv"<<glowSource->tex->uvUnit()<<" )");

      fragmentShader.addMainVar(GLSLVariable("vec4", "_glowTexel", mapColor));
      mapColor = "_glowTexel*_glowTexel.a";

      //  change glow brightness by texture factor
      if(glowSource->tex->isFactorConstant()) {
        if(!isApprox( glowSource->tex->factor(), 1.0f )) {
          mapColor += FORMAT_STRING("*"<<glowSource->tex->factor());
        }
      } else {
        mapColor += FORMAT_STRING("*"<<glowSource->tex->name() << "factor");
      }
      if(glowColor.size()!=0) {
        mapColor = FORMAT_STRING("("<<mapColor<<") * "<<glowColor);
      }
      fragmentShader.addMainVar(GLSLVariable("vec4", "_glowColor", mapColor));
    }
    else
    {
      if(glowColor.size()==0) {
        glowColor = "vec4(1.0)";
      }
      fragmentShader.addMainVar( GLSLVariable("vec4", "_glowColor", glowColor));
    }
    if(glow_->cfg_.useMeshColor) {
      fragmentShader.addExport(
          GLSLExport(variableName(), "_color*_glowColor" ));
    } else {
      fragmentShader.addExport(
          GLSLExport(variableName(), "_glowColor" ));
    }
  } else {
    fragmentShader.addExport(
        GLSLExport(variableName(), "vec4(0.0)" ));
  }
}

/////////////////

BlurGlowPass::BlurGlowPass(GlowRenderer *glow, ShaderFunctions &f)
: UnitOrthoRenderPass(glow->scene_.get(), f),
  glow_(glow)
{
  glowTextureUniform_ = glGetUniformLocation(shader_->id(), "glowTexture");
}
BlurGlowHorizontalPass::BlurGlowHorizontalPass(GlowRenderer *glow, ShaderFunctions &f)
: BlurGlowPass(glow, f)
{
}
BlurGlowVerticalPass::BlurGlowVerticalPass(GlowRenderer *glow, ShaderFunctions &f)
: BlurGlowPass(glow, f)
{
}
void BlurGlowHorizontalPass::render()
{
  glow_->blurGlowBuffer_->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glViewport(0, 0,
      glow_->blurGlowTex_->width(),
      glow_->blurGlowTex_->height());

  glActiveTexture(GL_TEXTURE5);
  glow_->glowTex_->bind();

  shader_->enableShader();
  glUniform1i(glowTextureUniform_, 5);

  UnitOrthoRenderPass::render(false);
}
void BlurGlowVerticalPass::render()
{
  // use alpha blending
  glEnable(GL_BLEND);
  glBlendFunc(glow_->cfg_.blendFactorS, glow_->cfg_.blendFactorD);

  glActiveTexture(GL_TEXTURE5);
  glow_->blurGlowTex_->bind();

  shader_->enableShader();
  glUniform1i(glowTextureUniform_, 5);

  UnitOrthoRenderPass::render(false);

  glDisable (GL_BLEND);
}

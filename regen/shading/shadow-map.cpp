/*
 * shadow-map.cpp
 *
 *  Created on: 13.03.2013
 *      Author: daniel
 */

#include <cfloat>

#include <regen/states/state-configurer.h>

#include "shadow-map.h"
using namespace regen;

//////////////

static inline Vec2f findZRange(
    const Mat4f &mat, const Vec3f *frustumPoints)
{
  Vec2f range;
  // find the z-range of the current frustum as seen from the light
  // in order to increase precision
#define TRANSFORM_Z(vec) mat.x[2]*vec.x + mat.x[6]*vec.y + mat.x[10]*vec.z + mat.x[14]
  // note that only the z-component is needed and thus
  // the multiplication can be simplified from mat*vec4f(frustumPoints[0], 1.0f) to..
  GLfloat buf = TRANSFORM_Z(frustumPoints[0]);
  range.x = buf;
  range.y = buf;
  for(GLint i=1; i<8; ++i)
  {
    buf = TRANSFORM_Z(frustumPoints[i]);
    if(buf > range.y) { range.y = buf; }
    if(buf < range.x) { range.x = buf; }
  }
#undef TRANSFORM_Z
  return range;
}

/////////////

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

//////////////
ShadowMap::Config::Config()
{
  size = 512;
  numLayer = 1;
  depthFormat = GL_DEPTH_COMPONENT24;
  depthType = GL_FLOAT;
  textureTarget = GL_TEXTURE_2D;
  splitWeight = 0.8;
}
ShadowMap::Config::Config(const Config &other)
{
  size = other.size;
  numLayer = other.numLayer;
  depthFormat = other.depthFormat;
  depthType = other.depthType;
  textureTarget = other.textureTarget;
  splitWeight = other.splitWeight;
}

ShadowMap::ShadowMap(
    const ref_ptr<Light> &light,
    const ref_ptr<Camera> &sceneCamera,
    const Config &cfg)
: State(),
  Animation(GL_TRUE,GL_FALSE),
  HasInput(VertexBufferObject::USAGE_DYNAMIC),
  light_(light),
  sceneCamera_(sceneCamera),
  cfg_(cfg)
{
  RenderState *rs = RenderState::get();
  switch(light_->lightType()) {
  case Light::DIRECTIONAL:
    cfg_.textureTarget = GL_TEXTURE_2D_ARRAY;

    update_ = &ShadowMap::updateDirectional;
    computeDepth_ = &ShadowMap::computeDirectionalDepth;

    viewMatrix_ = new Mat4f[1];
    projectionMatrix_ = new Mat4f[cfg_.numLayer];
    viewProjectionMatrix_ = new Mat4f[cfg_.numLayer];
    break;

  case Light::POINT:
    cfg_.textureTarget = GL_TEXTURE_CUBE_MAP;
    cfg_.numLayer = 6;

    update_ = &ShadowMap::updatePoint;
    computeDepth_ = &ShadowMap::computePointDepth;

    viewMatrix_ = new Mat4f[cfg_.numLayer];
    projectionMatrix_ = new Mat4f[1];
    viewProjectionMatrix_ = new Mat4f[cfg_.numLayer];
    break;

  case Light::SPOT:
    cfg_.textureTarget = GL_TEXTURE_2D;
    cfg_.numLayer = 1;

    update_ = &ShadowMap::updateSpot;
    computeDepth_ = &ShadowMap::computeSpotDepth;

    viewMatrix_ = new Mat4f[1];
    projectionMatrix_ = new Mat4f[1];
    viewProjectionMatrix_ = new Mat4f[1];
    break;
  }

  depthFBO_ = ref_ptr<FrameBufferObject>::manage( new FrameBufferObject(
      cfg_.size,cfg_.size,cfg_.numLayer,
      cfg_.textureTarget,cfg_.depthFormat,cfg_.depthType));

  rs->drawFrameBuffer().push(depthFBO_->id());
  depthFBO_->drawBuffers().push(DrawBuffers::none());
  depthTexture_ = depthFBO_->depthTexture();
  depthTexture_->begin(rs);
  depthTexture_->set_wrapping(GL_REPEAT);
  depthTexture_->set_filter(GL_NEAREST,GL_NEAREST);
  depthTexture_->set_compare(GL_COMPARE_R_TO_TEXTURE, GL_LEQUAL);
  depthTexture_->end(rs);
  rs->drawFrameBuffer().pop();

  depthTextureState_ = ref_ptr<TextureState>::manage(
      new TextureState(depthTexture_, "inputTexture"));
  depthTextureState_->set_mapping(TextureState::MAPPING_CUSTOM);
  depthTextureState_->set_mapTo(TextureState::MAP_TO_CUSTOM);

  momentsQuad_ = Rectangle::getUnitQuad();

  shadowSize_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("shadowSize"));
  shadowSize_->setUniformData((GLfloat)cfg.size);
  setInput(shadowSize_);

  shadowInverseSize_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("shadowInverseSize"));
  shadowInverseSize_->setUniformData(1.0/(GLfloat)cfg.size);
  setInput(shadowInverseSize_);

  shadowFar_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("shadowFar"));
  shadowNear_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("shadowNear"));
  shadowMat_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("shadowMatrix"));
  switch(light_->lightType()) {
  case Light::DIRECTIONAL:
    shadowNear_->set_elementCount(cfg_.numLayer);
    shadowNear_->set_forceArray(GL_TRUE);
    shadowNear_->setUniformDataUntyped(NULL);

    shadowFar_->set_elementCount(cfg_.numLayer);
    shadowFar_->set_forceArray(GL_TRUE);
    shadowFar_->setUniformDataUntyped(NULL);

    shadowMat_->set_elementCount(cfg_.numLayer);
    shadowMat_->set_forceArray(GL_TRUE);
    shadowMat_->setUniformDataUntyped(NULL);
    break;
  case Light::POINT:
    shadowFar_->setUniformData(200.0f);
    shadowNear_->setUniformData(0.1f);

    shadowMat_->set_elementCount(cfg_.numLayer);
    shadowMat_->set_forceArray(GL_TRUE);
    shadowMat_->setUniformDataUntyped(NULL);
    shadowMat_->pushData((byte*)viewProjectionMatrix_[0].x);
    break;
  case Light::SPOT:
    shadowFar_->setUniformData(200.0f);
    shadowNear_->setUniformData(0.1f);
    shadowMat_->setUniformData(Mat4f::identity());
    break;
  }
  setInput(shadowFar_);
  setInput(shadowMat_);

  lightPosStamp_ = 0;
  lightDirStamp_ = 0;
  lightRadiusStamp_ = 0;
  projectionStamp_ = 0;

  if(light->lightType() == Light::POINT)
  { for(GLuint i=0; i<6; ++i) isCubeFaceVisible_[i] = GL_TRUE; }

  // avoid shadow acne
  setCullFrontFaces(GL_TRUE);

  // initially update shadow maps
  (this->*update_)();
}
ShadowMap::~ShadowMap()
{
  for(vector<Frustum*>::iterator
      it=shadowFrusta_.begin(); it!=shadowFrusta_.end(); ++it)
  { delete *it; }
  shadowFrusta_.clear();

  delete []viewMatrix_;
  delete []projectionMatrix_;
  delete []viewProjectionMatrix_;
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

void ShadowMap::set_depthFormat(GLenum f)
{
  cfg_.depthFormat = f;

  depthTexture_->begin(RenderState::get());
  depthTexture_->set_internalFormat(f);
  depthTexture_->texImage();
  depthTexture_->end(RenderState::get());
}
void ShadowMap::set_depthType(GLenum t)
{
  cfg_.depthType = t;

  depthTexture_->begin(RenderState::get());
  depthTexture_->set_pixelType(t);
  depthTexture_->texImage();
  depthTexture_->end(RenderState::get());
}
void ShadowMap::set_depthSize(GLuint shadowMapSize)
{
  cfg_.size = shadowMapSize;
  shadowSize_->setUniformData((GLfloat)shadowMapSize);
  shadowInverseSize_->setUniformData(1.0/(GLfloat)shadowMapSize);

  depthFBO_->resize(cfg_.size,cfg_.size,cfg_.numLayer);
  if(momentsTexture_.get()) {
    momentsFBO_->resize(cfg_.size,cfg_.size,cfg_.numLayer);
  }
  if(momentsFilter_.get()) {
    momentsFilter_->resize();
  }
}

void ShadowMap::set_shadowLayer(GLuint numLayer)
{
  if(light_->lightType()!=Light::DIRECTIONAL) { return; }

  if(cfg_.numLayer == numLayer) { return; }
  cfg_.numLayer = numLayer;

  ((DepthTexture3D*)depthTexture_.get())->set_depth(cfg_.numLayer);

  shadowMat_->set_elementCount(cfg_.numLayer);
  shadowMat_->setUniformDataUntyped(NULL);
  shadowFar_->set_elementCount(cfg_.numLayer);
  shadowFar_->setUniformDataUntyped(NULL);
  shadowNear_->set_elementCount(cfg_.numLayer);
  shadowNear_->setUniformDataUntyped(NULL);

  delete []projectionMatrix_;
  delete []viewProjectionMatrix_;
  projectionMatrix_ = new Mat4f[cfg_.numLayer];
  viewProjectionMatrix_ = new Mat4f[cfg_.numLayer];

  set_depthSize(cfg_.size);
}
void ShadowMap::set_splitWeight(GLdouble splitWeight)
{
  cfg_.splitWeight = splitWeight;
}

void ShadowMap::set_isCubeFaceVisible(GLenum face, GLboolean visible)
{
  isCubeFaceVisible_[face - GL_TEXTURE_CUBE_MAP_POSITIVE_X] = visible;
}

///////////
///////////

void ShadowMap::updateDirectional()
{
  Mat4f *shadowMatrices = (Mat4f*)shadowMat_->dataPtr();
  shadowMat_->nextStamp();

  // update near/far values when projection changed
  if(projectionStamp_ != sceneCamera_->projection()->stamp())
  {
    const Mat4f &proj = sceneCamera_->projection()->getVertex16f(0);
    // update frustum splits
    for(vector<Frustum*>::iterator
        it=shadowFrusta_.begin(); it!=shadowFrusta_.end(); ++it)
    { delete *it; }
    shadowFrusta_ = sceneCamera_->frustum()->split(cfg_.numLayer, cfg_.splitWeight);
    // update near/far values
    GLfloat *farValues = (GLfloat*)shadowFar_->dataPtr();
    GLfloat *nearValues = (GLfloat*)shadowNear_->dataPtr();
    shadowFar_->nextStamp();
    shadowNear_->nextStamp();
    for(GLuint i=0; i<cfg_.numLayer; ++i)
    {
      Frustum *frustum = shadowFrusta_[i];
      const GLfloat &n = frustum->near()->getVertex1f(0);
      const GLfloat &f = frustum->far()->getVertex1f(0);
      // frustum_->far() is originally in eye space - tell's us how far we can see.
      // Here we compute it in camera homogeneous coordinates. Basically, we calculate
      // proj * (0, 0, far, 1)^t and then normalize to [0; 1]
      farValues[i]  = 0.5*(-f  * proj(2,2) + proj(3,2)) / f + 0.5;
      nearValues[i] = 0.5*(-n * proj(2,2) + proj(3,2)) / n + 0.5;
    }
    projectionStamp_ = sceneCamera_->projection()->stamp();
  }
  // update view matrix when light direction changed
  if(lightDirStamp_ != light_->direction()->stamp())
  {
    const Vec3f &dir = light_->direction()->getVertex3f(0);
    Vec3f f(-dir.x, -dir.y, -dir.z);
    f.normalize();
    Vec3f s( 0.0f, -f.z, f.y );
    s.normalize();
    // Equivalent to getLookAtMatrix(pos=(0,0,0), dir=f, up=(-1,0,0))
    viewMatrix_[0] = Mat4f(
        0.0f, s.y*f.z - s.z*f.y, -f.x, 0.0f,
         s.y,           s.z*f.x, -f.y, 0.0f,
         s.z,          -s.y*f.x, -f.z, 0.0f,
        0.0f,              0.0f, 0.0f, 1.0f
    );
    lightDirStamp_ = light_->direction()->stamp();
  }

  // update view and view-projection matrices
  for(register GLuint i=0; i<cfg_.numLayer; ++i)
  {
    Frustum *frustum = shadowFrusta_[i];
    // update frustum points in world space
    frustum->computePoints(
        sceneCamera_->position()->getVertex3f(0),
        sceneCamera_->direction()->getVertex3f(0));
    const Vec3f *frustumPoints = frustum->points();

    // get the projection matrix with the new z-bounds
    // note the inversion because the light looks at the neg. z axis
    Vec2f zRange = findZRange(viewMatrix_[0], frustumPoints);
    projectionMatrix_[i] = Mat4f::orthogonalMatrix(
        -1.0, 1.0, -1.0, 1.0, -zRange.y, -zRange.x);

    // find the extends of the frustum slice as projected in light's homogeneous coordinates
    Vec2f xRange(FLT_MAX,FLT_MIN);
    Vec2f yRange(FLT_MAX,FLT_MIN);
    Mat4f mvpMatrix = (viewMatrix_[0] * projectionMatrix_[i]).transpose();
    for(register GLuint j=0; j<8; ++j)
    {
        Vec4f transf = mvpMatrix * frustumPoints[j];
        transf.x /= transf.w;
        transf.y /= transf.w;
        if (transf.x > xRange.y) { xRange.y = transf.x; }
        if (transf.x < xRange.x) { xRange.x = transf.x; }
        if (transf.y > yRange.y) { yRange.y = transf.y; }
        if (transf.y < yRange.x) { yRange.x = transf.y; }
    }
    projectionMatrix_[i] *= Mat4f::cropMatrix(xRange.x, xRange.y, yRange.x, yRange.y);

    viewProjectionMatrix_[i] = viewMatrix_[0] * projectionMatrix_[i];
    // transforms world space coordinates to homogenous light space
    shadowMatrices[i] = viewProjectionMatrix_[i] * Mat4f::bias();
  }
}

void ShadowMap::updatePoint()
{
  if(lightPosStamp_ == light_->position()->stamp() &&
      lightRadiusStamp_ == light_->radius()->stamp())
  { return; }
  shadowMat_->nextStamp();

  const Vec3f &pos = light_->position()->getVertex3f(0);
  GLfloat far = light_->radius()->getVertex2f(0).y;

  shadowFar_->setVertex1f(0, far);
  projectionMatrix_[0] = Mat4f::projectionMatrix(
      90.0, 1.0f, shadowNear_->getVertex1f(0), far);
  Mat4f::cubeLookAtMatrices(pos, viewMatrix_);

  for(register GLuint i=0; i<6; ++i) {
    if(!isCubeFaceVisible_[i]) { continue; }
    viewProjectionMatrix_[i] = viewMatrix_[i] * projectionMatrix_[0];
  }

  lightPosStamp_ = light_->position()->stamp();
  lightRadiusStamp_ = light_->radius()->stamp();
}

void ShadowMap::updateSpot()
{
  if(lightPosStamp_ == light_->position()->stamp() &&
      lightDirStamp_ == light_->direction()->stamp() &&
      lightRadiusStamp_ == light_->radius()->stamp())
  { return; }

  const Vec3f &pos = light_->position()->getVertex3f(0);
  const Vec3f &dir = light_->direction()->getVertex3f(0);
  const Vec2f &a = light_->radius()->getVertex2f(0);
  shadowFar_->setVertex1f(0, a.y);

  viewMatrix_[0] = Mat4f::lookAtMatrix(pos, dir, Vec3f::up());

  const Vec2f &coneAngle = light_->coneAngle()->getVertex2f(0);
  projectionMatrix_[0] = Mat4f::projectionMatrix(
      2.0*acos(coneAngle.y)*RAD_TO_DEGREE,
      1.0f,
      shadowNear_->getVertex1f(0),
      shadowFar_->getVertex1f(0));
  viewProjectionMatrix_[0] = viewMatrix_[0] * projectionMatrix_[0];
  // transforms world space coordinates to homogenous light space
  shadowMat_->setVertex16f(0, viewProjectionMatrix_[0] * Mat4f::bias());

  lightPosStamp_ = light_->position()->stamp();
  lightDirStamp_ = light_->direction()->stamp();
  lightRadiusStamp_ = light_->radius()->stamp();
}

////////////
////////////

void ShadowMap::computeDirectionalDepth(RenderState *rs)
{
  sceneCamera_->position()->pushData((byte*)&Vec3f::zero().x);
  sceneCamera_->view()->pushData((byte*)viewMatrix_[0].x);
  for(register GLuint i=0; i<cfg_.numLayer; ++i)
  {
    glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT, depthTexture_->id(), 0, i);
    glClear(GL_DEPTH_BUFFER_BIT);

    sceneCamera_->projection()->pushData((byte*)projectionMatrix_[i].x);
    sceneCamera_->viewProjection()->pushData((byte*)viewProjectionMatrix_[i].x);

    traverse(rs);

    sceneCamera_->viewProjection()->popData();
    sceneCamera_->projection()->popData();
  }
  sceneCamera_->view()->popData();
  sceneCamera_->position()->popData();
}

void ShadowMap::computePointDepth(RenderState *rs)
{
  sceneCamera_->position()->pushData(light_->position()->dataPtr());
  sceneCamera_->projection()->pushData((byte*)projectionMatrix_[0].x);
  for(register GLuint i=0; i<6; ++i)
  {
    if(!isCubeFaceVisible_[i]) { continue; }
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,
        depthTexture_->id(), 0);
    glClear(GL_DEPTH_BUFFER_BIT);

    sceneCamera_->view()->pushData((byte*)viewMatrix_[i].x);
    sceneCamera_->viewProjection()->pushData((byte*)viewProjectionMatrix_[i].x);

    traverse(rs);

    sceneCamera_->viewProjection()->popData();
    sceneCamera_->view()->popData();
  }
  sceneCamera_->projection()->popData();
  sceneCamera_->position()->popData();
}

void ShadowMap::computeSpotDepth(RenderState *rs)
{
  sceneCamera_->position()->pushData(light_->position()->dataPtr());
  sceneCamera_->view()->pushData((byte*) viewMatrix_[0].x);
  sceneCamera_->projection()->pushData((byte*) projectionMatrix_[0].x);
  sceneCamera_->viewProjection()->pushData((byte*) viewProjectionMatrix_[0].x);

  glClear(GL_DEPTH_BUFFER_BIT);
  traverse(rs);

  sceneCamera_->view()->popData();
  sceneCamera_->projection()->popData();
  sceneCamera_->viewProjection()->popData();
  sceneCamera_->position()->popData();
}

///////////
///////////

void ShadowMap::setComputeMoments()
{
  if(momentsCompute_.get()) { return; }
  RenderState *rs = RenderState::get();

  momentsFBO_ = ref_ptr<FrameBufferObject>::manage(new FrameBufferObject(
      cfg_.size,cfg_.size,cfg_.numLayer,
      GL_NONE,GL_NONE,GL_NONE));

  rs->drawFrameBuffer().push(momentsFBO_->id());
  momentsFBO_->drawBuffers().push(DrawBuffers::attachment0());
  momentsTexture_ = momentsFBO_->addTexture(1,
      depthTexture_->targetType(),
      GL_RGBA, GL_RGBA, GL_BYTE);
      //GL_RGBA, GL_RGBA32F, GL_FLOAT);
  momentsTexture_->begin(rs);
  momentsTexture_->set_wrapping(GL_CLAMP_TO_EDGE);
  momentsTexture_->set_filter(GL_LINEAR,GL_LINEAR);
  momentsTexture_->end(rs);
  rs->drawFrameBuffer().pop();

  momentsCompute_ = ref_ptr<ShaderState>::manage(new ShaderState);
  StateConfigurer cfg;
  cfg.addState(depthTextureState_.get());
  cfg.addState(momentsQuad_.get());
  switch(cfg_.textureTarget) {
  case GL_TEXTURE_CUBE_MAP:
    cfg.define("IS_CUBE_SHADOW", "TRUE");
    break;
  case GL_TEXTURE_2D_ARRAY: {
    Texture3D *depth = (Texture3D*) depthTexture_.get();
    cfg.define("IS_ARRAY_SHADOW", "TRUE");
    cfg.define("NUM_SHADOW_LAYER", REGEN_STRING(depth->depth()));
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
  momentsQuad_->initializeResources(rs, cfg.cfg(), momentsCompute_->shader());

  momentsFilter_ = ref_ptr<FilterSequence>::manage(new FilterSequence(momentsTexture_));

  momentsBlurSize_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("numBlurPixels"));
  momentsBlurSize_->setUniformData(4);
  momentsFilter_->joinShaderInput(momentsBlurSize_);

  momentsBlurSigma_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurSigma"));
  momentsBlurSigma_->setUniformData(2.0);
  momentsFilter_->joinShaderInput(momentsBlurSigma_);

}
void ShadowMap::createBlurFilter(
    GLuint size, GLfloat sigma,
    GLboolean downsampleTwice)
{
  // first downsample the moments texture
  momentsFilter_->addFilter(ref_ptr<Filter>::manage(new Filter("sampling.downsample", 0.5)));
  if(downsampleTwice) {
    momentsFilter_->addFilter(ref_ptr<Filter>::manage(new Filter("sampling.downsample", 0.5)));
  }
  momentsFilter_->addFilter(ref_ptr<Filter>::manage(new Filter("blur.vertical")));
  momentsFilter_->addFilter(ref_ptr<Filter>::manage(new Filter("blur.horizontal")));
  momentsBlurSize_->setVertex1i(0, size);
  momentsBlurSigma_->setVertex1f(0, sigma);
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

void ShadowMap::glAnimate(RenderState *rs, GLdouble dt)
{
  GL_ERROR_LOG();
  (this->*update_)();

  {
    rs->drawFrameBuffer().push(depthFBO_->id());
    rs->viewport().push(depthFBO_->glViewport());

    enable(rs);
    {
      // we use unmodified tree passed in. but it is not
      // allowed to push some states during shadow traversal.
      // we lock them here so pushes will not change server
      // side state.
      rs->drawFrameBuffer().lock();
      rs->viewport().lock();
      rs->cullFace().lock();
      rs->frontFace().lock();
      rs->colorMask().lock();
      rs->depthFunc().lock();
      rs->depthMask().lock();
      rs->depthRange().lock();
    }
    (this->*computeDepth_)(rs);
    {
      rs->drawFrameBuffer().unlock();
      rs->viewport().unlock();
      rs->cullFace().unlock();
      rs->frontFace().unlock();
      rs->colorMask().unlock();
      rs->depthFunc().unlock();
      rs->depthMask().unlock();
      rs->depthRange().unlock();
    }
    disable(rs);

    rs->drawFrameBuffer().pop();
    rs->viewport().pop();
    GL_ERROR_LOG();
  }

  // compute depth moments
  if(momentsTexture_.get()) {
    GLint channel = rs->reserveTextureChannel();

    rs->drawFrameBuffer().push(momentsFBO_->id());
    rs->viewport().push(momentsFBO_->glViewport());
    rs->toggles().push(RenderState::DEPTH_TEST, GL_FALSE);
    rs->depthMask().push(GL_FALSE);

    depthTexture_->begin(rs,channel);
    depthTexture_->set_compare(GL_NONE, GL_LEQUAL);

    // update moments texture
    momentsCompute_->enable(rs);
    shadowNear_->enableUniform(momentsNear_);
    shadowFar_->enableUniform(momentsFar_);
    momentsQuad_->enable(rs);
    momentsQuad_->disable(rs);
    momentsCompute_->disable(rs);
    // and filter the result
    momentsFilter_->enable(rs);
    momentsFilter_->disable(rs);

    // reset to old state
    depthTexture_->set_compare(GL_COMPARE_R_TO_TEXTURE, GL_LEQUAL);
    depthTexture_->end(rs,channel);

    rs->depthMask().pop();
    rs->toggles().pop(RenderState::DEPTH_TEST);
    rs->viewport().pop();
    rs->drawFrameBuffer().pop();

    rs->releaseTextureChannel();
    GL_ERROR_LOG();
  }
}

///////////
///////////

const ref_ptr<Texture>& ShadowMap::shadowDepth() const
{ return depthTexture_; }

const ref_ptr<Texture>& ShadowMap::shadowMomentsUnfiltered() const
{ return momentsTexture_; }
const ref_ptr<Texture>& ShadowMap::shadowMoments() const
{ return momentsFilter_->output(); }
const ref_ptr<FilterSequence>& ShadowMap::momentsFilter() const
{ return momentsFilter_; }
const ref_ptr<ShaderInput1i>& ShadowMap::momentsBlurSize() const
{ return momentsBlurSize_; }
const ref_ptr<ShaderInput1f>& ShadowMap::momentsBlurSigma() const
{ return momentsBlurSigma_; }

const ref_ptr<ShaderInputMat4>& ShadowMap::shadowMat() const
{ return shadowMat_; }
const ref_ptr<ShaderInput1f>& ShadowMap::shadowFar() const
{ return shadowFar_; }
const ref_ptr<ShaderInput1f>& ShadowMap::shadowNear() const
{ return shadowNear_; }
const ref_ptr<ShaderInput1f>& ShadowMap::shadowSize() const
{ return shadowSize_; }
const ref_ptr<ShaderInput1f>& ShadowMap::shadowInverseSize() const
{ return shadowInverseSize_; }

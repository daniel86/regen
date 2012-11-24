/*
 * directional-shadow-map.cpp
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#include <ogle/utility/string-util.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>

#include "directional-shadow-map.h"

// #define DEBUG_SHADOW_MAPS

// TODO SM: layered shaders avoiding multiple draw calls.
//      * http://www.opengl.org/wiki/Geometry_Shader#Layered_rendering

static Vec2f findZRange(
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

class ShadowRenderState : public RenderState
{
public:
  virtual GLboolean isNodeHidden(StateNode *node) {
    return RenderState::isNodeHidden(node);
  }
  virtual GLboolean isStateHidden(State *state) {
    return RenderState::isStateHidden(state);
  }

  virtual void pushFBO(FrameBufferObject *tex) {}
  virtual void popFBO() {}
};

GLuint DirectionalShadowMap::numSplits_ = 3;

DirectionalShadowMap::DirectionalShadowMap(
    ref_ptr<DirectionalLight> &light,
    ref_ptr<Frustum> &sceneFrustum,
    ref_ptr<PerspectiveCamera> &sceneCamera,
    GLuint shadowMapSize)
: ShadowMap(),
  light_(light),
  sceneCamera_(sceneCamera),
  sceneFrustum_(sceneFrustum)
{
  // create a 3d depth texture - each frustum slice gets one layer
  texture_ = ref_ptr<DepthTexture3D>::manage(new DepthTexture3D(
      1, GL_DEPTH_COMPONENT24, GL_FLOAT, GL_TEXTURE_2D_ARRAY));
  texture_->set_numTextures(numSplits_);
  texture_->bind();
  texture_->set_size(shadowMapSize, shadowMapSize);
  texture_->set_filter(GL_LINEAR,GL_LINEAR);
  texture_->set_wrapping(GL_CLAMP_TO_EDGE);
  texture_->set_compare(GL_COMPARE_R_TO_TEXTURE, GL_LEQUAL);
  texture_->texImage();
  // create depth only render target for updating the shadow maps
  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glDrawBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  projectionMatrices_ = new Mat4f[numSplits_];
  // uniforms for shadow sampling
  shadowMatUniform_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4(
      FORMAT_STRING("shadowMatrices"<<light->id()), numSplits_));
  shadowMatUniform_->set_forceArray(GL_TRUE);
  shadowMatUniform_->setInstanceData(1, 1, NULL);
  shadowFarUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f(
      FORMAT_STRING("shadowFar"<<light->id()), numSplits_));
  shadowFarUniform_->set_forceArray(GL_TRUE);
  shadowFarUniform_->setInstanceData(1, 1, NULL);

  shadowMap_ = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(texture_)));
  shadowMap_->set_name(FORMAT_STRING("shadowMap"<<light->id()));
  shadowMap_->set_mapping(MAPPING_CUSTOM);
  shadowMap_->setMapTo(MAP_TO_CUSTOM);

  updateLightDirection();
  updateProjection();

  light_->joinShaderInput(
      ref_ptr<ShaderInput>::cast(shadowMatUniform()));
  light_->joinShaderInput(
      ref_ptr<ShaderInput>::cast(shadowFarUniform()));
  light_->joinStates(
      ref_ptr<State>::cast(shadowMap()));
  light_->shaderDefine(
      FORMAT_STRING("LIGHT"<<light_->id()<<"_HAS_SM"), "TRUE");
  light_->shaderDefine(
      FORMAT_STRING("NUM_SHADOW_MAP_SLICES"), FORMAT_STRING(numSplits_));
}
DirectionalShadowMap::~DirectionalShadowMap()
{
  for(vector<Frustum*>::iterator
      it=shadowFrusta_.begin(); it!=shadowFrusta_.end(); ++it) { delete *it; }
  shadowFrusta_.clear();
  delete projectionMatrices_;
}

void DirectionalShadowMap::set_numSplits(GLuint numSplits)
{
  numSplits_ = numSplits;
}
GLuint DirectionalShadowMap::numSplits()
{
  return numSplits_;
}

ref_ptr<ShaderInputMat4>& DirectionalShadowMap::shadowMatUniform()
{
  return shadowMatUniform_;
}
ref_ptr<ShaderInput1f>& DirectionalShadowMap::shadowFarUniform()
{
  return shadowFarUniform_;
}
ref_ptr<TextureState>& DirectionalShadowMap::shadowMap()
{
  return shadowMap_;
}

void DirectionalShadowMap::updateLightDirection()
{
  const Vec3f &dir = light_->direction()->getVertex3f(0);
  Vec3f f(-dir.x, -dir.y, -dir.z);
  normalize(f);
  Vec3f s( 0.0f, -f.z, f.y );
  normalize(s);
  // Equivalent to getLookAtMatrix(pos=(0,0,0), dir=-dir, up=(-1,0,0))
  viewMatrix_ = Mat4f(
      0.0f, s.y*f.z - s.z*f.y, -f.x, 0.0f,
       s.y,           s.z*f.x, -f.y, 0.0f,
       s.z,          -s.y*f.x, -f.z, 0.0f,
      0.0f,              0.0f, 0.0f, 1.0f
  );
}

void DirectionalShadowMap::updateProjection()
{
  for(vector<Frustum*>::iterator
      it=shadowFrusta_.begin(); it!=shadowFrusta_.end(); ++it) { delete *it; }
  shadowFrusta_ = sceneFrustum_->split(numSplits_, 0.75);

  Mat4f &proj = sceneCamera_->projectionUniform()->getVertex16f(0);
  GLfloat *farValues = (GLfloat*)shadowFarUniform_->dataPtr();

  for(GLuint i=0; i<numSplits_; ++i)
  {
    Frustum *frustum = shadowFrusta_[i];
    // frustum_->far() is originally in eye space - tell's us how far we can see.
    // Here we compute it in camera homogeneous coordinates. Basically, we calculate
    // proj * (0, 0, far, 1)^t and then normalize to [0; 1]
    farValues[i] = 0.5*(-frustum->far() * proj(2,2) + proj(3,2)) / frustum->far() + 0.5;
  }
}

void DirectionalShadowMap::updateCamera()
{
  static Mat4f staticBiasMatrix = Mat4f(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0 );

  Mat4f *shadowMatrices = (Mat4f*)shadowMatUniform_->dataPtr();

  for(register GLuint i=0; i<numSplits_; ++i)
  {
    Frustum *frustum = shadowFrusta_[i];
    // update frustum points in world space
    frustum->calculatePoints(
        sceneCamera_->position(),
        sceneCamera_->direction(),
        UP_VECTOR);
    const Vec3f *frustumPoints = frustum->points();

    // find ranges to optimize precision
    Vec2f xRange(1000.0,-1000.0);
    Vec2f yRange(1000.0,-1000.0);
    Vec2f zRange = findZRange(viewMatrix_, frustumPoints);

    // get the projection matrix with the new z-bounds
    // note the inversion because the light looks at the neg. z axis
    projectionMatrices_[i] = getOrthogonalProjectionMatrix(
        -1.0, 1.0, -1.0, 1.0, -zRange.y, -zRange.x);

    // find the extends of the frustum slice as projected in light's homogeneous coordinates
    Mat4f mvpMatrix = transpose(viewMatrix_ * projectionMatrices_[i]);
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

    projectionMatrices_[i] = projectionMatrices_[i] *
        getCropMatrix(xRange.x, xRange.y, yRange.x, yRange.y);
    // transforms world space coordinates to homogenous light space
    shadowMatrices[i] = viewMatrix_ * projectionMatrices_[i] * staticBiasMatrix;
  }
}

void DirectionalShadowMap::updateGraphics(GLdouble dt)
{
  updateCamera();

  // offset the geometry slightly to prevent z-fighting
  // note that this introduces some light-leakage artifacts
#ifdef OFFSET_GEOMETRY
  glEnable( GL_POLYGON_OFFSET_FILL );
  glPolygonOffset( 1.1, 4096.0 );
#endif
  // moves acne to back faces
  glCullFace(GL_FRONT);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glDrawBuffer(GL_NONE);
  glViewport(0, 0, texture_->width(), texture_->height());

  // remember scene view and projection
  Mat4f sceneView = sceneCamera_->viewUniform()->getVertex16f(0);
  Mat4f sceneProjection = sceneCamera_->projectionUniform()->getVertex16f(0);
  sceneCamera_->viewUniform()->setVertex16f(0, viewMatrix_);

  for(register GLuint i=0; i<numSplits_; ++i) {
    // make the current depth map a rendering target
    glFramebufferTextureLayer(GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT, texture_->id(), 0, i);
    // clear the depth texture from last time
    glClear(GL_DEPTH_BUFFER_BIT);
    sceneCamera_->projectionUniform()->setVertex16f(0, projectionMatrices_[i]);

    // tree traverse
    for(list< ref_ptr<StateNode> >::iterator
        it=caster_.begin(); it!=caster_.end(); ++it)
    {
      ShadowRenderState rs;
      traverseTree(&rs, it->get());
    }
  }

  sceneCamera_->viewUniform()->setVertex16f(0, sceneView);
  sceneCamera_->projectionUniform()->setVertex16f(0, sceneProjection);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glCullFace(GL_BACK);
#ifdef OFFSET_GEOMETRY
  glDisable(GL_POLYGON_OFFSET_FILL);
#endif

#ifdef DEBUG_SHADOW_MAPS
  drawDebugHUD();
#endif
}

void DirectionalShadowMap::drawDebugHUD()
{
    static GLint layerLoc;
    static GLint textureLoc;
    static ref_ptr<ShaderState> debugShader;
    if(debugShader.get() == NULL) {
      debugShader = ref_ptr<ShaderState>::manage(new ShaderState);
      map<string, string> shaderConfig;
      map<GLenum, string> shaderNames;
      shaderNames[GL_FRAGMENT_SHADER] = "shadow-mapping.debugDirectional.fs";
      shaderNames[GL_VERTEX_SHADER] = "shadow-mapping.debug.vs";
      debugShader->createSimple(shaderConfig,shaderNames);
      debugShader->shader()->compile();
      debugShader->shader()->link();

      layerLoc = glGetUniformLocation(debugShader->shader()->id(), "in_shadowLayer");
      textureLoc = glGetUniformLocation(debugShader->shader()->id(), "in_shadowMap");
    }

    glDisable(GL_DEPTH_TEST);

    glUseProgram(debugShader->shader()->id());
    glUniform1i(textureLoc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture( GL_TEXTURE_2D_ARRAY, texture_->id() );
    glTexParameteri(GL_TEXTURE_2D_ARRAY,
        GL_TEXTURE_COMPARE_MODE, GL_NONE);

    for(GLuint i=0; i<numSplits_; ++i) {
      glViewport(130*i, 0, 128, 128);
      glUniform1f(layerLoc, float(i));

      glBegin(GL_QUADS);
      glVertex3f(-1.0, -1.0, 0.0);
      glVertex3f( 1.0, -1.0, 0.0);
      glVertex3f( 1.0,  1.0, 0.0);
      glVertex3f(-1.0,  1.0, 0.0);
      glEnd();
    }

    // reset states
    glTexParameteri(GL_TEXTURE_2D_ARRAY,
        GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    glEnable(GL_DEPTH_TEST);
}

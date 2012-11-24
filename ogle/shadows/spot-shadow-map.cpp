/*
 * spot-shadow-map.cpp
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#include <ogle/utility/string-util.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>

#include "spot-shadow-map.h"

//#define DEBUG_SHADOW_MAPS

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

SpotShadowMap::SpotShadowMap(
    ref_ptr<SpotLight> &light,
    ref_ptr<PerspectiveCamera> &sceneCamera,
    GLuint shadowMapSize)
: ShadowMap(),
  light_(light),
  sceneCamera_(sceneCamera),
  compareMode_(GL_COMPARE_R_TO_TEXTURE)
  //compareMode_(GL_NONE)
{
  // create a 3d depth texture - each frustum slice gets one layer
  texture_ = ref_ptr<DepthTexture2D>::manage(new DepthTexture2D);
  texture_->set_internalFormat(GL_DEPTH_COMPONENT24);
  texture_->set_pixelType(GL_FLOAT);
  //texture_->set_targetType(GL_TEXTURE_2D);
  texture_->bind();
  texture_->set_size(shadowMapSize, shadowMapSize);
  //texture_->set_filter(GL_LINEAR,GL_LINEAR);
  texture_->set_filter(GL_NEAREST,GL_NEAREST);
  texture_->set_wrapping(GL_CLAMP_TO_EDGE);
  texture_->set_compare(compareMode_, GL_LEQUAL);
  texture_->texImage();
  // create depth only render target for updating the shadow maps
  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glDrawBuffer(GL_NONE);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_->id(), 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // uniforms for shadow sampling
  shadowMatUniform_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4(
      FORMAT_STRING("shadowMatrix"<<light->id())));
  shadowMatUniform_->setInstanceData(1, 1, NULL);

  shadowMap_ = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(texture_)));
  shadowMap_->set_name(FORMAT_STRING("shadowMap"<<light->id()));
  shadowMap_->set_mapping(MAPPING_CUSTOM);
  shadowMap_->setMapTo(MAP_TO_CUSTOM);

  updateLight();

  light_->joinShaderInput(
      ref_ptr<ShaderInput>::cast(shadowMatUniform()));
  light_->joinStates(
      ref_ptr<State>::cast(shadowMap()));
  light_->shaderDefine(
      FORMAT_STRING("LIGHT"<<light_->id()<<"_HAS_SM"), "TRUE");
}

ref_ptr<ShaderInputMat4>& SpotShadowMap::shadowMatUniform()
{
  return shadowMatUniform_;
}
ref_ptr<TextureState>& SpotShadowMap::shadowMap()
{
  return shadowMap_;
}

void SpotShadowMap::updateLight()
{
  static Mat4f staticBiasMatrix = Mat4f(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0 );
  // everything below this attenuation does not appear in the shadow map
  static const GLfloat farAttenuation = 0.01f;

  const Vec3f &pos = light_->position()->getVertex3f(0);
  const Vec3f &dir = light_->spotDirection()->getVertex3f(0);
  const Vec2f &coneAngle = light_->coneAngle()->getVertex2f(0);
  const Vec3f &a = light_->attenuation()->getVertex3f(0);
  viewMatrix_ = getLookAtMatrix(pos, dir, UP_VECTOR);

  // adjust far value for better precision
  // TODO SM: allow to specify bounds to increase precision
  // TODO SM: use scene frustum to increase precision
  GLfloat far;
  // find far value where light attenuation reaches threshold,
  // insert farAttenuation in the attenuation equation and solve
  // equation for distance
  GLdouble p2 = a.y/(2.0*a.z);
  far = -p2 + sqrt(p2*p2 - (a.x/farAttenuation - 1.0/(farAttenuation*a.z)));
  // hard limit to 200.0 z range
  if(far>200.0) far=200.0;

  projectionMatrix_ = projectionMatrix(
      // spot fov
      2.0*360.0*acos(coneAngle.x)/(2.0*M_PI),
      // shadow map aspect
      1.0f,
      // near
      2.0f,
      far);

  // transforms world space coordinates to homogenous light space
  shadowMatUniform_->getVertex16f(0) =
      viewMatrix_ * projectionMatrix_ * staticBiasMatrix;
}

void SpotShadowMap::updateGraphics(GLdouble dt)
{
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
  glClear(GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, texture_->width(), texture_->height());

  // remember scene view and projection
  Mat4f sceneView = sceneCamera_->viewUniform()->getVertex16f(0);
  Mat4f sceneProjection = sceneCamera_->projectionUniform()->getVertex16f(0);
  sceneCamera_->viewUniform()->setVertex16f(0, viewMatrix_);
  sceneCamera_->projectionUniform()->setVertex16f(0, projectionMatrix_);

  // tree traverse
  for(list< ref_ptr<StateNode> >::iterator
      it=caster_.begin(); it!=caster_.end(); ++it)
  {
    ShadowRenderState rs;
    traverseTree(&rs, it->get());
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

void SpotShadowMap::drawDebugHUD()
{
    static GLint layerLoc;
    static GLint textureLoc;
    static ref_ptr<ShaderState> debugShader;
    if(debugShader.get() == NULL) {
      debugShader = ref_ptr<ShaderState>::manage(new ShaderState);
      map<string, string> shaderConfig;
      map<GLenum, string> shaderNames;
      shaderNames[GL_FRAGMENT_SHADER] = "shadow-mapping.debugSpot.fs";
      shaderNames[GL_VERTEX_SHADER] = "shadow-mapping.debug.vs";
      debugShader->createSimple(shaderConfig,shaderNames);
      debugShader->shader()->compile();
      debugShader->shader()->link();

      textureLoc = glGetUniformLocation(debugShader->shader()->id(), "in_shadowMap");
    }

    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, 128, 128);

    glUseProgram(debugShader->shader()->id());
    glUniform1i(textureLoc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_->id());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    glBegin(GL_QUADS);
    glVertex3f(-1.0, -1.0, 0.0);
    glVertex3f( 1.0, -1.0, 0.0);
    glVertex3f( 1.0,  1.0, 0.0);
    glVertex3f(-1.0,  1.0, 0.0);
    glEnd();

    // reset states
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, compareMode_);
    glEnable(GL_DEPTH_TEST);
}

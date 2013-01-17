/*
 * shadow-map.h
 *
 *  Created on: 24.11.2012
 *      Author: daniel
 */

#ifndef SHADOW_MAP_H_
#define SHADOW_MAP_H_

#include <ogle/animations/animation.h>
#include <ogle/states/camera.h>
#include <ogle/states/light-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/render-tree/state-node.h>

// TODO: increase precision for spot&point lights using the scene frustum
// TODO: handle tesselation in layered shadow shader
//    * force using GL_PATCHES, use dummy stages without any tesselation
//    * is it bad for performance to use dummy tesselation stages ?
//    * alternative, use two shadow shaders. One with tess enabled the
//      other with tess enabled.
//    * or special shader for tess using stuff
// TODO: transparent mesh shadows.
//    1. Colored Stochastic Shadow Maps (CSSM)
//       * use stochastic transparency then ?
//    2. deep shadow maps
//       * deep opacity map @ grass hopper http://prideout.net/blog/?p=69
//    3. transparency shadow map
//       * storing a transparency as a function of depth for each pixel
//    4. render SM with different alpha thresholds

class ShadowRenderState : public RenderState
{
public:
  ShadowRenderState(const ref_ptr<Texture> &texture);
  virtual ~ShadowRenderState() {}

  virtual void enable();

  virtual void set_shadowViewProjectionMatrices(Mat4f *mat) {};

  virtual void pushTexture(TextureState *tex);

  virtual GLboolean isStateHidden(State *state);
  // no transform feedback
  virtual void set_useTransformFeedback(GLboolean) {}
  // shadow map fbo is used
  virtual void pushFBO(FrameBufferObject *tex) {}
  virtual void popFBO() {}

protected:
  GLuint fbo_;
  ref_ptr<Texture> texture_;
};

class LayeredShadowRenderState : public ShadowRenderState
{
public:
  LayeredShadowRenderState(const ref_ptr<Texture> &texture, GLuint numShadowLayer);

  virtual void enable();

  virtual void set_shadowViewProjectionMatrices(Mat4f *mat);

  virtual void set_bones(GLuint numWeights, GLuint numBones);

  virtual void set_modelMat(Mat4f *mat);
  virtual void set_viewMatrix(Mat4f *mat);
  virtual void set_ignoreViewRotation(GLboolean v);
  virtual void set_ignoreViewTranslation(GLboolean v);
  virtual void set_useTesselation(GLboolean v);
  virtual void set_projectionMatrix(Mat4f *mat);

  virtual void pushShaderInput(ShaderInput *att);
  virtual void popShaderInput(const string&) {};
  // update shader overwrites object shaders
  virtual void pushShader(Shader *tex) {}
  virtual void popShader() {}

protected:
  ref_ptr<ShaderState> updateShader_;

  GLuint numShadowLayer_;
  GLint modelMatLoc_;
  GLint numBoneWeightsLoc_;
  GLint viewMatrixLoc_;
  GLint ignoreViewRotationLoc_;
  GLint ignoreViewTranslationLoc_;
  GLint shadowVPMatricesLoc_;
  GLint projectionMatrixLoc_;
  GLint useTesselationLoc_;
  GLint posLocation_;
};

/**
 * Basceclass for shadow maps.
 */
class ShadowMap : public Animation, public State
{
public:
  enum FilterMode {
    // just take a single texel
      SINGLE
    // Bilinear weighted 4-tap filter
    , PCF_4TAB
    , PCF_8TAB_RAND
    // Gaussian 3x3 filter
    , PCF_GAUSSIAN
    //, VSM
  };

  static Mat4f biasMatrix_;

  ShadowMap(const ref_ptr<Light> &light, const ref_ptr<Texture> &texture);

  /**
   * Sets the filtering mode that should be used.
   */
  void set_filteringMode(FilterMode mode);

  /**
   * Sets texture size.
   */
  void set_shadowMapSize(GLuint shadowMapSize);
  /**
   * Sets texture internal format.
   */
  void set_internalFormat(GLenum internalFormat);
  /**
   * Sets texture pixel type.
   */
  void set_pixelType(GLenum pixelType);

  /**
   * Offset the geometry slightly to prevent z-fighting
   * Note that this introduces some light-leakage artifacts.
   */
  void setPolygonOffset(GLfloat factor=1.1, GLfloat units=4096.0);
  /**
   * Moves shadow acne to back faces. But it results in light bleeding
   * artifacts for some models.
   */
  void setCullFrontFaces(GLboolean v=GL_TRUE);

  /**
   * Adds a shadow caster tree to this shadow map.
   * You can just add the render tree of the perspective pass here.
   * But this tree might contain unneeded states (for example color maps).
   * A more optimized application may want to handle a special tree
   * for the shadow map traversal containing only relevant geometry
   * and states.
   */
  void addCaster(const ref_ptr<StateNode> &caster);
  void removeCaster(StateNode *caster);

  /**
   * Traverse all added shadow caster.
   */
  void traverse(RenderState *rs);

  /**
   * Associated texture state.
   */
  const ref_ptr<TextureState>& shadowMap() const;

  /**
   * Draws debug HUD.
   */
  void drawDebugHUD(
      GLenum textureTarget,
      GLenum textureCompareMode,
      GLuint numTextures,
      GLuint textureID,
      const string &fragmentShader);

  // override
  virtual void animate(GLdouble dt);
  virtual GLboolean useGLAnimation() const;
  virtual GLboolean useAnimation() const;

protected:
  ref_ptr<Light> light_;

  ref_ptr<Texture> texture_;
  ref_ptr<TextureState> shadowMap_;
  ref_ptr<ShaderInput1f> shadowMapSize_;

  list< ref_ptr<StateNode> > caster_;
  ref_ptr<State> cullState_;
  ref_ptr<State> polygonOffsetState_;

  ref_ptr<ShaderState> debugShader_;
  GLint debugLayerLoc_;
  GLint debugTextureLoc_;
};

#endif /* SHADOW_MAP_H_ */

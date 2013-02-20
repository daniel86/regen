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
#include <ogle/states/state-node.h>

// TODO: increase precision for spot&point lights using the scene frustum
// TODO: transparent mesh shadows.
//    1. Colored Stochastic Shadow Maps (CSSM)
//       * use stochastic transparency then ?
//    2. deep shadow maps
//       * deep opacity map @ grass hopper http://prideout.net/blog/?p=69
//    3. transparency shadow map
//       * storing a transparency as a function of depth for each pixel
//    4. render SM with different alpha thresholds

class DepthRenderState : public RenderState
{
public:
  virtual GLboolean isStateHidden(State *state);
  virtual void pushFBO(FrameBufferObject *tex) {}
  virtual void popFBO() {}
  virtual void set_useTransformFeedback(GLboolean) {}
};

/**
 * Basceclass for shadow maps.
 */
class ShadowMap : public Animation, public State
{
public:
  enum FilterMode {
    // just take a single texel
      FILTERING_NONE
    // Bilinear weighted 4-tap filter
    , FILTERING_PCF_4TAB
    , FILTERING_PCF_8TAB_RAND
    // Gaussian 3x3 filter
    , FILTERING_PCF_GAUSSIAN
    , FILTERING_VSM
  };

  static Mat4f biasMatrix_;

  ShadowMap(const ref_ptr<Light> &light, GLuint shadowMapSize);

  /**
   * Sets texture size.
   */
  void set_shadowMapSize(GLuint shadowMapSize);
  const ref_ptr<ShaderInput1f>& shadowMapSize() const;

  void set_depthTexture(
      const ref_ptr<Texture> &tex,
      GLenum compare, const string &samplerType);
  /**
   * Sets texture internal format.
   */
  void set_depthFormat(GLenum format);
  /**
   * Sets texture pixel type.
   */
  void set_depthType(GLenum type);

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
  const ref_ptr<TextureState>& shadowSampler() const;

  virtual void update() = 0;
  virtual void computeDepth() = 0;
  virtual void computeMoment() = 0;

  // override
  virtual void glAnimate(GLdouble dt);
  virtual void animate(GLdouble dt);
  virtual GLboolean useGLAnimation() const;
  virtual GLboolean useAnimation() const;

protected:
  ref_ptr<Light> light_;
  ref_ptr<MeshState> textureQuad_;
  ref_ptr<FrameBufferObject> fbo_;

  // XXX redundant
  ref_ptr<ShaderInput1f> shadowMapSizeUniform_;
  GLuint shadowMapSize_;

  list< ref_ptr<StateNode> > caster_;
  ref_ptr<Texture> depthTexture_;
  ref_ptr<TextureState> depthTextureState_;
  ref_ptr<State> cullState_;
  ref_ptr<State> polygonOffsetState_;

  ref_ptr<Texture> momentTexture_;
  ref_ptr<TextureState> momentTextureState_;
  ref_ptr<ShaderState> momentCompute_;

  DepthRenderState depthRenderState_;
  RenderState filteringRenderState_;
};

#endif /* SHADOW_MAP_H_ */

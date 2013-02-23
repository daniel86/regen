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
#include <ogle/filter/filter-sequence.h>

// TODO: transparent mesh shadows.
//    1. Colored Stochastic Shadow Maps (CSSM)
//       * use stochastic transparency then ?
//    2. deep shadow maps
//       * deep opacity map @ grass hopper http://prideout.net/blog/?p=69
//    3. transparency shadow map
//       * storing a transparency as a function of depth for each pixel
//    4. render SM with different alpha thresholds
//    5. sample accumulated alpha color from light perspective

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
      FILTERING_NONE
    , FILTERING_PCF_GAUSSIAN
    , FILTERING_VSM
  };

  static Mat4f biasMatrix_;

  ShadowMap(const ref_ptr<Light> &light,
      GLenum shadowMapTarget,
      GLuint shadowMapSize,
      GLuint shadowMapDepth,
      GLenum depthFormat,
      GLenum depthType);

  /**
   * Depth as seen from light
   */
  const ref_ptr<Texture>& shadowDepth() const;
  /**
   * Sets depth texture internal format.
   */
  void set_depthFormat(GLenum format);
  /**
   * Sets depth texture pixel type.
   */
  void set_depthType(GLenum type);
  /**
   * Sets depth texture size.
   * Note that all other sizes of textures are relative to the
   * depth texture size.
   */
  void set_depthSize(GLuint size);

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
   * Toggles on computation of moments used for VSM.
   */
  void setComputeMoments();
  /**
   * Moments as seen from light (used for VSM).
   */
  const ref_ptr<Texture>& shadowMoments() const;
  /**
   * Sequence of filters applied to moments texture each frame.
   */
  const ref_ptr<FilterSequence>& momentsFilter() const;
  /**
   * Adds default blur filter to the filter sequence.
   */
  void createBlurFilter(
      ShaderConfig &cfg,
      GLuint size, GLfloat sigma,
      GLboolean downsampleTwice=GL_FALSE);

  void addCaster(const ref_ptr<StateNode> &caster);
  void removeCaster(StateNode *caster);
  void traverse(RenderState *rs);

  virtual void update() = 0;
  virtual void computeDepth() = 0;
  virtual void computeMoment() = 0;
  virtual GLenum samplerType() = 0;

  // override
  virtual void glAnimate(GLdouble dt);
  virtual void animate(GLdouble dt);
  virtual GLboolean useGLAnimation() const;
  virtual GLboolean useAnimation() const;

protected:
  ref_ptr<Light> light_;
  ref_ptr<MeshState> textureQuad_;

  ref_ptr<ShaderInput1f> shadowMapSizeUniform_;
  GLuint depthTextureSize_;
  GLuint depthTextureDepth_;

  ref_ptr<FrameBufferObject> depthFBO_;
  ref_ptr<Texture> depthTexture_;
  ref_ptr<TextureState> depthTextureState_;
  list< ref_ptr<StateNode> > caster_;
  ref_ptr<State> cullState_;
  ref_ptr<State> polygonOffsetState_;

  ref_ptr<FrameBufferObject> momentsFBO_;
  ref_ptr<Texture> momentsTexture_;
  ref_ptr<ShaderState> momentsCompute_;
  GLint momentsLayer_;
  GLint momentsNear_;
  GLint momentsFar_;

  ref_ptr<FilterSequence> momentsFilter_;

  DepthRenderState depthRenderState_;
  RenderState filteringRenderState_;

  void createMomentsTexture();
};

#endif /* SHADOW_MAP_H_ */

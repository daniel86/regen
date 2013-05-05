/*
 * shadow-map.h
 *
 *  Created on: 24.11.2012
 *      Author: daniel
 */

#ifndef SHADOW_MAP_H_
#define SHADOW_MAP_H_

#include <regen/gl-types/shader-input-container.h>
#include <regen/states/atomic-states.h>
#include <regen/states/texture-state.h>
#include <regen/states/shader-state.h>
#include <regen/states/camera.h>
#include <regen/states/filter.h>
#include <regen/states/state-node.h>
#include <regen/math/frustum.h>
#include <regen/shading/light-state.h>

namespace regen {
  /**
   * \brief Implements shadow mapping for directional,spot and point Light's.
   *
   * Parallel-Split-Shadow-Mapping (PSSM) is used for directional lights.
   * Omnidirectional shadow mapping is done using a depth cubemap.
   *
   * @see http://http.developer.nvidia.com/GPUGems3/gpugems3_ch10.html
   * @todo transparent shadows...
   *      - Colored Stochastic Shadow Maps (CSSM)
   *              -use stochastic transparency then ?
   *      - deep shadow maps
   *              - deep opacity map http://prideout.net/blog/?p=69
   *      - transparency shadow map
   *              - storing a transparency as a function of depth for each pixel
   *      - render SM with different alpha thresholds
   *      - sample accumulated alpha color from light perspective
   * @todo drop cube map shadows for point lights and
   *       replace it with algorithm with less render passes...
   *      - paper: 'Practical Implementation of Dual Parabloid Shadow Maps' (2 passes)
   *      - paper: 'Dual Sphere-Unfolding Method for Single Pass Omni-directional Shadow Mapping' (single pass)
   */
  class ShadowMap : public State, public Animation, public HasInput
  {
  public:
    /**
     * \brief Configures the shadow map.
     */
    struct Config {
      /** Shadow map width/height. */
      GLuint size;
      /** Number of layers for Frustum split. */
      GLuint numLayer;
      /** Depth texture internal format. */
      GLenum depthFormat;
      /** Depth texture pixel type. */
      GLenum depthType;
      /** Split weight for Frustum splits. */
      GLdouble splitWeight;
      /** Texture  target format (no need to set) */
      GLenum textureTarget;
      Config();
      /**
       * Copy constructor.
       */
      Config(const Config &other);
    };
    /**
     * \brief Configures filtering.
     */
    enum FilterMode {
      FILTERING_NONE,        //!< No special filtering
      FILTERING_PCF_GAUSSIAN,//!< PCF filtering using Gauss kernel
      FILTERING_VSM          //!< VSM filtering
    };

    /**
     * @param f a filter mode.
     * @return filter mode string representaion.
     */
    static string shadowFilterMode(FilterMode f);
    /**
     * @param f a filter mode.
     * @return true if filtering mode uses moments texture.
     */
    static GLboolean useShadowMoments(FilterMode f);
    /**
     * @param f a filter mode.
     * @return true if filtering mode uses moments texture.
     */
    static GLboolean useShadowSampler(FilterMode f);

    /**
     * @param light the shadow casting light.
     * @param sceneCamera a perspective camera.
     * @param cfg shadow map configuration.
     */
    ShadowMap(
        const ref_ptr<Light> &light,
        const ref_ptr<Camera> &sceneCamera,
        const Config &cfg);
    ~ShadowMap();

    /**
     * Offset the geometry slightly to prevent z-fighting
     * Note that this introduces some light-leakage artifacts.
     */
    void setPolygonOffset(GLfloat factor, GLfloat units);
    /**
     * Moves shadow acne to back faces. But it results in light bleeding
     * artifacts for some models.
     */
    void setCullFrontFaces(GLboolean v);

    /**
     * Add a caster to the list of shadow casters.
     * @param caster a caster.
     */
    void addCaster(const ref_ptr<StateNode> &caster);
    /**
     * @param caster a previously added caster.
     */
    void removeCaster(StateNode *caster);

    /**
     * @return depth as seen from Light
     */
    const ref_ptr<Texture>& shadowDepth() const;
    /**
     * @param f depth Texture internal format.
     */
    void set_depthFormat(GLenum f);
    /**
     * @param t depth Texture pixel type.
     */
    void set_depthType(GLenum t);
    /**
     * @param shadowSize depth Texture size.
     */
    void set_depthSize(GLuint shadowSize);

    /**
     * @param numLayer number of Texture layers.
     */
    void set_shadowLayer(GLuint numLayer);
    /**
     * @param splitWeight split weight for Frustum splits.
     */
    void set_splitWeight(GLdouble splitWeight);

    /**
     * Discard specified cube faces.
     */
    void set_isCubeFaceVisible(GLenum face, GLboolean visible);

    /**
     * Toggles on computation of moments used for VSM.
     */
    void setComputeMoments();
    /**
     * @return depth moments as seen from light.
     */
    const ref_ptr<Texture>& shadowMoments() const;
    /**
     * @return depth moments as seen from light.
     */
    const ref_ptr<Texture>& shadowMomentsUnfiltered() const;
    /**
     * @return sequence of filters applied to moments texture each frame.
     */
    const ref_ptr<FilterSequence>& momentsFilter() const;
    /**
     * Adds default blur filter to the moments filter sequence.
     */
    void createBlurFilter(
        GLuint size, GLfloat sigma,
        GLboolean downsampleTwice);
    /**
     * @return number of samples for moment blur.
     */
    const ref_ptr<ShaderInput1i>& momentsBlurSize() const;
    /**
     * @return blur sigma for moments blur.
     */
    const ref_ptr<ShaderInput1f>& momentsBlurSigma() const;

    /**
     * @return shadow transformation matrix.
     */
    const ref_ptr<ShaderInputMat4>& shadowMat() const;
    /**
     * @return shadow camera far value.
     */
    const ref_ptr<ShaderInput1f>& shadowFar() const;
    /**
     * @return shadow camera near value.
     */
    const ref_ptr<ShaderInput1f>& shadowNear() const;
    /**
     * @return the shadow map size.
     */
    const ref_ptr<ShaderInput1f>& shadowSize() const;
    /**
     * @return the shadow map texel size.
     */
    const ref_ptr<ShaderInput1f>& shadowInverseSize() const;

    // override
    void glAnimate(RenderState *rs, GLdouble dt);

  protected:
    ref_ptr<Light> light_;
    ref_ptr<Camera> sceneCamera_;
    ref_ptr<Mesh> momentsQuad_;
    Config cfg_;

    list< ref_ptr<StateNode> > caster_;

    ref_ptr<FrameBufferObject> depthFBO_;
    ref_ptr<Texture> depthTexture_;
    ref_ptr<TextureState> depthTextureState_;

    ref_ptr<ShaderInput1f> shadowSize_;
    ref_ptr<ShaderInput1f> shadowInverseSize_;
    ref_ptr<ShaderInput1f> shadowFar_;
    ref_ptr<ShaderInput1f> shadowNear_;
    ref_ptr<ShaderInputMat4> shadowMat_;
    vector<Frustum*> shadowFrusta_;

    Mat4f *viewMatrix_;
    Mat4f *projectionMatrix_;
    Mat4f *viewProjectionMatrix_;

    ref_ptr<State> cullState_;
    ref_ptr<State> polygonOffsetState_;

    ref_ptr<FrameBufferObject> momentsFBO_;
    ref_ptr<Texture> momentsTexture_;
    ref_ptr<ShaderState> momentsCompute_;
    ref_ptr<FilterSequence> momentsFilter_;
    ref_ptr<ShaderInput1i> momentsBlurSize_;
    ref_ptr<ShaderInput1f> momentsBlurSigma_;
    GLint momentsLayer_;
    GLint momentsNear_;
    GLint momentsFar_;

    GLuint lightPosStamp_;
    GLuint lightDirStamp_;
    GLuint lightRadiusStamp_;
    GLuint projectionStamp_;

    GLboolean isCubeFaceVisible_[6];

    void (ShadowMap::*update_)();
    void updateDirectional();
    void updatePoint();
    void updateSpot();

    void (ShadowMap::*computeDepth_)(RenderState *rs);
    void computeDirectionalDepth(RenderState *rs);
    void computePointDepth(RenderState *rs);
    void computeSpotDepth(RenderState *rs);

    void traverse(RenderState *rs);
  };
} // namespace

#endif /* SHADOW_MAP_H_ */

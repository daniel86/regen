/*
 * glowing.h
 *
 *  Created on: 23.04.2012
 *      Author: daniel
 */

#ifndef GLOWING_H_
#define GLOWING_H_

#include <render-pass.h>
#include <scene.h>
#include <texture.h>
#include <shader-generator.h>

struct GlowSourceData;

struct GlowConfig {
  /**
   * Scale factor for glow buffer.
   * 3.0 means the glow buffer will be 1/3 in size.
   */
  float glowToScreenScale;
  /**
   * Multiply mesh color with glow color ?
   */
  bool useMeshColor;
  /**
   * Toggle a constant glow color multiplied to
   * obtain glow color.
   */
  bool useConstantGlowColor;
  /**
   * Constant glow color, ignored if !useConstantGlowColor
   */
  Vec4f constantGlowColor;
  /**
   * Toggle a variable glow color multiplied to
   * obtain glow color. You can change the uniform by calling
   * GlowRenderer::set_glowColor
   */
  bool useUniformGlowColor;
  /**
   * Used for blending glow onto screen.
   */
  GLenum blendFactorS;
  /**
   * Used for blending glow onto screen.
   */
  GLenum blendFactorD;
  /**
   * Blur pass config.
   */
  BlurConfig blurCfg;
  GlowConfig()
  : glowToScreenScale(3.0f),
    blendFactorS(GL_SRC_ALPHA),
    blendFactorD(GL_ONE_MINUS_SRC_ALPHA),
    useConstantGlowColor(false),
    useUniformGlowColor(false),
    useMeshColor(true)
  {
  }
};

class GlowRenderer
{
public:
  GlowRenderer(
      ref_ptr<Scene> scene,
      const GlowConfig &cfg);
  ~GlowRenderer();

  void addGlowSource(Mesh *mesh,
      ref_ptr<Texture> tex=ref_ptr<Texture>());
  void removeGlowSource(Mesh *mesh);
  bool isGlowSource(Mesh *mesh) const;

  void set_glowColor(const Vec4f &col) {
    glowColorUniform_->set_value(col);
  }

  void resize();
protected:
  // render target for main pass
  ref_ptr<Texture2D> glowTex_;
  // apply blur to glowTex_ and downsample
  ref_ptr<FrameBufferObject> blurGlowBuffer_;
  ref_ptr<Texture2D> blurGlowTex_;

  ref_ptr<ShaderFragmentOutput> glowFragmentOutput_;

  ref_ptr<Scene> scene_;

  ref_ptr<RenderPass> blurGlowHorizontalPass_;
  ref_ptr<RenderPass> blurGlowVerticalPass_;

  ref_ptr<EventCallable> projectionChangedCB_;

  ref_ptr<UniformVec4> glowColorUniform_;

  map<Mesh*, GlowSourceData> glowSources_;

  GlowConfig cfg_;

  GlowSourceData* glowSourceData(Mesh *mesh);
  void makeGlowBlurPass(
      bool isHorizontal, const BlurConfig &blurcfg);

  friend class BlurGlowPass;
  friend class BlurGlowHorizontalPass;
  friend class BlurGlowVerticalPass;
  friend class GlowFragmentOutput;
};

#endif /* GLOWING_H_ */

/*
 * atmosphere.h
 *
 *  Created on: Jan 4, 2014
 *      Author: daniel
 */

#ifndef ATMOSPHERE_H_
#define ATMOSPHERE_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/meshes/sky-box.h>
#include <regen/gl-types/fbo.h>

namespace regen {
  /**
   * Defines the look of the sky.
   */
  struct AtmosphereProperties {
    /** nitrogen profile */
    Vec3f rayleigh;
    /** aerosol profile */
    Vec4f mie;
    /** sun-spotlight */
    GLfloat spot;
    /** scattering strength */
    GLfloat scatterStrength;
    /** Absorption color */
    Vec3f absorption;
  };

  /**
   * \brief Atmospheric scattering.
   * @see http://codeflow.org/entries/2011/apr/13/advanced-webgl-part-2-sky-rendering/
   * @see http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html
   */
  class Atmosphere : public SkyLayer {
  public:
    Atmosphere(
        const ref_ptr<Sky> &sky,
        GLuint cubeMapSize=512,
        GLboolean useFloatBuffer=GL_FALSE,
        GLuint levelOfDetail=0);

    /**
     * Sets given planet properties.
     */
    void setProperties(AtmosphereProperties &p);
    /**
     * Approximates planet properties for earth.
     */
    void setEarth();
    /**
     * Approximates planet properties for mars.
     */
    void setMars();
    /**
     * Approximates planet properties for uranus.
     */
    void setUranus();
    /**
     * Approximates planet properties for venus.
     */
    void setVenus();
    /**
     * Approximates planet properties for imaginary alien planet.
     */
    void setAlien();

    /**
     * Sets brightness for nitrogen profile
     */
    void setRayleighBrightness(GLfloat v);
    /**
     * Sets strength for nitrogen profile
     */
    void setRayleighStrength(GLfloat v);
    /**
     * Sets collect amount for nitrogen profile
     */
    void setRayleighCollect(GLfloat v);
    /**
     * rayleigh profile
     */
    ref_ptr<ShaderInput3f>& rayleigh();

    /**
     * Sets brightness for aerosol profile
     */
    void setMieBrightness(GLfloat v);
    /**
     * Sets strength for aerosol profile
     */
    void setMieStrength(GLfloat v);
    /**
     * Sets collect amount for aerosol profile
     */
    void setMieCollect(GLfloat v);
    /**
     * Sets distribution amount for aerosol profile
     */
    void setMieDistribution(GLfloat v);
    /**
     * aerosol profile
     */
    ref_ptr<ShaderInput4f>& mie();

    /**
     * @param v the spot brightness.
     */
    void setSpotBrightness(GLfloat v);
    /**
     * @return the spot brightness.
     */
    ref_ptr<ShaderInput1f>& spotBrightness();

    /**
     * @param v scattering strength.
     */
    void setScatterStrength(GLfloat v);
    /**
     * @return scattering strength.
     */
    ref_ptr<ShaderInput1f>& scatterStrength();

    /**
     * @param color the absorbtion color.
     */
    void setAbsorbtion(const Vec3f &color);
    /**
     * @return the absorbtion color.
     */
    ref_ptr<ShaderInput3f>& absorbtion();

    const ref_ptr<TextureCube>& cubeMap() const;

    ref_ptr<Mesh> getMeshState();

    ref_ptr<HasShader> getShaderState();

    void set_updateInterval(GLdouble interval);

    GLdouble updateInterval() const;

    // Override
    void updateSky(RenderState *rs, GLdouble dt);

  protected:
    ref_ptr<Sky> sky_;

    GLdouble updateInterval_;
    GLdouble dt_;

    ref_ptr<FBO> fbo_;

    ref_ptr<SkyBox> drawState_;
    ref_ptr<State> updateState_;
    ref_ptr<ShaderState> updateShader_;

    ref_ptr<ShaderInput3f> rayleigh_;
    ref_ptr<ShaderInput4f> mie_;
    ref_ptr<ShaderInput1f> spotBrightness_;
    ref_ptr<ShaderInput1f> scatterStrength_;
    ref_ptr<ShaderInput3f> skyAbsorbtion_;
  };
}

#endif /* ATMOSPHERE_H_ */

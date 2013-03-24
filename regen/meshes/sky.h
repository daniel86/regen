/*
 * sky-box.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef SKY_BOX_H_
#define SKY_BOX_H_

#include <regen/meshes/box.h>
#include <regen/states/camera.h>
#include <regen/states/texture-state.h>
#include <regen/shading/light-state.h>
#include <regen/states/shader-state.h>

namespace regen {
/**
 * \brief A special Box that is not translated by camera movement.
 */
class SkyBox : public Box
{
public:
  SkyBox();

  /**
   * @return the cube map texture.
   */
  const ref_ptr<TextureCube>& cubeMap() const;
  /**
   * @param cubeMap the cube map texture.
   */
  void setCubeMap(const ref_ptr<TextureCube> &cubeMap);

protected:
  ref_ptr<TextureState> texState_;
  ref_ptr<TextureCube> cubeMap_;
};

/**
 * \brief Sky with atmospheric scattering.
 * @see http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html
 * @todo nicer sky...
 *      - nicer, brighter stars. 512 pixel cube map size seems not enough for small stars
 *      - clouds
 *      - moons and satellites
 *      - god rays , volumetric light scattering
 *              - http://http.developer.nvidia.com/GPUGems3/gpugems3_ch13.html
 *      - use irradiance environment map for global illumination
 *              - http://codeflow.org/entries/2011/apr/18/advanced-webgl-part-3-irradiance-environment-map/
 */
class SkyScattering : public SkyBox
{
public:
  /**
   * Defines the look of the sky.
   */
  struct PlanetProperties {
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

  SkyScattering(GLuint cubeMapSize=512, GLboolean useFloatBuffer=GL_FALSE);

  /**
   * Update the sky map.
   * @param rs the render state
   * @param dt the time difference to last call in milliseconds
   */
  void update(RenderState *rs, GLdouble dt);
  /**
   * Light that can be used to approximate influence of the
   * sun. For more accuracy use irradiance environment maps instead.
   */
  ref_ptr<Light>& sun();

  /**
   * Parameters that influence the sun elevation.
   * Note: The sun elevation is approximated with a simple function
   * that interpolates between min and max elevation in a none linear way.
   */
  void setSunElevation(GLdouble dayLength, GLdouble maxElevation, GLdouble minElevation);

  /**
   * Sets number of milliseconds between updates of the
   * sky cubemap.
   */
  void set_updateInterval(GLdouble ms);

  /**
   * Sets the daytime used to place the sun [0,1].
   */
  void set_dayTime(GLdouble time);
  /**
   * Scaled delta t changes day time.
   */
  void set_timeScale(GLdouble scale);

  //////

  /**
   * Sets given planet properties.
   */
  void setPlanetProperties(PlanetProperties &p);
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

  //////

  /**
   * Star map that is blended with the atmosphere.
   */
  void setStarMap(ref_ptr<Texture> starMap);
  /**
   * Star map that is blended with the atmosphere.
   */
  void setStarMapBrightness(GLfloat brightness);
  /**
   * Star map that is blended with the atmosphere.
   */
  ref_ptr<ShaderInput1f>& setStarMapBrightness();

protected:
  GLdouble dayTime_;
  GLdouble timeScale_;
  ref_ptr<FrameBufferObject> fbo_;

  ref_ptr<Light> sun_;

  GLdouble dayLength_;
  GLdouble maxSunElevation_;
  GLdouble minSunElevation_;

  ref_ptr<State> updateState_;
  ref_ptr<ShaderState> updateShader_;
  ref_ptr<ShaderInput3f> sunDirection_;
  ref_ptr<ShaderInput3f> rayleigh_;
  ref_ptr<ShaderInput4f> mie_;
  ref_ptr<ShaderInput1f> spotBrightness_;
  ref_ptr<ShaderInput1f> scatterStrength_;
  ref_ptr<ShaderInput3f> skyAbsorbtion_;
  ref_ptr<ShaderInputMat4> mvpMatrices_;

  ////////////
  ////////////

  ref_ptr<Texture> starMap_;
  ref_ptr<State> starMapState_;
  ref_ptr<ShaderState> starMapShader_;
  ref_ptr<ShaderInput1f> starMapBrightness_;
  ref_ptr<ShaderInputMat4> starMapRotation_;

  void updateSky(RenderState *rs);
  void updateStarMap(RenderState *rs);
  void updateMoons(RenderState *rs);
};
} // namespace

#endif /* SKY_BOX_H_ */

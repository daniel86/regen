/*
 * sky-box.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef SKY_BOX_H_
#define SKY_BOX_H_

#include <ogle/meshes/box.h>
#include <ogle/states/camera.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/light-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/render-state.h>

/**
 * Sky boxes are not translated by camera movement.
 * They are always centered at (0,0,0) in view space.
 */
class SkyBox : public Box
{
public:
  SkyBox();

  /**
   * The cube map texture.
   */
  const ref_ptr<TextureCube>& cubeMap() const;
  /**
   * The cube map texture.
   */
  void setCubeMap(const ref_ptr<TextureCube> &cubeMap);

protected:
  ref_ptr<TextureState> texState_;
  ref_ptr<TextureCube> cubeMap_;

  GLboolean ignoredViewRotation_;
};

//////////
//////////
//////////

/**
 * Defines the look of the sky.
 */
struct PlanetProperties {
  // nitrogen profile
  Vec3f rayleigh;
  // aerosol profile
  Vec4f mie;
  // sun-spotlight
  GLfloat spot;
  GLfloat scatterStrength;
  Vec3f absorbtion;
};

/**
 * TODO: nicer sky...
 *      - nicer, brighter stars. 512 pixel cube map size seems not enough for small stars
 *      - clouds
 *      - moons and satellites
 *      - use irradiance environment map for global illumination
 */
class DynamicSky : public SkyBox, public Animation
{
public:
  DynamicSky(GLuint cubeMapSize=512, GLboolean useFloatBuffer=GL_FALSE);
  ~DynamicSky();

  /**
   * Light that can be used to approximate influence of the
   * sun. For more accuracy use irradiance environment maps instead.
   */
  ref_ptr<DirectionalLight>& sun();

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

  void setSpotBrightness(GLfloat v);
  ref_ptr<ShaderInput1f>& spotBrightness();

  void setScatterStrength(GLfloat v);
  ref_ptr<ShaderInput1f>& scatterStrength();

  void setAbsorbtion(const Vec3f &color);
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

  // override
  virtual void glAnimate(GLdouble dt);
  virtual void animate(GLdouble dt);
  virtual GLboolean useGLAnimation() const;
  virtual GLboolean useAnimation() const;

protected:
  GLdouble dayTime_;
  GLdouble timeScale_;
  GLdouble updateInterval_;
  GLdouble dt_;
  GLuint fbo_;

  ref_ptr<DirectionalLight> sun_;

  GLdouble dayLength_;
  GLdouble maxSunElevation_;
  GLdouble minSunElevation_;

  RenderState rs_;
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

  void updateSky();
  void updateStarMap();
  void updateMoons();
};

#endif /* SKY_BOX_H_ */

/*
 * sky-box.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef SKY_BOX_H_
#define SKY_BOX_H_

#include <ogle/models/cube.h>
#include <ogle/states/camera.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/light-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/render-state.h>

/**
 * Sky boxes are not translated by camera movement.
 * They are always centered at (0,0,0) in view space.
 */
class SkyBox : public UnitCube
{
public:
  SkyBox(GLfloat far);

  void resize(GLfloat far);

  ref_ptr<TextureCube>& cubeMap();
  void setCubeMap(ref_ptr<TextureCube> &cubeMap);

  // override
  virtual void configureShader(ShaderConfig *cfg);
  virtual void enable(RenderState *rs);
  virtual void disable(RenderState *rs);
protected:
  ref_ptr<TextureState> texState_;
  ref_ptr<TextureCube> cubeMap_;

  GLboolean ignoredViewRotation_;
};

class StarSkyMap : public TextureCube
{
public:
  StarSkyMap(GLuint cubeMapSize);
  ~StarSkyMap();

  GLboolean readStarFile(const string &path, GLuint numStars);
  GLboolean readStarFile_short(const string &path, GLuint numStars);

  /**
   * Uploads star data to buffer bound at GL_ARRAY_BUFFER.
   */
  void uploadVertexData();
  /**
   * Enables star data on buffer bound at GL_ARRAY_BUFFER.
   */
  void enableVertexData();
  /**
   * Disables star data on buffer bound at GL_ARRAY_BUFFER.
   */
  void disableVertexData();

  void update();

protected:
  GLuint numStars_;
  GLfloat *vertexData_;
  GLuint vertexSize_;

  GLuint fbo_;

  ref_ptr<ShaderState> updateShader_;
  ref_ptr<State> updateState_;
  ref_ptr<ShaderInputMat4> mvpMatrices_;
  RenderState rs_;
};

class DynamicSky : public SkyBox, public Animation
{
public:
  DynamicSky(ref_ptr<MeshState> orthoQuad,
      GLfloat far, GLuint cubeMapSize=512);

  void setBrightStarMap(const ref_ptr<TextureCube> &starMap);
  void setMilkyWayMap(const ref_ptr<TextureCube> &milkyWayMap);

  void setStarBrightness(GLfloat brightness);
  void setMilkyWayBrightness(GLfloat brightness);

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

  void setRayleighBrightness(GLfloat v);
  void setRayleighStrength(GLfloat v);
  void setRayleighCollect(GLfloat v);
  ref_ptr<ShaderInput3f>& rayleigh();

  void setMieBrightness(GLfloat v);
  void setMieStrength(GLfloat v);
  void setMieCollect(GLfloat v);
  void setMieDistribution(GLfloat v);
  ref_ptr<ShaderInput4f>& mie();

  void setSpotBrightness(GLfloat v);
  ref_ptr<ShaderInput1f>& spotBrightness();

  void setScatterStrength(GLfloat v);
  ref_ptr<ShaderInput1f>& scatterStrength();

  void setAbsorbtion(const Vec3f &color);
  ref_ptr<ShaderInput3f>& skyColor();

  /**
   * @param time the day time of maximal elevation
   * @param maxAngle sun elevates to this angle at time of maximal elevation.
   * @param minAngle sun elevates to this angle at time of minimum elevation.
   * @param orientation of the sun when elevation is maximal.
   */
  void setSunElevation(
      GLdouble time,
      GLdouble maxAngle,
      GLdouble minAngle,
      GLdouble orientation);

  // presets
  void setEarth();
  void setMars();
  void setUranus();
  void setVenus();
  void setAlien();

  void updateSky();

  /**
   * Light that can be used to approximate influence of the
   * sun. For more accuracy use irradiance environment maps instead.
   */
  ref_ptr<DirectionalLight>& sun();

  // override
  virtual void animate(GLdouble dt);
  virtual void updateGraphics(GLdouble dt);

protected:
  GLdouble dayTime_;
  GLdouble timeScale_;
  GLdouble updateInterval_;
  GLdouble dt_;

  GLuint fbo_;

  ref_ptr<DirectionalLight> sun_;
  GLdouble maxElevationTime_;
  GLdouble maxElevationAngle_;
  GLdouble minElevationAngle_;
  GLdouble maxElevationOrientation_;

  ref_ptr<TextureCube> brightStarMap_;
  GLint starMapChannel_;
  GLfloat starBrightness_;

  ref_ptr<TextureCube> milkyWayMap_;
  GLint milkyWayMapChannel_;
  GLfloat milkyWayBrightness_;

  RenderState rs_;
  ref_ptr<State> updateState_;
  ref_ptr<ShaderState> updateShader_;

  // uniforms for updating the sky
  ref_ptr<ShaderInputMat4> planetRotation_;
  ref_ptr<ShaderInput3f> lightDir_;
  ref_ptr<ShaderInput3f> rayleigh_;
  ref_ptr<ShaderInput4f> mie_;
  ref_ptr<ShaderInput1f> spotBrightness_;
  ref_ptr<ShaderInput1f> scatterStrength_;
  ref_ptr<ShaderInput3f> skyAbsorbtion_;
  ref_ptr<ShaderInput1f> starVisibility_;
  ref_ptr<ShaderInput1f> milkywayVisibility_;
};

#endif /* SKY_BOX_H_ */

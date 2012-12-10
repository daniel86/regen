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
#include <ogle/states/vbo-state.h>
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

//////////

class StarSky
{
public:
  StarSky();
  ~StarSky();

  void set_starSize(GLfloat size, GLfloat variance);
  void set_starAlphaScale(GLfloat alphaScale);

  GLboolean readStarFile(const string &path, GLuint numStars);
  GLboolean readStarFile_short(const string &path, GLuint numStars);

protected:
  GLuint numStars_;
  GLfloat *vertexData_;
  GLuint vertexSize_;
  ref_ptr<ShaderInput3f> posAttribute_;
  ref_ptr<ShaderInput4f> colorAttribute_;

  GLfloat starAlpha_;
  GLfloat starSize_;
  GLfloat starSizeVariance_;

  // is called when the star data was loaded
  virtual void starDataUpdated() {};
};

class StarSkyMap : public TextureCube, public StarSky
{
public:
  StarSkyMap(GLuint cubeMapSize);
  ~StarSkyMap();

  void update();

protected:
  GLuint fbo_;

  ref_ptr<ShaderState> updateShader_;
  ref_ptr<State> updateState_;
  ref_ptr<ShaderInputMat4> mvpMatrices_;
  RenderState rs_;
};

class StarSkyMesh : public MeshState, public StarSky
{
public:
  StarSkyMesh();

  // override
  virtual void configureShader(ShaderConfig *cfg);

protected:
  ref_ptr<VBOState> vboState_;

  virtual void starDataUpdated();
};

//////////

struct PlanetProperties {
  // tilt in degree
  GLdouble tilt;
  // distance to sun in astro units
  GLdouble sunDistance;
  // diameter in kilometers
  GLdouble diameter;
  // location on the planet relative to equator
  GLdouble longitude;
  GLdouble latitude;

  Vec3f rayleigh;
  Vec4f mie;
  GLfloat spot;
  GLfloat scatterStrength;
  Vec3f absorbtion;
};

class DynamicSky : public SkyBox, public Animation
{
public:
  DynamicSky(ref_ptr<MeshState> orthoQuad,
      GLfloat far, GLuint cubeMapSize=512, GLboolean useFloatBuffer=GL_FALSE);
  ~DynamicSky();

  void setPlanetProperties(PlanetProperties &p);

  void setBrightStarMap(const ref_ptr<TextureCube> &starMap, GLfloat brightness);
  void setMilkyWayMap(const ref_ptr<TextureCube> &milkyWayMap, GLfloat brightness);

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

  // presets
  void setEarth(GLdouble longitude, GLdouble latitude);
  void setMars(GLdouble longitude, GLdouble latitude);
  void setUranus(GLdouble longitude, GLdouble latitude);
  void setVenus(GLdouble longitude, GLdouble latitude);
  void setAlien(GLdouble longitude, GLdouble latitude);

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

  GLdouble sunDistance_;
  GLdouble planetDiameter_;
  Vec3f planetAxis_;
  Vec3f yAxis_;
  Vec3f zAxis_;
  GLdouble timeOffset_;

  ref_ptr<DirectionalLight> sun_;

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

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
#include <ogle/gl-types/volume-texture.h>

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
//////////
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

struct MoonProperties {
  GLdouble inclination; // in grad
  GLdouble distance;    // in km
  GLdouble period;      // in days
  GLdouble diameter;    // in km
  Vec3f color;
  string moonMap;

  Vec3f dir;
  Vec3f axis;
  GLdouble theta;
  ref_ptr<Texture> texture;
};

class DynamicSky : public SkyBox, public Animation
{
public:
  DynamicSky(ref_ptr<MeshState> orthoQuad,
      GLfloat far,
      GLuint cubeMapSize=512,
      GLboolean useFloatBuffer=GL_FALSE);
  ~DynamicSky();

  /**
   * Light that can be used to approximate influence of the
   * sun. For more accuracy use irradiance environment maps instead.
   */
  ref_ptr<DirectionalLight>& sun();

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

  void setPlanetProperties(PlanetProperties &p);
  void setEarth(GLdouble longitude, GLdouble latitude);
  void setMars(GLdouble longitude, GLdouble latitude);
  void setUranus(GLdouble longitude, GLdouble latitude);
  void setVenus(GLdouble longitude, GLdouble latitude);
  void setAlien(GLdouble longitude, GLdouble latitude);

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
  ref_ptr<ShaderInput3f>& absorbtion();

  //////

  void setStarMap(ref_ptr<TextureCube> &starMap);
  void setStarMapBrightness(GLfloat brightness);
  ref_ptr<ShaderInput1f>& setStarMapBrightness();

  //////

  void setMoons(MoonProperties *moons, GLuint numMoons);

  // override
  virtual void animate(GLdouble dt);
  virtual void updateGraphics(GLdouble dt);

protected:
  ref_ptr<MeshState> orthoQuad_;

  GLdouble dayTime_;
  GLdouble timeScale_;
  GLdouble updateInterval_;
  GLdouble dt_;
  GLuint fbo_;

  GLdouble planetDiameter_;
  Vec3f planetAxis_;
  Vec3f yAxis_;
  Vec3f zAxis_;
  GLdouble timeOffset_;

  ref_ptr<DirectionalLight> sun_;

  RenderState rs_;
  ref_ptr<State> updateState_;
  ref_ptr<ShaderState> updateShader_;
  ref_ptr<ShaderInput3f> sunDirection_;
  ref_ptr<ShaderInput1f> sunDistance_;
  ref_ptr<ShaderInput3f> rayleigh_;
  ref_ptr<ShaderInput4f> mie_;
  ref_ptr<ShaderInput1f> spotBrightness_;
  ref_ptr<ShaderInput1f> scatterStrength_;
  ref_ptr<ShaderInput3f> skyAbsorbtion_;
  ref_ptr<ShaderInputMat4> mvpMatrices_;

  ////////////
  ////////////

  ref_ptr<TextureCube> starMap_;
  ref_ptr<State> starMapState_;
  ref_ptr<ShaderState> starMapShader_;
  ref_ptr<ShaderInput1f> starMapBrightness_;
  ref_ptr<ShaderInputMat4> starMapRotation_;

  ////////////
  ////////////

  MoonProperties *moons_;
  GLuint numMoons_;
  GLuint moonVertexSize_;
  byte *moonData_;
  ref_ptr<State> moonState_;
  ref_ptr<ShaderState> moonShader_;
  ref_ptr<ShaderInput4f> moonDirection_;
  ref_ptr<ShaderInput3f> moonColor_;
  ref_ptr<Texture2DArray> moonMaps_;
  GLint moonMapChannel_;

  void updateSky();
  void updateStarMap();
  void updateMoons(
      const Vec3f &cameraY,
      const Vec3f &cameraZ,
      const Vec3f &location);
};

#endif /* SKY_BOX_H_ */

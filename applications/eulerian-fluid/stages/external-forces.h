
#ifndef EULERIAN_ADD_FORCE_H_
#define EULERIAN_ADD_FORCE_H_

#include "primitive.h"
#include "helper.h"

typedef struct {
  FluidBuffer buffer;
  Vec3f pos;
  float radius;
  float width;
  float height;
  // 0=cirlce, 1=texture, 2=rectangle
  int mode;
  Vec3f value;
  ref_ptr<Texture> tex;
}SplatSource;

/**
 * The fourth term encapsulates acceleration due to external forces applied to the fluid.
 * These forces may be either local forces or body forces.
 * Local forces are applied to a specific region of the fluid - for example, the force of a fan blowing air.
 * Body forces, such as the force of gravity, apply evenly to the entire fluid.
 */
class EulerianSplat : public EulerianStage
{
public:
  EulerianSplat(EulerianPrimitive*);

  void addSplatSource(ref_ptr<SplatSource> source);
  void removeSplatSource(ref_ptr<SplatSource> source);

  void set_obstaclesTexture(ref_ptr<Texture> tex);

  virtual void update();

protected:
  list< ref_ptr<SplatSource> > slpatSources_;

  ref_ptr<Texture> obstaclesTexture_;

  ref_ptr<Shader> splatShader_;
  GLuint positionLoc_;
  GLuint radiusLoc_;
  GLuint splatModeLoc_;
  GLuint widthLoc_;
  GLuint heightLoc_;
  GLuint fillColorLoc_;
  GLuint splatTextureLoc_;
};

class EulerianGravity : public EulerianStage
{
public:
  EulerianGravity(EulerianPrimitive*);

  void set_velocityBuffer(const FluidBuffer &buffer);
  void set_obstaclesTexture(ref_ptr<Texture> tex);
  void set_levelSetTexture(ref_ptr<Texture> tex);

  void addGravitySource(const Vec3f &source);

  virtual void update();

protected:
  list< Vec3f > gravitySources_;

  FluidBuffer velocityBuffer_;
  ref_ptr<Texture> obstaclesTexture_;
  ref_ptr<Texture> levelSetTexture_;

  ref_ptr<Shader> gravityShader_;
  GLuint gravityLoc_;
};



typedef struct {
  Vec3f velocity;
  Vec3f center;
  float radius;
}LiquidStreamSource;

class EulerianLiquidStream : public EulerianStage
{
public:
  EulerianLiquidStream(EulerianPrimitive*);

  void set_velocityBuffer(const FluidBuffer &buffer);
  void set_levelSetBuffer(const FluidBuffer &buffer);
  void set_obstaclesTexture(ref_ptr<Texture> tex);

  void addStreamSource(ref_ptr<LiquidStreamSource> s) {
    streamSources_.push_back(s);
  }
  void removeStreamSource(ref_ptr<LiquidStreamSource> s) {
    streamSources_.remove(s);
  }

  virtual void update();

protected:
  FluidBuffer velocityBuffer_;
  FluidBuffer levelSetBuffer_;
  ref_ptr<Texture> obstaclesTexture_;

  list< ref_ptr<LiquidStreamSource> > streamSources_;

  ref_ptr<Shader> liquidStreamShader_;
  GLuint streamColorLoc_;
  GLuint streamCenterLoc_;
  GLuint streamRadiusLoc_;
  GLuint outputColorLoc_;
};



class InjectLiquidStage : public EulerianStage {
public:
  InjectLiquidStage(EulerianPrimitive *primitive, UniformFloat *liquidHeight_);
  void set_levelSetBuffer(const FluidBuffer &buffer);
  void update();
protected:
  ref_ptr<Shader> injectLiquidShader_;
  FluidBuffer levelSetBuffer_;
};

#endif /* EULERIAN_ADD_FORCE_H_ */

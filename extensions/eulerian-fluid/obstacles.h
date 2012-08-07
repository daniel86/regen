/*
 * obstacles.h
 *
 *  Created on: 24.02.2012
 *      Author: daniel
 */

#ifndef OBSTACLES_H_
#define OBSTACLES_H_

#include "primitive.h"
#include "texture.h"

/**
 * Interface that provides a obstacle texture.
 * There is a update method for dynamic obstacles,
 * it is called each update step.
 *
 * The obstacle texture contains a flag in the r component, if it is bigger
 * then zero the region is considered to be occupied by an obstacle.
 * The other components supposed to share the velocity of the obstacle.
 * This way moving obstacles can push the fluid by their velocity.
 */
class EulerianObstacles {
public:
  EulerianObstacles(ref_ptr<EulerianPrimitive> primitive) : primitive_(primitive) {}
  virtual void update(float dt) {}
  /**
   * Returns the obstacle texture.
   */
  virtual ref_ptr<Texture> tex() = 0;
protected:
  ref_ptr<EulerianPrimitive> primitive_;
};

/////////////

class PixelDataObstacles : public EulerianObstacles {
public:
  PixelDataObstacles(ref_ptr<EulerianPrimitive> primitive);
  ~PixelDataObstacles();

  virtual void updateTextureData();
  virtual void updateObstacles() {}
  float obstacleValue(Vec2i pos);
  float obstacleValue(Vec3i pos);
  virtual ref_ptr<Texture> tex() {
    return tex_;
  }
  int index(int x, int y);
  int index(int x, int y, int z);
protected:
  ref_ptr<Texture> tex_;
  GLuint width_;
  GLuint height_;
  float *pixelData_;
};

class MovableObstacle3D : public PixelDataObstacles {
public:
  MovableObstacle3D(ref_ptr<EulerianPrimitive> primitive, float rValue=0.9f, float radius=30.0f);
  virtual void update(float dt);

  void move(const Vec3f &ds);
  void set_position(const Vec3f &pos);
  bool select(Vec3i pos);
  void unselect();
protected:
  Vec3f pos_;
  Vec3f lastPos_;
  Vec3f velocity_;
  float rValue_;
  bool isSelected_;
  bool positionChanged_;
  bool velocitySet_;
};

class SphereObstacle : public MovableObstacle3D {
public:
  SphereObstacle(ref_ptr<EulerianPrimitive> primitive, float radius=30.0f);
  virtual void updateObstacles();
protected:
  float radius_;
};

class MovableObstacle2D : public PixelDataObstacles {
public:
  MovableObstacle2D(ref_ptr<EulerianPrimitive> primitive, float rValue=0.9f, float radius=30.0f);
  virtual void update(float dt);

  void move(const Vec2f &ds);
  void set_position(const Vec2f &pos);
  bool select(Vec2i pos);
  void unselect();
protected:
  Vec2f pos_;
  Vec2f lastPos_;
  Vec2f velocity_;
  float rValue_;
  bool isSelected_;
  bool positionChanged_;
  bool velocitySet_;
};

class CircleObstacle : public MovableObstacle2D {
public:
  CircleObstacle(ref_ptr<EulerianPrimitive> primitive, float radius=30.0f);
  virtual void updateObstacles();
protected:
  float radius_;
};

class RectangleObstacle : public MovableObstacle2D {
public:
  RectangleObstacle(ref_ptr<EulerianPrimitive> primitive,
      float width=30.0f, float height=30.0f);
  virtual void updateObstacles();
protected:
  float width_;
  float height_;
};

////////////////

/**
 * Static obstacles loaded from file.
 */
class EulerianImageTextureObstacles : public EulerianObstacles {
public:
  /**
   * just load the texture and provide the texture handle.
   */
  EulerianImageTextureObstacles(
      ref_ptr<EulerianPrimitive> primitive,
      const string &imageFilePath);
  virtual ref_ptr<Texture> tex() {
    return tex_;
  }
protected:
  ref_ptr<Texture> tex_;
};

#endif /* OBSTACLES_H_ */

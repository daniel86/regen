/*
 * obstacles.cpp
 *
 *  Created on: 27.02.2012
 *      Author: daniel
 */

#include "include/obstacles.h"
#include "image-texture.h"
#include <volume-texture.h>

EulerianImageTextureObstacles::EulerianImageTextureObstacles(
    ref_ptr<EulerianPrimitive> primitive,
    const string &imageFilePath)
: EulerianObstacles(primitive)
{
  // load image texture and scale to grid size
  tex_ = ref_ptr<Texture>::manage(
      new ImageTexture(imageFilePath,
          primitive->width(),
          primitive->height(),
          primitive->depth()));
}



PixelDataObstacles::PixelDataObstacles(ref_ptr<EulerianPrimitive> primitive)
: EulerianObstacles(primitive)
{
  int w = primitive_->width();
  int h = primitive_->height();
  int d = primitive_->depth();
  int numElems = w*h*4;
  if(!primitive_->is2D()) numElems *= d;

  pixelData_ = new float[numElems];

  // create texture
  if(primitive_->is2D()) {
    tex_ = ref_ptr<Texture>::manage( new Texture2D );
  } else {
    tex_ = ref_ptr<Texture>::manage( new Texture3D );
    ((Texture3D*)tex_.get())->set_numTextures(d);
  }
  tex_->set_pixelType( GL_FLOAT );
  tex_->set_internalFormat( GL_RGBA16F );
  tex_->set_format( GL_RGBA );
  tex_->set_size( w, h );
  tex_->set_data( pixelData_ );
}

PixelDataObstacles::~PixelDataObstacles() {
  delete pixelData_;
}

int PixelDataObstacles::index(int x, int y) {
  return y*primitive_->width() + x;
}
int PixelDataObstacles::index(int x, int y, int z) {
  return z*primitive_->width()*primitive_->height() + y*primitive_->width() + x;
}

void PixelDataObstacles::updateTextureData() {
  int w = primitive_->width();
  int h = primitive_->height();
  int d = primitive_->depth();
  int numElems = w*h*4;
  if(!primitive_->is2D()) numElems *= d;

  // clear texture to black
  for(int i=0; i<numElems; ++i) pixelData_[i] = 0.0f;

  // border around the texture to prevent fluid from escaping
  float borderValue = 0.2;
  if(primitive_->is2D()) {
    int x;
    for(x=0; x<w*4; x+=4) pixelData_[x] = borderValue;
    for(x=numElems-w*4; x<numElems; x+=4) pixelData_[x] = borderValue;
    for(x=0; x<numElems; x+=w*4) pixelData_[x] = borderValue;
    for(x=w*4-4; x<numElems; x+=w*4) pixelData_[x] = borderValue;
  } else {
    int x, y, z;
    x = 0;   for(y=0; y<h; ++y) for(z=0; z<d; ++z) pixelData_[ index(x,y,z)*4 ] = borderValue;
    x = w-1; for(y=0; y<h; ++y) for(z=0; z<d; ++z) pixelData_[ index(x,y,z)*4 ] = borderValue;
    y = 0;   for(x=0; x<w; ++x) for(z=0; z<d; ++z) pixelData_[ index(x,y,z)*4 ] = borderValue;
    y = h-1; for(x=0; x<w; ++x) for(z=0; z<d; ++z) pixelData_[ index(x,y,z)*4 ] = borderValue;
    z = 0;   for(x=0; x<w; ++x) for(y=0; y<h; ++y) pixelData_[ index(x,y,z)*4 ] = borderValue;
    z = d-1; for(x=0; x<w; ++x) for(y=0; y<h; ++y) pixelData_[ index(x,y,z)*4 ] = borderValue;
  }

  // the obstacle itself
  updateObstacles();

  // upload new texture data to GL
  tex_->bind();
  tex_->texImage();
  // use linear filtering, no mipmapping
  tex_->set_filter(GL_LINEAR, GL_LINEAR);
  tex_->set_wrapping( GL_CLAMP_TO_EDGE );
}

float PixelDataObstacles::obstacleValue(Vec2i pos) {
  int w = primitive_->width();
  int h = primitive_->height();
  // check if range valid
  if(pos.x > w || pos.y > h) return -1.0f;
  // check if r value is bugger then zero
  return pixelData_[index(pos.x, pos.y)*4];
}
float PixelDataObstacles::obstacleValue(Vec3i pos) {
  int w = primitive_->width();
  int h = primitive_->height();
  int d = primitive_->height();
  // check if range valid
  if(pos.x > w || pos.y > h || pos.z > d) return -1.0f;
  // check if r value is bugger then zero
  return pixelData_[index(pos.x, pos.y, pos.z)*4];
}



MovableObstacle2D::MovableObstacle2D(ref_ptr<EulerianPrimitive> primitive, float rValue, float radius)
: PixelDataObstacles(primitive),
  pos_( (Vec2f) {primitive_->width()*0.5f, primitive_->height()*0.5f} ),
  lastPos_( pos_ ),
  velocity_( (Vec2f) {0.0f, 0.0f} ),
  rValue_( rValue ),
  positionChanged_( true ),
  velocitySet_( false ),
  isSelected_( false )
{
}
void MovableObstacle2D::update(float dt) {
  if(positionChanged_) {
    // velocity in grid pixels per second
    velocity_ = (dt>0.0f ? (pos_ - lastPos_) * (1.0/dt) : (Vec2f){0.0f,0.0f} );
    updateTextureData();
    positionChanged_ = false;
    velocitySet_ = true;
    lastPos_ = pos_;
  } else if(velocitySet_) {
    velocity_ = (Vec2f) {0.0f, 0.0f};
    updateTextureData();
    velocitySet_ = false;
  }
}
void MovableObstacle2D::move(const Vec2f &ds)
{
  if(isSelected_) {
    pos_ += ds;
    positionChanged_ = true;
  }
}
void MovableObstacle2D::set_position(const Vec2f &pos) {
  pos_ = pos;
  lastPos_ = pos;
  positionChanged_ = true;
}
bool MovableObstacle2D::select(Vec2i pos) {
  float v = obstacleValue(pos);
  isSelected_ = (v == rValue_);
  return v > 0.0f;
}
void MovableObstacle2D::unselect() {
  isSelected_ = false;
}

CircleObstacle::CircleObstacle(ref_ptr<EulerianPrimitive> primitive, float radius)
: MovableObstacle2D(primitive, 0.9f), radius_(radius)
{
}
inline float length(const Vec2i &a)
{
  return sqrt( pow(a.x,2) + pow(a.y,2) );
}
void CircleObstacle::updateObstacles() {
  int w = primitive_->width();
  int h = primitive_->height();
  int xMin = max(0, (int) ( (int) pos_.x - radius_));
  int xMax = min(w, (int) ( (int) pos_.x + radius_ + 1));
  int yMin = max(0, (int) ( (int) pos_.y - radius_));
  int yMax = min(h, (int) ( (int) pos_.y + radius_ + 1));
  int x, y;
  Vec2i posi((int)pos_.x,(int)pos_.y);
  for(x=xMin; x<xMax; ++x)
  {
    for(y=yMin; y<yMax; ++y)
    {
      Vec2i p(x,y);
      float l = length( posi - p );
      if( l > radius_ ) continue;
      int i = index(x,y)*4;
      float threshold = radius_ - 2.0f;
      if(l<threshold) {
        pixelData_[i+0] = rValue_;
      } else {
        // anti aliasing
        pixelData_[i+0] = rValue_ * (1.0f - (l-threshold)/(radius_-threshold));
      }
      pixelData_[i+1] = velocity_.x;
      pixelData_[i+2] = velocity_.y;
      pixelData_[i+3] = 0.0f;
    }
  }
}

RectangleObstacle::RectangleObstacle(ref_ptr<EulerianPrimitive> primitive,
      float width, float height)
: MovableObstacle2D(primitive, 0.9f), width_(width), height_(height)
{
}
void RectangleObstacle::updateObstacles() {
  int w = primitive_->width();
  int h = primitive_->height();
  int xMin = max(0, (int) ( pos_.x - width_/2));
  int xMax = min(w, (int) ( pos_.x + width_/2));
  int yMin = max(0, (int) ( pos_.y - height_/2));
  int yMax = min(h, (int) ( pos_.y + height_/2));
  int x, y;
  for(x=xMin; x<xMax; ++x)
  {
    for(y=yMin; y<yMax; ++y)
    {
      int i = index(x,y)*4;
      pixelData_[i+0] = rValue_;
      pixelData_[i+1] = velocity_.x;
      pixelData_[i+2] = velocity_.y;
      pixelData_[i+3] = 0.0f;
    }
  }
}


////////

MovableObstacle3D::MovableObstacle3D(ref_ptr<EulerianPrimitive> primitive, float rValue, float radius)
: PixelDataObstacles(primitive),
  pos_( (Vec3f) {primitive_->width()*0.5f, primitive_->height()*0.5f, primitive_->depth()*0.5f} ),
  lastPos_( pos_ ),
  velocity_( (Vec3f) {0.0f, 0.0f, 0.0f} ),
  rValue_( rValue ),
  positionChanged_( true ),
  velocitySet_( false ),
  isSelected_( false )
{
}
void MovableObstacle3D::update(float dt) {
  if(positionChanged_) {
    // velocity in grid pixels per second
    velocity_ = (dt>0.0f ? (pos_ - lastPos_) * (1.0/dt) : (Vec3f){0.0f,0.0f,0.0f} );
    updateTextureData();
    positionChanged_ = false;
    velocitySet_ = true;
    lastPos_ = pos_;
  } else if(velocitySet_) {
    velocity_ = (Vec3f) {0.0f, 0.0f,0.0f};
    updateTextureData();
    velocitySet_ = false;
  }
}
void MovableObstacle3D::move(const Vec3f &ds)
{
  if(isSelected_) {
    pos_ += ds;
    positionChanged_ = true;
  }
}
void MovableObstacle3D::set_position(const Vec3f &pos) {
  pos_ = pos;
  lastPos_ = pos;
  positionChanged_ = true;
}
bool MovableObstacle3D::select(Vec3i pos) {
  float v = obstacleValue(pos);
  isSelected_ = (v == rValue_);
  return v > 0.0f;
}
void MovableObstacle3D::unselect() {
  isSelected_ = false;
}

SphereObstacle::SphereObstacle(ref_ptr<EulerianPrimitive> primitive, float radius)
: MovableObstacle3D(primitive, 0.9f), radius_(radius)
{
}
inline float length(const Vec3i &a)
{
  return sqrt( pow(a.x,2) + pow(a.y,2) + pow(a.z,2) );
}
void SphereObstacle::updateObstacles() {
  int w = primitive_->width();
  int h = primitive_->height();
  int d = primitive_->depth();
  int xMin = max(0, (int) ( (int) pos_.x - radius_));
  int xMax = min(w, (int) ( (int) pos_.x + radius_ + 1));
  int yMin = max(0, (int) ( (int) pos_.y - radius_));
  int yMax = min(h, (int) ( (int) pos_.y + radius_ + 1));
  int zMin = max(0, (int) ( (int) pos_.z - radius_));
  int zMax = min(d, (int) ( (int) pos_.z + radius_ + 1));
  int x, y, z;
  Vec3i posi((int)pos_.x,(int)pos_.y,(int)pos_.z);
  for(x=xMin; x<xMax; ++x)
  {
    for(y=yMin; y<yMax; ++y)
    {
      for(z=zMin; z<zMax; ++z)
      {
        Vec3i p(x,y,z);
        if( length( posi - p ) > radius_ ) continue;
        int i = index(x,y,z)*4;
        pixelData_[i+0] = rValue_;
        pixelData_[i+1] = velocity_.x;
        pixelData_[i+2] = velocity_.y;
        pixelData_[i+3] = velocity_.z;
      }
    }
  }
}


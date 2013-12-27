/*
 * rectangle.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "tessellation.h"
#include "rectangle.h"
using namespace regen;

ref_ptr<Rectangle> Rectangle::getUnitQuad()
{
  static ref_ptr<Rectangle> mesh;
  if(mesh.get()==NULL) {
    Config cfg;
    cfg.centerAtOrigin = GL_FALSE;
    cfg.isNormalRequired = GL_FALSE;
    cfg.isTangentRequired = GL_FALSE;
    cfg.isTexcoRequired = GL_FALSE;
    cfg.levelOfDetail = 0;
    cfg.posScale = Vec3f(2.0f);
    cfg.rotation = Vec3f(0.5*M_PI, 0.0f, 0.0f);
    cfg.texcoScale = Vec2f(1.0);
    cfg.translation = Vec3f(-1.0f,-1.0f,0.0f);
    cfg.usage = VBO::USAGE_STATIC;
    mesh = ref_ptr<Rectangle>::alloc(cfg);
    return mesh;
  } else {
    return ref_ptr<Rectangle>::alloc(mesh);
  }
}

Rectangle::Rectangle(const Config &cfg)
: Mesh(GL_TRIANGLES,cfg.usage)
{
  pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
  nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
  texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
  tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
  updateAttributes(cfg);
}
Rectangle::Rectangle(const ref_ptr<Rectangle> &other)
: Mesh(other)
{
  pos_ = ref_ptr<ShaderInput3f>::upCast(
      inputContainer_->getInput(ATTRIBUTE_NAME_POS));
  nor_ = ref_ptr<ShaderInput3f>::upCast(
      inputContainer_->getInput(ATTRIBUTE_NAME_NOR));
  texco_ = ref_ptr<ShaderInput2f>::upCast(
      inputContainer_->getInput("texco0"));
  tan_ = ref_ptr<ShaderInput4f>::upCast(
      inputContainer_->getInput(ATTRIBUTE_NAME_TAN));
}

Rectangle::Config::Config()
: levelOfDetail(0),
  posScale(Vec3f(1.0f)),
  rotation(Vec3f(0.0f)),
  translation(Vec3f(0.0f)),
  texcoScale(Vec2f(1.0f)),
  isNormalRequired(GL_TRUE),
  isTexcoRequired(GL_TRUE),
  isTangentRequired(GL_FALSE),
  centerAtOrigin(GL_FALSE),
  usage(VBO::USAGE_DYNAMIC)
{
}

void Rectangle::updateAttributes(Config cfg)
{
  vector<TriangleFace> *faces; {
    Vec3f level0[4] = {
        Vec3f(0.0,0.0,0.0),
        Vec3f(1.0,0.0,0.0),
        Vec3f(1.0,0.0,1.0),
        Vec3f(0.0,0.0,1.0)
    };
    vector<TriangleFace> facesLevel0(2);
    facesLevel0[0] = TriangleFace( level0[0], level0[1], level0[3] );
    facesLevel0[1] = TriangleFace( level0[1], level0[2], level0[3] );
    faces = tessellate(cfg.levelOfDetail, facesLevel0);
  }
  GLuint numVertices = faces->size()*3;
  Mat4f rotMat = Mat4f::rotationMatrix(cfg.rotation.x, cfg.rotation.y, cfg.rotation.z);

  if(cfg.isTangentRequired) {
    cfg.isNormalRequired = GL_TRUE;
    cfg.isTexcoRequired = GL_TRUE;
  }
  Vec3f startPos;
  if(cfg.centerAtOrigin) {
    startPos = Vec3f(-cfg.posScale.x*0.5f, 0.0f, -cfg.posScale.z*0.5f);
  } else {
    startPos = Vec3f(0.0f, 0.0f, 0.0f);
  }

  // allocate attributes
  pos_->setVertexData(numVertices);
  if(cfg.isNormalRequired) {
    nor_->setVertexData(numVertices);
  }
  if(cfg.isTexcoRequired) {
    texco_->setVertexData(numVertices);
  }
  if(cfg.isTangentRequired) {
    tan_->setVertexData(numVertices);
  }

  for(GLuint faceIndex=0; faceIndex<faces->size(); ++faceIndex) {
    GLuint vertexIndex = faceIndex*3;
    TriangleFace &face = (*faces)[faceIndex];
    Vec3f *f = (Vec3f*)&face;

#define _TRANSFORM_(x) (rotMat.transformVector(cfg.posScale*x + startPos) + cfg.translation)
    for(GLuint i=0; i<3; ++i) pos_->setVertex(vertexIndex+i, _TRANSFORM_(f[i]));
#undef _TRANSFORM_

    if(cfg.isNormalRequired) {
      Vec3f normal = rotMat.transformVector(Vec3f(0.0,-1.0,0.0));
      for(GLuint i=0; i<3; ++i) nor_->setVertex(vertexIndex+i, normal);
    }

    if(cfg.isTexcoRequired) {
#define _TRANSFORM_(x) ( cfg.texcoScale - (cfg.texcoScale*x) )
      for(GLuint i=0; i<3; ++i) {
        texco_->setVertex(vertexIndex+i, _TRANSFORM_(Vec2f(f[i].x,f[i].z)));
      }
#undef _TRANSFORM_
    }

    if(cfg.isTangentRequired) {
      Vec3f *vertices = ((Vec3f*)pos_->clientDataPtr())+vertexIndex;
      Vec2f *texcos = ((Vec2f*)texco_->clientDataPtr())+vertexIndex;
      Vec3f *normals = ((Vec3f*)nor_->clientDataPtr())+vertexIndex;
      Vec4f tangent = calculateTangent(vertices, texcos, *normals);
      for(GLuint i=0; i<3; ++i) tan_->setVertex(vertexIndex+i, tangent);
    }
  }
  delete faces;

  begin(ShaderInputContainer::INTERLEAVED);
  setInput(pos_);
  if(cfg.isNormalRequired)
    setInput(nor_);
  if(cfg.isTexcoRequired)
    setInput(texco_);
  if(cfg.isTangentRequired)
    setInput(tan_);
  end();
}

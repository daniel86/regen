/*
 * cone.cpp
 *
 *  Created on: 03.02.2013
 *      Author: daniel
 */


#include "cone.h"
using namespace ogle;

ConeOpened::ConeOpened(const Config &cfg)
: MeshState(GL_TRIANGLE_FAN)
{
  pos_ = ref_ptr<ShaderInput>::manage(new PositionShaderInput);
  nor_ = ref_ptr<ShaderInput>::manage(new NormalShaderInput);
  updateAttributes(cfg);
}

ConeOpened::Config::Config()
: cosAngle(0.5),
  height(1.0f),
  isNormalRequired(GL_TRUE),
  levelOfDetail(1)
{
}

void ConeOpened::updateAttributes(const Config &cfg)
{
  GLfloat phi = acos( cfg.cosAngle );
  GLfloat radius = tan(phi)*cfg.height;

  GLint subdivisions = 4*pow(cfg.levelOfDetail,2);
  GLint numVertices = subdivisions+2;

  pos_->setVertexData(numVertices);
  if(cfg.isNormalRequired) {
    nor_->setVertexData(numVertices);
  }

  GLfloat angle = 0.0f;
  GLfloat angleStep = 2.0f*M_PI/(GLfloat)subdivisions;
  GLint i=0;

  pos_->setVertex3f(0, Vec3f(0.0f));
  if(cfg.isNormalRequired) {
    nor_->setVertex3f(0, Vec3f(0.0f,-1.0f,0.0f));
  }

  for(; i<subdivisions+1; ++i)
  {
      angle += angleStep;
      GLfloat s = sin(angle)*radius;
      GLfloat c = cos(angle)*radius;
      pos_->setVertex3f(i+1, Vec3f(c,s,cfg.height));
      if(cfg.isNormalRequired) {
        Vec3f n(c,0.0,s);
        n.normalize();
        nor_->setVertex3f(i+1, n);
      }
  }

  setInput(ref_ptr<ShaderInput>::cast(pos_));
  if(cfg.isNormalRequired) {
    setInput(ref_ptr<ShaderInput>::cast(nor_));
  }
}

static void loadConeData(
    Vec3f *pos, Vec3f *nor,
    GLboolean useBase,
    GLuint subdivisions,
    GLfloat radius,
    GLfloat height)
{
  GLfloat angle = 0.0f;
  GLfloat angleStep = 2.0f*M_PI/(GLfloat)subdivisions;
  GLint i=0;

  // apex
  pos[i] = Vec3f(0.0f);
  if(nor) nor[i] = Vec3f(0.0f,0.0f,-1.0f);
  ++i;
  // base center
  if(useBase) {
    pos[i] = Vec3f(0.0f,0.0f,height);
    if(nor) nor[i] = Vec3f(0.0f,0.0f,1.0f);
    ++i;
  }

  GLint numVertices = subdivisions+i;
  for(; i<numVertices; ++i)
  {
      angle += angleStep;
      GLfloat s = sin(angle)*radius;
      GLfloat c = cos(angle)*radius;
      pos[i] = Vec3f(c,s,height);
      if(nor) {
        nor[i] = Vec3f(c,s,0.0);
        nor[i].normalize();
      }
  }
}

/////////////////
/////////////////

ref_ptr<ConeClosed> ConeClosed::getBaseCone()
{
  static ref_ptr<ConeClosed> mesh;
  if(mesh.get()==NULL) {
    Config cfg;
    cfg.height = 1.0f;
    cfg.radius = 0.5;
    cfg.levelOfDetail = 3;
    cfg.isNormalRequired = GL_FALSE;
    cfg.isBaseRequired = GL_TRUE;
    mesh = ref_ptr<ConeClosed>::manage(new ConeClosed(cfg));
  }
  return mesh;
}

ConeClosed::Config::Config()
: radius(0.5),
  height(1.0f),
  isNormalRequired(GL_TRUE),
  isBaseRequired(GL_TRUE),
  levelOfDetail(1)
{
}

ConeClosed::ConeClosed(const Config &cfg)
: IndexedMeshState(GL_TRIANGLES)
{
  pos_ = ref_ptr<ShaderInput>::manage(new PositionShaderInput);
  nor_ = ref_ptr<ShaderInput>::manage(new NormalShaderInput);
  updateAttributes(cfg);
}

void ConeClosed::updateAttributes(const Config &cfg)
{
  GLuint subdivisions = 4*pow(cfg.levelOfDetail,2);
  GLuint numVertices = subdivisions+1;
  if(cfg.isBaseRequired) numVertices += 1;

  pos_->setVertexData(numVertices);
  if(cfg.isNormalRequired) {
    nor_->setVertexData(numVertices);
  }
  loadConeData(
      (Vec3f*) pos_->dataPtr(),
      (Vec3f*) nor_->dataPtr(),
      cfg.isBaseRequired, subdivisions,
      cfg.radius, cfg.height);
  setInput(ref_ptr<ShaderInput>::cast(pos_));
  if(cfg.isNormalRequired) {
    setInput(ref_ptr<ShaderInput>::cast(nor_));
  }

  GLuint apexIndex = 0;
  GLuint baseCenterIndex = 1;

  GLuint numFaces = subdivisions;
  if(cfg.isBaseRequired) { numFaces *= 2; }
  GLuint numIndices = numFaces*3;

  ref_ptr<VertexAttribute> indices = ref_ptr<VertexAttribute>::manage(
      new VertexAttribute("i", GL_UNSIGNED_INT, sizeof(GLuint), 1, 1, GL_FALSE));
  indices->setVertexData(numIndices);
  GLuint *faceIndices = (GLuint*)indices->dataPtr();
  GLuint faceIndex = 0;
  GLint vIndex = cfg.isBaseRequired ? 2 : 1;
  // cone
  for(GLuint i=0; i<subdivisions; ++i)
  {
    faceIndices[faceIndex] = apexIndex;
    ++faceIndex;

    faceIndices[faceIndex] = (i+1==subdivisions ? vIndex : vIndex+i+1);
    ++faceIndex;

    faceIndices[faceIndex] = vIndex+i;
    ++faceIndex;
  }
  // base
  if(cfg.isBaseRequired)
  {
    for(GLuint i=0; i<subdivisions; ++i)
    {
      faceIndices[faceIndex] = baseCenterIndex;
      ++faceIndex;

      faceIndices[faceIndex] = vIndex+i;
      ++faceIndex;

      faceIndices[faceIndex] = (i+1==subdivisions ? vIndex : vIndex+i+1);
      ++faceIndex;
    }
  }
  setIndices(indices, numVertices);
}

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
    TriangleVertex level0[4];
    level0[0] = TriangleVertex(Vec3f(0.0,0.0,0.0),0);
    level0[1] = TriangleVertex(Vec3f(1.0,0.0,0.0),1);
    level0[2] = TriangleVertex(Vec3f(1.0,0.0,1.0),2);
    level0[3] = TriangleVertex(Vec3f(0.0,0.0,1.0),3);

    vector<TriangleFace> facesLevel0(2);
    facesLevel0[0] = TriangleFace( level0[0], level0[1], level0[3] );
    facesLevel0[1] = TriangleFace( level0[1], level0[2], level0[3] );
    faces = tessellate(cfg.levelOfDetail, facesLevel0);
  }
  Mat4f rotMat = Mat4f::rotationMatrix(cfg.rotation.x, cfg.rotation.y, cfg.rotation.z);
  GLboolean useIndexBuffer = (cfg.levelOfDetail>1);

  GLuint numVertices;
  map<GLuint,GLuint> indexMap;
  set<GLuint> processedIndices;

  ref_ptr<ShaderInput1ui> indices;
  if(useIndexBuffer) {
    // Allocate RAM for indices
    GLuint numIndices = faces->size()*3;
    indices = ref_ptr<ShaderInput1ui>::alloc("i");
    indices->setVertexData(numIndices);
    GLuint *indicesPtr = (GLuint*)indices->clientDataPtr();

    // Set index data and compute vertex count
    GLuint currIndex = 0;
    for(GLuint faceIndex=0; faceIndex<faces->size(); ++faceIndex) {
      TriangleFace &face = (*faces)[faceIndex];
      TriangleVertex *vertices = (TriangleVertex*)&face;

      for(GLuint i=0; i<3; ++i) {
        TriangleVertex &vertex = vertices[i];
        // Find vertex index
        if(indexMap.count(vertex.i)==0) {
          indexMap[vertex.i] = currIndex;
          currIndex += 1;
        }
        // Add to indices attribute
        *indicesPtr = indexMap[vertex.i];
        indicesPtr += 1;
      }
    }
    numVertices = indexMap.size();
  }
  else {
    numVertices = faces->size()*3;
  }

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

  GLuint triIndices[3];
  Vec3f triVertices[3];
  Vec2f triTexco[3];
  Vec3f normal = rotMat.transformVector(Vec3f(0.0,-1.0,0.0));

  for(GLuint faceIndex=0; faceIndex<faces->size(); ++faceIndex) {
    TriangleFace &face = (*faces)[faceIndex];
    TriangleVertex *vertices = (TriangleVertex*)&face;

    for(GLuint i=0; i<3; ++i) {
      GLuint vertexIndex;
      if(useIndexBuffer) {
        vertexIndex = indexMap[vertices[i].i];
      }
      else {
        vertexIndex = faceIndex*3+i;
      }
      triIndices[i] = vertexIndex;

      if(processedIndices.count(vertexIndex)>0) {
        if(cfg.isTangentRequired) {
          triVertices[i] = pos_->getVertex(vertexIndex);
          triTexco[i]    = texco_->getVertex(vertexIndex);
        }
        continue;
      }
      processedIndices.insert(vertexIndex);

      pos_->setVertex(vertexIndex, rotMat.transformVector(
          cfg.posScale*vertices[i].p + startPos) + cfg.translation);
      if(cfg.isNormalRequired) {
        nor_->setVertex(vertexIndex, normal);
      }
      if(cfg.isTexcoRequired) {
        texco_->setVertex(vertexIndex, cfg.texcoScale -
            (cfg.texcoScale*Vec2f(vertices[i].p.x,vertices[i].p.z)));
      }
      if(cfg.isTangentRequired) {
        triVertices[i] = pos_->getVertex(vertexIndex);
        triTexco[i] = texco_->getVertex(vertexIndex);
      }
    }

    if(cfg.isTangentRequired) {
      Vec4f tangent = calculateTangent(triVertices, triTexco, normal);
      for(GLuint i=0; i<3; ++i) {
        tan_->setVertex(triIndices[i], tangent);
      }
    }
  }
  delete faces;

  begin(ShaderInputContainer::INTERLEAVED);
  if(useIndexBuffer) setIndices(indices, numVertices);
  setInput(pos_);
  if(cfg.isNormalRequired)
    setInput(nor_);
  if(cfg.isTexcoRequired)
    setInput(texco_);
  if(cfg.isTangentRequired)
    setInput(tan_);
  end();

  minPosition_ = -cfg.posScale;
  maxPosition_ = cfg.posScale;
}

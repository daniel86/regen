/*
 * box.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "tessellation.h"
#include "box.h"
using namespace regen;

namespace regen {
  ostream& operator<<(ostream &out, const Box::TexcoMode &mode)
  {
    switch(mode) {
    case Box::TEXCO_MODE_NONE:     return out << "NONE";
    case Box::TEXCO_MODE_UV:       return out << "UV";
    case Box::TEXCO_MODE_CUBE_MAP: return out << "CUBE_MAP";
    }
    return out;
  }
  istream& operator>>(istream &in, Box::TexcoMode &mode)
  {
    string val;
    in >> val;
    boost::to_upper(val);
    if(val == "NONE")          mode = Box::TEXCO_MODE_NONE;
    else if(val == "UV")       mode = Box::TEXCO_MODE_UV;
    else if(val == "CUBE_MAP") mode = Box::TEXCO_MODE_CUBE_MAP;
    else {
      REGEN_WARN("Unknown box texco mode '" << val << "'. Using NONE texco.");
      mode = Box::TEXCO_MODE_NONE;
    }
    return in;
  }
}

ref_ptr<Box> Box::getUnitCube()
{
  static ref_ptr<Box> mesh;
  if(mesh.get()==NULL) {
    Config cfg;
    cfg.posScale = Vec3f(1.0f);
    cfg.rotation = Vec3f(0.0, 0.0f, 0.0f);
    cfg.texcoMode = TEXCO_MODE_NONE;
    cfg.isNormalRequired = GL_FALSE;
    cfg.isTangentRequired = GL_FALSE;
    cfg.usage = VBO::USAGE_STATIC;
    mesh = ref_ptr<Box>::alloc(cfg);
    return mesh;
  } else {
    return ref_ptr<Box>::alloc(mesh);
  }
}

Box::Box(const Config &cfg)
: Mesh(GL_TRIANGLES, cfg.usage)
{
  pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
  nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
  tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
  updateAttributes(cfg);
}
Box::Box(const ref_ptr<Box> &other)
: Mesh(other)
{
  pos_ = ref_ptr<ShaderInput3f>::upCast(
      inputContainer_->getInput(ATTRIBUTE_NAME_POS));
  nor_ = ref_ptr<ShaderInput3f>::upCast(
      inputContainer_->getInput(ATTRIBUTE_NAME_NOR));
  tan_ = ref_ptr<ShaderInput4f>::upCast(
      inputContainer_->getInput(ATTRIBUTE_NAME_TAN));
}

Box::Config::Config()
: levelOfDetail(0),
  posScale(Vec3f(1.0f)),
  rotation(Vec3f(0.0f)),
  texcoScale(Vec2f(1.0f)),
  texcoMode(TEXCO_MODE_UV),
  isNormalRequired(GL_TRUE),
  isTangentRequired(GL_FALSE),
  usage(VBO::USAGE_DYNAMIC)
{
}

void Box::updateAttributes(const Config &cfg)
{
  static const Vec3f cubeNormals[] = {
      Vec3f( 0.0f, 0.0f, 1.0f), // Front
      Vec3f( 0.0f, 0.0f,-1.0f), // Back
      Vec3f( 0.0f, 1.0f, 0.0f), // Top
      Vec3f( 0.0f,-1.0f, 0.0f), // Bottom
      Vec3f( 1.0f, 0.0f, 0.0f), // Right
      Vec3f(-1.0f, 0.0f, 0.0f)  // Left
  };
  static const TriangleVertex cubeVertices[] = {
      // Front
      TriangleVertex(Vec3f(-1.0,-1.0, 1.0),0),
      TriangleVertex(Vec3f( 1.0,-1.0, 1.0),1),
      TriangleVertex(Vec3f( 1.0, 1.0, 1.0),2),
      TriangleVertex(Vec3f(-1.0, 1.0, 1.0),3),
      // Back
      TriangleVertex(Vec3f(-1.0,-1.0,-1.0),0),
      TriangleVertex(Vec3f(-1.0, 1.0,-1.0),1),
      TriangleVertex(Vec3f( 1.0, 1.0,-1.0),2),
      TriangleVertex(Vec3f( 1.0,-1.0,-1.0),3),
      // Top
      TriangleVertex(Vec3f(-1.0, 1.0,-1.0),0),
      TriangleVertex(Vec3f(-1.0, 1.0, 1.0),1),
      TriangleVertex(Vec3f( 1.0, 1.0, 1.0),2),
      TriangleVertex(Vec3f( 1.0, 1.0,-1.0),3),
      // Bottom
      TriangleVertex(Vec3f(-1.0,-1.0,-1.0),0),
      TriangleVertex(Vec3f( 1.0,-1.0,-1.0),1),
      TriangleVertex(Vec3f( 1.0,-1.0, 1.0),2),
      TriangleVertex(Vec3f(-1.0,-1.0, 1.0),3),
      // Right
      TriangleVertex(Vec3f( 1.0,-1.0,-1.0),0),
      TriangleVertex(Vec3f( 1.0, 1.0,-1.0),1),
      TriangleVertex(Vec3f( 1.0, 1.0, 1.0),2),
      TriangleVertex(Vec3f( 1.0,-1.0, 1.0),3),
      // Left
      TriangleVertex(Vec3f(-1.0,-1.0,-1.0),0),
      TriangleVertex(Vec3f(-1.0,-1.0, 1.0),1),
      TriangleVertex(Vec3f(-1.0, 1.0, 1.0),2),
      TriangleVertex(Vec3f(-1.0, 1.0,-1.0),3)
  };
  static const GLfloat cubeTexcoords[] = {
      1.0, 1.0,  0.0, 1.0,  0.0, 0.0,  1.0, 0.0, // Front
      0.0, 1.0,  0.0, 0.0,  1.0, 0.0,  1.0, 1.0, // Back
      1.0, 0.0,  1.0, 1.0,  0.0, 1.0,  0.0, 0.0, // Top
      0.0, 0.0,  1.0, 0.0,  1.0, 1.0,  0.0, 1.0, // Bottom
      0.0, 1.0,  0.0, 0.0,  1.0, 0.0,  1.0, 1.0, // Right
      1.0, 1.0,  0.0, 1.0,  0.0, 0.0,  1.0, 0.0  // Left
  };

  GLuint vertexBase = 0;
  GLuint numVertices = pow(4.0,cfg.levelOfDetail)*6*2*3;
  Mat4f rotMat = Mat4f::rotationMatrix(
      cfg.rotation.x, cfg.rotation.y, cfg.rotation.z);

  // allocate attributes
  pos_->setVertexData(numVertices);
  if(cfg.isNormalRequired) {
    nor_->setVertexData(numVertices);
  }
  if(cfg.isTangentRequired) {
    tan_->setVertexData(numVertices);
  }

  TexcoMode texcoMode = cfg.texcoMode;
  if(cfg.isTangentRequired && cfg.texcoMode==TEXCO_MODE_NONE) {
    texcoMode = TEXCO_MODE_UV;
  }
  if(texcoMode == TEXCO_MODE_CUBE_MAP) {
    texco_ = ref_ptr<ShaderInput3f>::alloc("texco0");
    texco_->setVertexData(numVertices);
  }
  else if(texcoMode == TEXCO_MODE_UV) {
    texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
    texco_->setVertexData(numVertices);
  }

  for(GLuint sideIndex=0; sideIndex<6; ++sideIndex) {
    const Vec3f &normal = cubeNormals[sideIndex];
    TriangleVertex *level0 = (TriangleVertex*)cubeVertices;
    Vec2f *uv0 = (Vec2f*)cubeTexcoords;
    level0 += sideIndex*4;
    uv0 += sideIndex*4;

    // Tessellate cube face
    vector<TriangleFace> *faces; {
      vector<TriangleFace> facesLevel0(2);
      facesLevel0[0] = TriangleFace( level0[0], level0[1], level0[3] );
      facesLevel0[1] = TriangleFace( level0[1], level0[2], level0[3] );
      faces = tessellate(cfg.levelOfDetail, facesLevel0);
    }

    for(GLuint faceIndex=0; faceIndex<faces->size(); ++faceIndex) {
      GLuint vertexIndex = faceIndex*3 + vertexBase;
      TriangleFace &face = (*faces)[faceIndex];
      TriangleVertex *f = (TriangleVertex*)&face;

      for(GLuint i=0; i<3; ++i) {
        pos_->setVertex(vertexIndex+i, cfg.posScale * rotMat.transformVector(f[i].p));
      }
      if(cfg.isNormalRequired) {
        for(GLuint i=0; i<3; ++i) nor_->setVertex(vertexIndex+i, normal);
      }

      if(texcoMode == TEXCO_MODE_CUBE_MAP) {
        Vec3f *texco = (Vec3f*) texco_->clientData();
        for(GLuint i=0; i<3; ++i) {
          Vec3f v = f[i].p;
          v.normalize();
          texco[vertexIndex+i] = v;
        }
      }
      else if(texcoMode == TEXCO_MODE_UV) {
        Vec2f *texco = (Vec2f*) texco_->clientData();
        for(GLuint i=0; i<3; ++i) {
          // linear interpolate texture coordinates
          const Vec3f &p = pos_->getVertex(vertexIndex+i);
          GLfloat d0 = (p-level0[0].p).length();
          GLfloat d1 = (p-level0[1].p).length();
          GLfloat d2 = (p-level0[2].p).length();
          GLfloat d3 = (p-level0[3].p).length();
          GLfloat f0 = d1/(d0+d1);
          GLfloat f1 = d0/(d0+d1);
          GLfloat f2 = d2/(d2+d3);
          GLfloat f3 = d3/(d2+d3);
          Vec3f p01 = level0[0].p*f0 + level0[1].p*f1;
          Vec3f p23 = level0[2].p*f2 + level0[3].p*f3;
          Vec2f uv01 = uv0[0]*f0 + uv0[1]*f1;
          Vec2f uv23 = uv0[2]*f2 + uv0[3]*f3;
          GLfloat d01 = (p-p01).length();
          GLfloat d23 = (p-p23).length();
          GLfloat f01 = d23/(d01+d23);
          GLfloat f23 = d01/(d01+d23);
          texco[vertexIndex+i] = uv01*f01 + uv23*f23;
        }
      }

      if(cfg.isTangentRequired) {
        Vec3f *vertices = ((Vec3f*)pos_->clientDataPtr())+vertexIndex;
        Vec2f *texcos = ((Vec2f*)texco_->clientDataPtr())+vertexIndex;
        Vec4f tangent = calculateTangent(vertices, texcos, normal);
        for(GLuint i=0; i<3; ++i) tan_->setVertex(vertexIndex+i, tangent);
      }
    }
    vertexBase += faces->size()*3;
  }

  begin(ShaderInputContainer::INTERLEAVED);
  setInput(pos_);
  if(cfg.isNormalRequired)
    setInput(nor_);
  if(cfg.texcoMode!=TEXCO_MODE_NONE)
    setInput(texco_);
  if(cfg.isTangentRequired)
    setInput(tan_);
  end();
}

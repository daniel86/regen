/*
 * box.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

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
{ updateAttributes(cfg); }
Box::Box(const ref_ptr<Box> &other)
: Mesh(other)
{}

Box::Config::Config()
: posScale(Vec3f(1.0f)),
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
  const GLfloat vertices[] = {
      -1.0,-1.0, 1.0,   1.0,-1.0, 1.0,   1.0, 1.0, 1.0,  -1.0, 1.0, 1.0, // Front
      -1.0,-1.0,-1.0,  -1.0, 1.0,-1.0,   1.0, 1.0,-1.0,   1.0,-1.0,-1.0, // Back
      -1.0, 1.0,-1.0,  -1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0,-1.0, // Top
      -1.0,-1.0,-1.0,   1.0,-1.0,-1.0,   1.0,-1.0, 1.0,  -1.0,-1.0, 1.0, // Bottom
       1.0,-1.0,-1.0,   1.0, 1.0,-1.0,   1.0, 1.0, 1.0,   1.0,-1.0, 1.0, // Right
      -1.0,-1.0,-1.0,  -1.0,-1.0, 1.0,  -1.0, 1.0, 1.0,  -1.0, 1.0,-1.0  // Left
  };
  const GLfloat texcoords[] = {
      1.0, 1.0,  0.0, 1.0,  0.0, 0.0,  1.0, 0.0, // Front
      0.0, 1.0,  0.0, 0.0,  1.0, 0.0,  1.0, 1.0, // Back
      1.0, 0.0,  1.0, 1.0,  0.0, 1.0,  0.0, 0.0, // Top
      0.0, 0.0,  1.0, 0.0,  1.0, 1.0,  0.0, 1.0, // Bottom
      0.0, 1.0,  0.0, 0.0,  1.0, 0.0,  1.0, 1.0, // Right
      1.0, 1.0,  0.0, 1.0,  0.0, 0.0,  1.0, 0.0  // Left
  };
  const GLfloat normals[] = {
      0.0f, 0.0f, 1.0f, // Front
      0.0f, 0.0f,-1.0f, // Back
      0.0f, 1.0f, 0.0f, // Top
      0.0f,-1.0f, 0.0f, // Bottom
      1.0f, 0.0f, 0.0f, // Right
     -1.0f, 0.0f, 0.0f  // Left
  };

  Mat4f rotMat = Mat4f::rotationMatrix(
      cfg.rotation.x, cfg.rotation.y, cfg.rotation.z);

  ref_ptr<ShaderInput1ui> indices = ref_ptr<ShaderInput1ui>::alloc("i");
  indices->setVertexData(6*6);
  GLuint *faceIndices = (GLuint*) indices->clientDataPtr();
  GLuint index = 0;
  for(GLuint i=0; i<6; ++i)
  {
    faceIndices[index] = i*4 + 0; ++index;
    faceIndices[index] = i*4 + 1; ++index;
    faceIndices[index] = i*4 + 2; ++index;
    faceIndices[index] = i*4 + 0; ++index;
    faceIndices[index] = i*4 + 2; ++index;
    faceIndices[index] = i*4 + 3; ++index;
  }

  ref_ptr<ShaderInput3f> pos =
      ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
  pos->setVertexData(24);
  for(GLuint i=0; i<24; ++i)
  {
    Vec3f &v = ((Vec3f*)vertices)[i];
    pos->setVertex(i, cfg.posScale * rotMat.transform(v) );
  }

  ref_ptr<ShaderInput3f> nor;
  if(cfg.isNormalRequired) {
    nor = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
    nor->setVertexData(24);

    GLint index = 0;
    Vec3f *n = (Vec3f*)normals;
    for(GLuint i=0; i<6; ++i)
    {
      for(GLuint j=0; j<4; ++j)
      {
        nor->setVertex(index, *n);
        ++index;
      }
      n += 1;
    }
  }

  ref_ptr<ShaderInput> texco;
  TexcoMode texcoMode = cfg.texcoMode;
  if(cfg.isTangentRequired && cfg.texcoMode==TEXCO_MODE_NONE) {
    texcoMode = TEXCO_MODE_UV;
  }
  switch(texcoMode) {
  case TEXCO_MODE_NONE:
    break;
  case TEXCO_MODE_CUBE_MAP: {
    Vec3f* vertices = (Vec3f*)pos->clientDataPtr();
    ref_ptr<ShaderInput3f> texco_ = ref_ptr<ShaderInput3f>::alloc("texco0");
    texco_->setVertexData(24);
    for(GLuint i=0; i<24; ++i)
    {
      Vec3f v = vertices[i];
      v.normalize();
      texco_->setVertex(i, v);
    }
    texco = texco_;
    break;
  }
  case TEXCO_MODE_UV: {
    ref_ptr<ShaderInput2f> texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
    texco_->setVertexData(24);
    for(GLuint i=0; i<24; ++i)
    {
      Vec2f &uv = ((Vec2f*)texcoords)[i];
      texco_->setVertex(i, cfg.texcoScale*uv );
    }
    texco = texco_;
    break;
  }}

  ref_ptr<ShaderInput4f> tan;
  if(cfg.isTangentRequired) {
    tan = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
    tan->setVertexData(24);

    // calculate tangent for each face
    Vec3f *v = (Vec3f*)vertices;
    Vec3f *n = (Vec3f*)normals;
    Vec2f *uv = (Vec2f*)texcoords;
    GLint index = 0;
    for(GLuint i=0; i<6; ++i)
    {
      Vec4f t = calculateTangent(v,uv,*n);
      for(GLuint j=0; j<4; ++j)
      {
        tan->setVertex(index, t);
        ++index;
      }
      n += 1; v += 4; uv += 4;
    }
  }

  begin(ShaderInputContainer::INTERLEAVED);
  setIndices(indices, 23);
  setInput(pos);
  if(cfg.isNormalRequired)
    setInput(nor);
  if(cfg.texcoMode!=TEXCO_MODE_NONE)
    setInput(texco);
  if(cfg.isTangentRequired)
    setInput(tan);
  end();
}

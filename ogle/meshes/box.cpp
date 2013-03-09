/*
 * box.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "box.h"
using namespace ogle;

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
    mesh = ref_ptr<Box>::manage(new Box(cfg));
  }
  return mesh;
}

Box::Box(const Config &cfg)
: IndexedMeshState(GL_TRIANGLES)
{
  updateAttributes(cfg);
}

Box::Config::Config()
: posScale(Vec3f(1.0f)),
  rotation(Vec3f(0.0f)),
  texcoScale(Vec2f(1.0f)),
  texcoMode(TEXCO_MODE_UV),
  isNormalRequired(GL_TRUE),
  isTangentRequired(GL_FALSE)
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

  ref_ptr<VertexAttribute> indices = ref_ptr<VertexAttribute>::manage(
      new VertexAttribute("i", GL_UNSIGNED_INT, sizeof(GLuint), 1, 1, GL_FALSE));
  indices->setVertexData(6*6);
  GLuint *faceIndices = (GLuint*) indices->dataPtr();
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
  setIndices(indices, 23);

  ref_ptr<ShaderInput3f> pos =
      ref_ptr<ShaderInput3f>::manage(new ShaderInput3f(ATTRIBUTE_NAME_POS));
  pos->setVertexData(24);
  for(GLuint i=0; i<24; ++i)
  {
    Vec3f &v = ((Vec3f*)vertices)[i];
    pos->setVertex3f(i, cfg.posScale * rotMat.transform(v) );
  }
  setInput(ref_ptr<ShaderInput>::cast(pos));

  if(cfg.isNormalRequired) {
    ref_ptr<ShaderInput3f> nor = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f(ATTRIBUTE_NAME_NOR));
    nor->setVertexData(24);

    GLint index = 0;
    Vec3f *n = (Vec3f*)normals;
    for(GLuint i=0; i<6; ++i)
    {
      for(GLuint j=0; j<4; ++j)
      {
        nor->setVertex3f(index, *n);
        ++index;
      }
      n += 1;
    }
    setInput(ref_ptr<ShaderInput>::cast(nor));
  }

  TexcoMode texcoMode = cfg.texcoMode;
  if(cfg.isTangentRequired && cfg.texcoMode==TEXCO_MODE_NONE) {
    texcoMode = TEXCO_MODE_UV;
  }
  switch(texcoMode) {
  case TEXCO_MODE_NONE:
    break;
  case TEXCO_MODE_CUBE_MAP: {
    Vec3f* vertices = (Vec3f*)pos->dataPtr();
    ref_ptr<ShaderInput3f> texco =
        ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("texco0"));
    texco->setVertexData(24);
    for(GLuint i=0; i<24; ++i)
    {
      Vec3f v = vertices[i];
      v.normalize();
      texco->setVertex3f(i, v);
    }
    setInput(ref_ptr<ShaderInput>::cast(texco));
    break;
  }
  case TEXCO_MODE_UV: {
    ref_ptr<ShaderInput2f> texco =
        ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("texco0"));
    texco->setVertexData(24);
    for(GLuint i=0; i<24; ++i)
    {
      Vec2f &uv = ((Vec2f*)texcoords)[i];
      texco->setVertex2f(i, cfg.texcoScale*uv );
    }
    setInput(ref_ptr<ShaderInput>::cast(texco));
    break;
  }}

  if(cfg.isTangentRequired) {
    ref_ptr<ShaderInput4f> tan = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f(ATTRIBUTE_NAME_TAN));
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
        tan->setVertex4f(index, t);
        ++index;
      }
      n += 1; v += 4; uv += 4;
    }

    setInput(ref_ptr<ShaderInput>::cast(tan));
  }
}


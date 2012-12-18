/*
 * Cube.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "cube.h"

UnitCube::UnitCube(const Config &cfg)
: IndexedMeshState(GL_TRIANGLES)
{
  updateAttributes(cfg);
}

UnitCube::Config::Config()
: posScale(Vec3f(1.0f)),
  rotation(Vec3f(0.0f)),
  texcoScale(Vec2f(1.0f)),
  texcoMode(TEXCO_MODE_UV),
  isNormalRequired(GL_TRUE),
  isTangentRequired(GL_FALSE)
{
}

void UnitCube::updateAttributes(const Config &cfg)
{
  Mat4f rotMat = xyzRotationMatrix(
      cfg.rotation.x, cfg.rotation.y, cfg.rotation.z);

  GLuint *faceIndices = new GLuint[6*6];
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
  setFaceIndicesui(faceIndices, 6, 6);
  delete[] faceIndices;

  const GLfloat vertices[] = {
      -1.0,-1.0, 1.0,   1.0,-1.0, 1.0,   1.0, 1.0, 1.0,  -1.0, 1.0, 1.0, // Front
      -1.0,-1.0,-1.0,  -1.0, 1.0,-1.0,   1.0, 1.0,-1.0,   1.0,-1.0,-1.0, // Back
      -1.0, 1.0,-1.0,  -1.0, 1.0, 1.0,   1.0, 1.0, 1.0,   1.0, 1.0,-1.0, // Top
      -1.0,-1.0,-1.0,   1.0,-1.0,-1.0,   1.0,-1.0, 1.0,  -1.0,-1.0, 1.0, // Bottom
       1.0,-1.0,-1.0,   1.0, 1.0,-1.0,   1.0, 1.0, 1.0,   1.0,-1.0, 1.0, // Right
      -1.0,-1.0,-1.0,  -1.0,-1.0, 1.0,  -1.0, 1.0, 1.0,  -1.0, 1.0,-1.0  // Left
  };
  ref_ptr<PositionShaderInput> pos =
      ref_ptr<PositionShaderInput>::manage(new PositionShaderInput);
  pos->setVertexData(24);
  for(GLuint i=0; i<24; ++i)
  {
    Vec3f &v = ((Vec3f*)vertices)[i];
    pos->setVertex3f(i, cfg.posScale * transformVec3(rotMat,v) );
  }
  setInput(ref_ptr<ShaderInput>::cast(pos));

  if(cfg.isNormalRequired) {
    const GLfloat normals[] = {
        0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f, // Front
        0.0f, 0.0f,-1.0f,   0.0f, 0.0f,-1.0f,   0.0f, 0.0f,-1.0f,   0.0f, 0.0f,-1.0f, // Back
        0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f, // Top
        0.0f,-1.0f, 0.0f,   0.0f,-1.0f, 0.0f,   0.0f,-1.0f, 0.0f,   0.0f,-1.0f, 0.0f, // Bottom
        1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f, // Right
       -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f  // Left
    };

    ref_ptr<NormalShaderInput> nor = ref_ptr<NormalShaderInput>::manage(new NormalShaderInput);
    nor->setVertexData(24);
    for(GLuint i=0; i<24; ++i)
    {
      Vec3f &n = ((Vec3f*)normals)[i];
      nor->setVertex3f(i, transformVec3(rotMat,n) );
    }
    setInput(ref_ptr<ShaderInput>::cast(nor));
  }

  if(cfg.isTangentRequired) {
    const GLfloat tangents[] = {
        -1.0f, 0.0f, 0.0f,-1.0f,  -1.0f, 0.0f, 0.0f,-1.0f,  -1.0f, 0.0f, 0.0f,-1.0f,  -1.0f, 0.0f, 0.0f,-1.0f, // Front
        -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f, 1.0f, // Back
        -1.0f, 0.0f, 0.0f,-1.0f,  -1.0f, 0.0f, 0.0f,-1.0f,  -1.0f, 0.0f, 0.0f,-1.0f,  -1.0f, 0.0f, 0.0f,-1.0f, // Top
        -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f, 1.0f, // Bottom
         0.0f,-1.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f, 1.0f, // Left
         0.0f,-1.0f, 0.0f,-1.0f,   0.0f,-1.0f, 0.0f,-1.0f,   0.0f,-1.0f, 0.0f,-1.0f,   0.0f,-1.0f, 0.0f,-1.0f  // Right
    };

    ref_ptr<TangentShaderInput> tan = ref_ptr<TangentShaderInput>::manage(new TangentShaderInput);
    tan->setVertexData(24);
    for(GLuint i=0; i<24; ++i)
    {
      Vec4f &t = ((Vec4f*)tangents)[i];
      Vec3f t_(t.x,t.y,t.z);
      t_ = transformVec3(rotMat,t_);
      tan->setVertex4f(i, Vec4f(t_.x,t_.y,t_.z,t.w) );
    }
    setInput(ref_ptr<ShaderInput>::cast(tan));
  }

  switch(cfg.texcoMode) {
  case TEXCO_MODE_NONE:
    break;
  case TEXCO_MODE_CUBE_MAP: {
    Vec3f* vertices = (Vec3f*)pos->dataPtr();
    ref_ptr<TexcoShaderInput> texco = ref_ptr<TexcoShaderInput>::manage(new TexcoShaderInput( 0, 3 ));
    texco->setVertexData(24);
    for(GLuint i=0; i<24; ++i)
    {
      Vec3f v = vertices[i];
      normalize(v);
      texco->setVertex3f(i, v);
    }
    setInput(ref_ptr<ShaderInput>::cast(texco));
    break;
  }
  case TEXCO_MODE_UV: {
    const GLfloat texcoords[] = {
        1.0, 1.0,  0.0, 1.0,  0.0, 0.0,  1.0, 0.0, // Front
        0.0, 1.0,  0.0, 0.0,  1.0, 0.0,  1.0, 1.0, // Back
        1.0, 0.0,  1.0, 1.0,  0.0, 1.0,  0.0, 0.0, // Top
        0.0, 0.0,  1.0, 0.0,  1.0, 1.0,  0.0, 1.0, // Bottom
        0.0, 1.0,  0.0, 0.0,  1.0, 0.0,  1.0, 1.0, // Right
        1.0, 1.0,  0.0, 1.0,  0.0, 0.0,  1.0, 0.0  // Left
    };
    ref_ptr<TexcoShaderInput> texco =
        ref_ptr<TexcoShaderInput>::manage(new TexcoShaderInput( 0, 2 ));
    texco->setVertexData(24);
    for(GLuint i=0; i<24; ++i)
    {
      Vec2f &uv = ((Vec2f*)texcoords)[i];
      texco->setVertex2f(i, cfg.texcoScale*uv );
    }
    setInput(ref_ptr<ShaderInput>::cast(texco));
    break;
  }}
}


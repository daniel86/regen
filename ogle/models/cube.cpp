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

Vec4f calculateTangent(Vec3f *vertices, Vec2f *texco, Vec3f &normal)
{
  Vec3f tangent, binormal;
  // calculate vertex and uv edges
  Vec3f edge1 = vertices[1] - vertices[0]; normalize(edge1);
  Vec3f edge2 = vertices[2] - vertices[0]; normalize(edge2);
  Vec2f texEdge1 = texco[1] - texco[0]; normalize(texEdge1);
  Vec2f texEdge2 = texco[2] - texco[0]; normalize(texEdge2);
  GLfloat det = texEdge1.x * texEdge2.y - texEdge2.x * texEdge1.y;

  if(isApprox(det,0.0)) {
    tangent  = Vec3f( 1.0, 0.0, 0.0 );
    binormal  = Vec3f( 0.0, 1.0, 0.0 );
  }
  else {
    det = 1.0 / det;
    tangent = Vec3f(
      (texEdge2.y * edge1.x - texEdge1.y * edge2.x),
      (texEdge2.y * edge1.y - texEdge1.y * edge2.y),
      (texEdge2.y * edge1.z - texEdge1.y * edge2.z)
    ) * det;
    binormal = Vec3f(
      (-texEdge2.x * edge1.x + texEdge1.x * edge2.x),
      (-texEdge2.x * edge1.y + texEdge1.x * edge2.y),
      (-texEdge2.x * edge1.z + texEdge1.x * edge2.z)
    ) * det;
  }

  // Gram-Schmidt orthogonalize tangent with normal.
  tangent -= normal * dot(normal, tangent);
  normalize(tangent);

  Vec3f bitangent = cross(normal, tangent);
  // Calculate the handedness of the local tangent space.
  float handedness = (dot(bitangent, binormal) < 0.0f) ? 1.0f : -1.0f;

  return Vec4f(tangent.x, tangent.y, tangent.z, handedness);
}

void UnitCube::updateAttributes(const Config &cfg)
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
    ref_ptr<NormalShaderInput> nor = ref_ptr<NormalShaderInput>::manage(new NormalShaderInput);
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
    ref_ptr<TexcoShaderInput> texco = ref_ptr<TexcoShaderInput>::manage(new TexcoShaderInput( 0, 2 ));
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
    ref_ptr<TangentShaderInput> tan = ref_ptr<TangentShaderInput>::manage(new TangentShaderInput);
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


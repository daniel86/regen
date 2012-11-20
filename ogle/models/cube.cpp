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
  isNormalRequired(true)
{
}

void UnitCube::updateAttributes(const Config &cfg)
{
  const GLuint numCubeSides = 6;
  const GLuint numCubeFaces = 2*numCubeSides;
  const GLuint numCubeFaceIndices = 3;
  const GLuint numCubeVertices = numCubeSides*4;

  GLuint *faceIndices = new GLuint[numCubeFaces*numCubeFaceIndices];
  GLuint index = 0;
  for(GLuint i=0; i<numCubeSides; i+=1)
  {
    faceIndices[index++] = i*4 + 0;
    faceIndices[index++] = i*4 + 1;
    faceIndices[index++] = i*4 + 2;
    faceIndices[index++] = i*4 + 0;
    faceIndices[index++] = i*4 + 2;
    faceIndices[index++] = i*4 + 3;
  }
  setFaceIndicesui(faceIndices, numCubeFaceIndices, numCubeFaces);
  delete[] faceIndices;

  // generate 'pos' attribute
  ref_ptr<PositionShaderInput> pos =
      ref_ptr<PositionShaderInput>::manage(new PositionShaderInput);
  Mat4f rotMat = xyzRotationMatrix(
      cfg.rotation.x, cfg.rotation.y, cfg.rotation.z);
  GLfloat x2=0.5f, y2=0.5f, z2=0.5f;

  pos->setVertexData(numCubeVertices);

#define TRANSFORM(x) (cfg.posScale * transformVec3(rotMat,x))
  // front
  pos->setVertex3f( 0, TRANSFORM(Vec3f(-x2, -y2,  z2)) );
  pos->setVertex3f( 1, TRANSFORM(Vec3f( x2, -y2,  z2)) );
  pos->setVertex3f( 2, TRANSFORM(Vec3f( x2,  y2,  z2)) );
  pos->setVertex3f( 3, TRANSFORM(Vec3f(-x2,  y2,  z2)) );
  // back
  pos->setVertex3f( 4, TRANSFORM(Vec3f(-x2, -y2, -z2)) );
  pos->setVertex3f( 5, TRANSFORM(Vec3f(-x2,  y2, -z2)) );
  pos->setVertex3f( 6, TRANSFORM(Vec3f( x2,  y2, -z2)) );
  pos->setVertex3f( 7, TRANSFORM(Vec3f( x2, -y2, -z2)) );
  // top
  pos->setVertex3f( 8, TRANSFORM(Vec3f(-x2,  y2, -z2)) );
  pos->setVertex3f( 9, TRANSFORM(Vec3f(-x2,  y2,  z2)) );
  pos->setVertex3f(10, TRANSFORM(Vec3f( x2,  y2,  z2)) );
  pos->setVertex3f(11, TRANSFORM(Vec3f( x2,  y2, -z2)) );
  // bottom
  pos->setVertex3f(12, TRANSFORM(Vec3f(-x2, -y2, -z2)) );
  pos->setVertex3f(13, TRANSFORM(Vec3f( x2, -y2, -z2)) );
  pos->setVertex3f(14, TRANSFORM(Vec3f( x2, -y2,  z2)) );
  pos->setVertex3f(15, TRANSFORM(Vec3f(-x2, -y2,  z2)) );
  // right
  pos->setVertex3f(16, TRANSFORM(Vec3f( x2, -y2, -z2)) );
  pos->setVertex3f(17, TRANSFORM(Vec3f( x2,  y2, -z2)) );
  pos->setVertex3f(18, TRANSFORM(Vec3f( x2,  y2,  z2)) );
  pos->setVertex3f(19, TRANSFORM(Vec3f( x2, -y2,  z2)) );
  // left
  pos->setVertex3f(20, TRANSFORM(Vec3f(-x2, -y2, -z2)) );
  pos->setVertex3f(21, TRANSFORM(Vec3f(-x2, -y2,  z2)) );
  pos->setVertex3f(22, TRANSFORM(Vec3f(-x2,  y2,  z2)) );
  pos->setVertex3f(23, TRANSFORM(Vec3f(-x2,  y2, -z2)) );
#undef TRANSFORM

  setInput(ref_ptr<ShaderInput>::cast(pos));

  // generate 'nor' attribute
  if(cfg.isNormalRequired)
  {
    ref_ptr<NormalShaderInput> nor = ref_ptr<NormalShaderInput>::manage(new NormalShaderInput);
    GLfloat* vertices = (GLfloat*)pos->dataPtr();

    nor->setVertexData(numCubeVertices);

    for(GLuint i=0; i<6; ++i)
    {
      Vec3f v0( vertices[i*12+0], vertices[i*12+1], vertices[i*12+2] );
      Vec3f v1( vertices[i*12+3], vertices[i*12+4], vertices[i*12+5] );
      Vec3f v2( vertices[i*12+6], vertices[i*12+7], vertices[i*12+8] );
      Vec3f normal = cross((v1 - v0), ( v2 - v0 ));

      for(GLuint j=0; j<4; ++j)
      {
#define TRANSFORM(x) transformVec3(rotMat,x)
        nor->setVertex3f(i*4 + j, TRANSFORM(normal));
#undef TRANSFORM
      }
    }
    setInput(ref_ptr<ShaderInput>::cast(nor));
  }

  // generate 'texco' attribute
  switch(cfg.texcoMode) {
  case TEXCO_MODE_NONE:
    break;
  case TEXCO_MODE_CUBE_MAP: {
    ref_ptr<TexcoShaderInput> texco = ref_ptr<TexcoShaderInput>::manage(new TexcoShaderInput( 0, 3 ));

    texco->setVertexData(numCubeVertices);

    GLfloat* vertices = (GLfloat*)pos->dataPtr();
    for(GLuint i=0; i<24; ++i)
    {
      Vec3f v( vertices[i*3+0], vertices[i*3+1], vertices[i*3+2] );
      normalize(v);
      texco->setVertex3f(i, v);
    }

    setInput(ref_ptr<ShaderInput>::cast(texco));
    break;
  }
  case TEXCO_MODE_UV: {
    ref_ptr<TexcoShaderInput> texco = ref_ptr<TexcoShaderInput>::manage(new TexcoShaderInput( 0, 2 ));

    texco->setVertexData(numCubeVertices);

#define TRANSFORM(x) cfg.texcoScale*x
    // front
    texco->setVertex2f( 0, TRANSFORM(Vec2f(1.0, 1.0)) );
    texco->setVertex2f( 1, TRANSFORM(Vec2f(0.0, 1.0)) );
    texco->setVertex2f( 2, TRANSFORM(Vec2f(0.0, 0.0)) );
    texco->setVertex2f( 3, TRANSFORM(Vec2f(1.0, 0.0)) );
    // back
    texco->setVertex2f( 4, TRANSFORM(Vec2f(0.0, 1.0)) );
    texco->setVertex2f( 5, TRANSFORM(Vec2f(0.0, 0.0)) );
    texco->setVertex2f( 6, TRANSFORM(Vec2f(1.0, 0.0)) );
    texco->setVertex2f( 7, TRANSFORM(Vec2f(1.0, 1.0)) );
    // top
    texco->setVertex2f( 8, TRANSFORM(Vec2f(1.0, 0.0)));
    texco->setVertex2f( 9, TRANSFORM(Vec2f(1.0, 1.0)));
    texco->setVertex2f(10, TRANSFORM(Vec2f(0.0, 1.0)));
    texco->setVertex2f(11, TRANSFORM(Vec2f(0.0, 0.0)));
    // bottom
    texco->setVertex2f(12, TRANSFORM(Vec2f(0.0, 0.0)));
    texco->setVertex2f(13, TRANSFORM(Vec2f(1.0, 0.0)));
    texco->setVertex2f(14, TRANSFORM(Vec2f(1.0, 1.0)));
    texco->setVertex2f(15, TRANSFORM(Vec2f(0.0, 1.0)));
    // right
    texco->setVertex2f(16, TRANSFORM(Vec2f(0.0, 1.0)));
    texco->setVertex2f(17, TRANSFORM(Vec2f(0.0, 0.0)));
    texco->setVertex2f(18, TRANSFORM(Vec2f(1.0, 0.0)));
    texco->setVertex2f(19, TRANSFORM(Vec2f(1.0, 1.0)));
    // left
    texco->setVertex2f(20, TRANSFORM(Vec2f(1.0, 1.0)));
    texco->setVertex2f(21, TRANSFORM(Vec2f(0.0, 1.0)));
    texco->setVertex2f(22, TRANSFORM(Vec2f(0.0, 0.0)));
    texco->setVertex2f(23, TRANSFORM(Vec2f(1.0, 0.0)));
#undef TRANSFORM

    setInput(ref_ptr<ShaderInput>::cast(texco));

    break;
  }}
}


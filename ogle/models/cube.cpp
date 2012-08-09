/*
 * Cube.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "cube.h"

UnitCube::UnitCube()
: AttributeState(GL_TRIANGLES)
{
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
  vector<MeshFace> faces;

  ref_ptr< vector<GLuint> > indexes;
  for(GLuint i=0; i<6; ++i)
  {
    indexes = ref_ptr< vector<GLuint> >::manage(new vector<GLuint>(6));
    indexes->data()[ 0] = i*4 + 0;
    indexes->data()[ 1] = i*4 + 1;
    indexes->data()[ 2] = i*4 + 2;
    indexes->data()[ 3] = i*4 + 0;
    indexes->data()[ 4] = i*4 + 2;
    indexes->data()[ 5] = i*4 + 3;
    faces.push_back( (MeshFace){indexes} );
  }
  setFaces(faces, 3);

  GLuint numCubeSides = 6;
  // TODO CUBE: the number of vertices can be reduced
  //    if cube vertices can share normal or if normal
  //    is not generated.
  GLuint numCubeVertices = numCubeSides*4;

  // generate 'pos' attribute
  ref_ptr<VertexAttributefv> pos = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_POS ));
  Mat4f rotMat = xyzRotationMatrix(
      cfg.rotation.x, cfg.rotation.y, cfg.rotation.z);
  GLfloat x2=0.5f, y2=0.5f, z2=0.5f;

  pos->setVertexData(numCubeVertices);

#define TRANSFORM(x) (cfg.posScale * transformVec3(rotMat,x))
  // front
  setAttributeVertex3f(pos.get(), 0, TRANSFORM(Vec3f(-x2, -y2,  z2)) );
  setAttributeVertex3f(pos.get(), 1, TRANSFORM(Vec3f( x2, -y2,  z2)) );
  setAttributeVertex3f(pos.get(), 2, TRANSFORM(Vec3f( x2,  y2,  z2)) );
  setAttributeVertex3f(pos.get(), 3, TRANSFORM(Vec3f(-x2,  y2,  z2)) );
  // back
  setAttributeVertex3f(pos.get(), 4, TRANSFORM(Vec3f(-x2, -y2, -z2)) );
  setAttributeVertex3f(pos.get(), 5, TRANSFORM(Vec3f(-x2,  y2, -z2)) );
  setAttributeVertex3f(pos.get(), 6, TRANSFORM(Vec3f( x2,  y2, -z2)) );
  setAttributeVertex3f(pos.get(), 7, TRANSFORM(Vec3f( x2, -y2, -z2)) );
  // top
  setAttributeVertex3f(pos.get(), 8, TRANSFORM(Vec3f(-x2,  y2, -z2)) );
  setAttributeVertex3f(pos.get(), 9, TRANSFORM(Vec3f(-x2,  y2,  z2)) );
  setAttributeVertex3f(pos.get(), 10, TRANSFORM(Vec3f( x2,  y2,  z2)) );
  setAttributeVertex3f(pos.get(), 11, TRANSFORM(Vec3f( x2,  y2, -z2)) );
  // bottom
  setAttributeVertex3f(pos.get(), 12, TRANSFORM(Vec3f(-x2, -y2, -z2)) );
  setAttributeVertex3f(pos.get(), 13, TRANSFORM(Vec3f( x2, -y2, -z2)) );
  setAttributeVertex3f(pos.get(), 14, TRANSFORM(Vec3f( x2, -y2,  z2)) );
  setAttributeVertex3f(pos.get(), 15, TRANSFORM(Vec3f(-x2, -y2,  z2)) );
  // right
  setAttributeVertex3f(pos.get(), 16, TRANSFORM(Vec3f( x2, -y2, -z2)) );
  setAttributeVertex3f(pos.get(), 17, TRANSFORM(Vec3f( x2,  y2, -z2)) );
  setAttributeVertex3f(pos.get(), 18, TRANSFORM(Vec3f( x2,  y2,  z2)) );
  setAttributeVertex3f(pos.get(), 19, TRANSFORM(Vec3f( x2, -y2,  z2)) );
  // left
  setAttributeVertex3f(pos.get(), 20, TRANSFORM(Vec3f(-x2, -y2, -z2)) );
  setAttributeVertex3f(pos.get(), 21, TRANSFORM(Vec3f(-x2, -y2,  z2)) );
  setAttributeVertex3f(pos.get(), 22, TRANSFORM(Vec3f(-x2,  y2,  z2)) );
  setAttributeVertex3f(pos.get(), 23, TRANSFORM(Vec3f(-x2,  y2, -z2)) );
#undef TRANSFORM

  setAttribute(pos);

  // generate 'nor' attribute
  if(cfg.isNormalRequired)
  {
    ref_ptr<VertexAttributefv> nor = ref_ptr<VertexAttributefv>::manage(
        new VertexAttributefv( ATTRIBUTE_NAME_NOR ));
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
        setAttributeVertex3f(nor.get(), i*4 + j, TRANSFORM(normal));
#undef TRANSFORM
      }
    }
    setAttribute(nor);
  }

  // generate 'texco' attribute
  switch(cfg.texcoMode) {
  case TEXCO_MODE_NONE:
    break;
  case TEXCO_MODE_CUBE_MAP: {
    ref_ptr<VertexAttributefv> texco = ref_ptr<VertexAttributefv>::manage(
        new TexcoAttribute( 0, 3 ));

    texco->setVertexData(numCubeVertices);

    GLfloat* vertices = (GLfloat*)pos->dataPtr();
    for(GLuint i=0; i<24; ++i)
    {
      Vec3f v( vertices[i*3+0], vertices[i*3+1], vertices[i*3+2] );
      normalize(v);
      setAttributeVertex3f(texco.get(), i, v);
    }

    setAttribute(texco);
    break;
  }
  case TEXCO_MODE_UV: {
    ref_ptr<VertexAttributefv> texco = ref_ptr<VertexAttributefv>::manage(
        new TexcoAttribute( 0, 2 ));

    texco->setVertexData(numCubeVertices);

#define TRANSFORM(x) cfg.texcoScale*x
    // front
    setAttributeVertex2f(texco.get(), 0, TRANSFORM(Vec2f(1.0, 1.0)) );
    setAttributeVertex2f(texco.get(), 1, TRANSFORM(Vec2f(0.0, 1.0)) );
    setAttributeVertex2f(texco.get(), 2, TRANSFORM(Vec2f(0.0, 0.0)) );
    setAttributeVertex2f(texco.get(), 3, TRANSFORM(Vec2f(1.0, 0.0)) );
    // back
    setAttributeVertex2f(texco.get(), 4, TRANSFORM(Vec2f(0.0, 1.0)) );
    setAttributeVertex2f(texco.get(), 5, TRANSFORM(Vec2f(0.0, 0.0)) );
    setAttributeVertex2f(texco.get(), 6, TRANSFORM(Vec2f(1.0, 0.0)) );
    setAttributeVertex2f(texco.get(), 7, TRANSFORM(Vec2f(1.0, 1.0)) );
    // top
    setAttributeVertex2f(texco.get(), 8, TRANSFORM(Vec2f(1.0, 0.0)));
    setAttributeVertex2f(texco.get(), 9, TRANSFORM(Vec2f(1.0, 1.0)));
    setAttributeVertex2f(texco.get(), 10, TRANSFORM(Vec2f(0.0, 1.0)));
    setAttributeVertex2f(texco.get(), 11, TRANSFORM(Vec2f(0.0, 0.0)));
    // bottom
    setAttributeVertex2f(texco.get(), 12, TRANSFORM(Vec2f(0.0, 0.0)));
    setAttributeVertex2f(texco.get(), 13, TRANSFORM(Vec2f(1.0, 0.0)));
    setAttributeVertex2f(texco.get(), 14, TRANSFORM(Vec2f(1.0, 1.0)));
    setAttributeVertex2f(texco.get(), 15, TRANSFORM(Vec2f(0.0, 1.0)));
    // right
    setAttributeVertex2f(texco.get(), 16, TRANSFORM(Vec2f(0.0, 1.0)));
    setAttributeVertex2f(texco.get(), 17, TRANSFORM(Vec2f(0.0, 0.0)));
    setAttributeVertex2f(texco.get(), 18, TRANSFORM(Vec2f(1.0, 0.0)));
    setAttributeVertex2f(texco.get(), 19, TRANSFORM(Vec2f(1.0, 1.0)));
    // left
    setAttributeVertex2f(texco.get(), 20, TRANSFORM(Vec2f(1.0, 1.0)));
    setAttributeVertex2f(texco.get(), 21, TRANSFORM(Vec2f(0.0, 1.0)));
    setAttributeVertex2f(texco.get(), 22, TRANSFORM(Vec2f(0.0, 0.0)));
    setAttributeVertex2f(texco.get(), 23, TRANSFORM(Vec2f(1.0, 0.0)));
#undef TRANSFORM

    setAttribute(texco);

    break;
  }}
}


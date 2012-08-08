/*
 * Cube.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "cube.h"

Cube::Cube()
: AttributeState(GL_TRIANGLES)
{
}

void Cube::createVertexData(
  const Vec3f &rotation,
  const Vec3f &scale,
  const Vec2f &uvScale,
  bool generateNormals,
  bool generateUV,
  bool generateCubeMapUV)
{
  vector<MeshFace> faces;
  ref_ptr< vector<GLuint> > indexes;
  ref_ptr<VertexAttributefv> pos = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_POS ));
  ref_ptr<VertexAttributefv> nor = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_NOR ));
  ref_ptr<VertexAttributefv> col0 = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_COL0, 4 ));
  ref_ptr<VertexAttributefv> uv0 = ref_ptr<VertexAttributefv>::manage(
      new TexcoAttribute( 0, generateCubeMapUV ? 3 : 2 ));

  for(unsigned int i=0; i<6; ++i) {
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

  Mat4f rotMat = xyzRotationMatrix(rotation.x, rotation.y, rotation.z);
  {
#define TRANSFORM(x) (scale * transformVec3(rotMat,x))
    pos->setVertexData(6*4);
    float x2=0.5f, y2=0.5f, z2=0.5f;

    // front
    setAttributeVertex3f(pos.get(), 0, TRANSFORM(((Vec3f) {-x2, -y2,  z2})) );
    setAttributeVertex3f(pos.get(), 1, TRANSFORM(((Vec3f) { x2, -y2,  z2})) );
    setAttributeVertex3f(pos.get(), 2, TRANSFORM(((Vec3f) { x2,  y2,  z2})) );
    setAttributeVertex3f(pos.get(), 3, TRANSFORM(((Vec3f) {-x2,  y2,  z2})) );
    // back
    setAttributeVertex3f(pos.get(), 4, TRANSFORM(((Vec3f) {-x2, -y2, -z2})) );
    setAttributeVertex3f(pos.get(), 5, TRANSFORM(((Vec3f) {-x2,  y2, -z2})) );
    setAttributeVertex3f(pos.get(), 6, TRANSFORM(((Vec3f) { x2,  y2, -z2})) );
    setAttributeVertex3f(pos.get(), 7, TRANSFORM(((Vec3f) { x2, -y2, -z2})) );
    // top
    setAttributeVertex3f(pos.get(), 8, TRANSFORM(((Vec3f) {-x2,  y2, -z2})) );
    setAttributeVertex3f(pos.get(), 9, TRANSFORM(((Vec3f) {-x2,  y2,  z2})) );
    setAttributeVertex3f(pos.get(), 10, TRANSFORM(((Vec3f) { x2,  y2,  z2})) );
    setAttributeVertex3f(pos.get(), 11, TRANSFORM(((Vec3f) { x2,  y2, -z2})) );
    // bottom
    setAttributeVertex3f(pos.get(), 12, TRANSFORM(((Vec3f) {-x2, -y2, -z2})) );
    setAttributeVertex3f(pos.get(), 13, TRANSFORM(((Vec3f) { x2, -y2, -z2})) );
    setAttributeVertex3f(pos.get(), 14, TRANSFORM(((Vec3f) { x2, -y2,  z2})) );
    setAttributeVertex3f(pos.get(), 15, TRANSFORM(((Vec3f) {-x2, -y2,  z2})) );
    // right
    setAttributeVertex3f(pos.get(), 16, TRANSFORM(((Vec3f) { x2, -y2, -z2})) );
    setAttributeVertex3f(pos.get(), 17, TRANSFORM(((Vec3f) { x2,  y2, -z2})) );
    setAttributeVertex3f(pos.get(), 18, TRANSFORM(((Vec3f) { x2,  y2,  z2})) );
    setAttributeVertex3f(pos.get(), 19, TRANSFORM(((Vec3f) { x2, -y2,  z2})) );
    // left
    setAttributeVertex3f(pos.get(), 20, TRANSFORM(((Vec3f) {-x2, -y2, -z2})) );
    setAttributeVertex3f(pos.get(), 21, TRANSFORM(((Vec3f) {-x2, -y2,  z2})) );
    setAttributeVertex3f(pos.get(), 22, TRANSFORM(((Vec3f) {-x2,  y2,  z2})) );
    setAttributeVertex3f(pos.get(), 23, TRANSFORM(((Vec3f) {-x2,  y2, -z2})) );

    setAttribute(pos);
#undef TRANSFORM
  }
  if(generateNormals){
#define TRANSFORM(x) transformVec3(rotMat,x)
    nor->setVertexData(6*4);
    GLfloat* vertices = (GLfloat*)pos->data->data();
    for(unsigned int i=0; i<6; ++i) {
      Vec3f v0 = (Vec3f) { vertices[i*12+0], vertices[i*12+1], vertices[i*12+2] };
      Vec3f v1 = (Vec3f) { vertices[i*12+3], vertices[i*12+4], vertices[i*12+5] };
      Vec3f v2 = (Vec3f) { vertices[i*12+6], vertices[i*12+7], vertices[i*12+8] };
      Vec3f normal = cross((v1 - v0), ( v2 - v0 ));
      for(unsigned int j=0; j<4; ++j) {
        setAttributeVertex3f(nor.get(), i*4 + j, TRANSFORM(normal));
      }
    }
    setAttribute(nor);
#undef TRANSFORM
  }
  if(generateCubeMapUV || generateUV) {
#define TRANSFORM(x) uvScale*x
    uv0->setVertexData(6*4);

    if(generateCubeMapUV) {
      GLfloat* vertices = (GLfloat*)pos->data->data();
      for(unsigned int i=0; i<24; ++i) {
        Vec3f v = (Vec3f) { vertices[i*3+0], vertices[i*3+1], vertices[i*3+2] };
        normalize(v);
        setAttributeVertex3f(uv0.get(), i, v);
      }
    } else {
      // front
      setAttributeVertex2f(uv0.get(), 0, TRANSFORM(((Vec2f) {1.0, 1.0})) );
      setAttributeVertex2f(uv0.get(), 1, TRANSFORM(((Vec2f) {0.0, 1.0})) );
      setAttributeVertex2f(uv0.get(), 2, TRANSFORM(((Vec2f) {0.0, 0.0})) );
      setAttributeVertex2f(uv0.get(), 3, TRANSFORM(((Vec2f) {1.0, 0.0})) );
      // back
      setAttributeVertex2f(uv0.get(), 4, TRANSFORM(((Vec2f) {0.0, 1.0})) );
      setAttributeVertex2f(uv0.get(), 5, TRANSFORM(((Vec2f) {0.0, 0.0})) );
      setAttributeVertex2f(uv0.get(), 6, TRANSFORM(((Vec2f) {1.0, 0.0})) );
      setAttributeVertex2f(uv0.get(), 7, TRANSFORM(((Vec2f) {1.0, 1.0})) );
      // top
      setAttributeVertex2f(uv0.get(), 8, TRANSFORM(((Vec2f) {1.0, 0.0})) );
      setAttributeVertex2f(uv0.get(), 9, TRANSFORM(((Vec2f) {1.0, 1.0})) );
      setAttributeVertex2f(uv0.get(), 10, TRANSFORM(((Vec2f) {0.0, 1.0})) );
      setAttributeVertex2f(uv0.get(), 11, TRANSFORM(((Vec2f) {0.0, 0.0})) );
      // bottom
      setAttributeVertex2f(uv0.get(), 12, TRANSFORM(((Vec2f) {0.0, 0.0})) );
      setAttributeVertex2f(uv0.get(), 13, TRANSFORM(((Vec2f) {1.0, 0.0})) );
      setAttributeVertex2f(uv0.get(), 14, TRANSFORM(((Vec2f) {1.0, 1.0})) );
      setAttributeVertex2f(uv0.get(), 15, TRANSFORM(((Vec2f) {0.0, 1.0})) );
      // right
      setAttributeVertex2f(uv0.get(), 16, TRANSFORM(((Vec2f) {0.0, 1.0})) );
      setAttributeVertex2f(uv0.get(), 17, TRANSFORM(((Vec2f) {0.0, 0.0})) );
      setAttributeVertex2f(uv0.get(), 18, TRANSFORM(((Vec2f) {1.0, 0.0})) );
      setAttributeVertex2f(uv0.get(), 19, TRANSFORM(((Vec2f) {1.0, 1.0})) );
      // left
      setAttributeVertex2f(uv0.get(), 20, TRANSFORM(((Vec2f) {1.0, 1.0})) );
      setAttributeVertex2f(uv0.get(), 21, TRANSFORM(((Vec2f) {0.0, 1.0})) );
      setAttributeVertex2f(uv0.get(), 22, TRANSFORM(((Vec2f) {0.0, 0.0})) );
      setAttributeVertex2f(uv0.get(), 23, TRANSFORM(((Vec2f) {1.0, 0.0})) );
    }

    setAttribute(uv0);
#undef TRANSFORM
  }
  //uploadGeometry();
}


/*
 * Quad.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "quad.h"

Quad::Quad()
: AttributeState(GL_QUADS)
{
}

void Quad::createVertexData(
    const Vec3f &rotation,
    const Vec3f &scale,
    const Vec2f &uvScale,
    unsigned int lod,
    bool generateTexco,
    bool generateNormal,
    bool centerAtOrigin,
    bool isOrtho)
{
  vector<MeshFace> faces;
  ref_ptr< vector<GLuint> > indexes;
  ref_ptr<VertexAttributefv> pos = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_POS ));
  ref_ptr<VertexAttributefv> nor = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_NOR ));
  ref_ptr<VertexAttributefv> uv0 = ref_ptr<VertexAttributefv>::manage(
      new UVAttribute( 0, 2 ));
  unsigned int numQuads = pow(4, lod);
  unsigned int numQuadsSide = sqrt(numQuads);
  float quadSize = 1.0/numQuadsSide;
  unsigned int counter;

  indexes = ref_ptr< vector<GLuint> >::manage(
      new vector<GLuint>(numQuads*4));
  pos->setVertexData(numQuads*4);
  if(generateNormal) {
    nor->setVertexData(numQuads*4);
  }
  if(generateTexco) {
    uv0->setVertexData(numQuads*4);
  }

  for(unsigned int i=0; i<numQuads; ++i) {
    indexes->data()[i*4 + 0] = i*4 + 0;
    indexes->data()[i*4 + 1] = i*4 + 1;
    indexes->data()[i*4 + 2] = i*4 + 2;
    indexes->data()[i*4 + 3] = i*4 + 3;
  }
  faces.push_back( (MeshFace){indexes} );

  Mat4f rotMat = xyzRotationMatrix(rotation.x, rotation.y, rotation.z);
  counter = 0;

  Vec3f startPos, curPos;
  Vec2f texcoPos;
  if(centerAtOrigin) {
    if(isOrtho) startPos = Vec3f(-scale.x*0.5f, 0.0f, -scale.z*0.5f);
    else         startPos = Vec3f(-scale.x*0.5f, scale.y*0.5f, 0.0f);
  } else {
    startPos = Vec3f(0.0f, 0.0f, 0.0f);
  }
  texcoPos = Vec2f(0.0f, 1.0f);

  texcoPos.x = 0.0f;
  curPos.x = startPos.x;
  if(isOrtho) curPos.y = 0.0f;
  else         curPos.z = 0.0f;
  for(unsigned int x=0; x<numQuadsSide; ++x)
  {
    texcoPos.y = 1.0f;
    if(isOrtho) curPos.z = startPos.z;
    else         curPos.y = startPos.y;
    for(unsigned int z=0; z<numQuadsSide; ++z)
    {
      {
        if(isOrtho) {
#define TRANSFORM(x) (scale*transformVec3(rotMat,x + curPos))
          setAttributeVertex3f(pos.get(), 4*counter + 0, TRANSFORM(Vec3f(0.0,0.0,0.0)));
          setAttributeVertex3f(pos.get(), 4*counter + 1, TRANSFORM(Vec3f(quadSize,0.0,0.0)));
          setAttributeVertex3f(pos.get(), 4*counter + 2, TRANSFORM(Vec3f(quadSize,0.0,quadSize)));
          setAttributeVertex3f(pos.get(), 4*counter + 3, TRANSFORM(Vec3f(0.0,0.0,quadSize)));
#undef TRANSFORM
        } else {
#define TRANSFORM(x) (scale*transformVec3(rotMat,x + curPos))
          setAttributeVertex3f(pos.get(), 4*counter + 0, TRANSFORM(Vec3f(0.0,0.0,0.0)));
          setAttributeVertex3f(pos.get(), 4*counter + 1, TRANSFORM(Vec3f(0.0,-quadSize,0.0)));
          setAttributeVertex3f(pos.get(), 4*counter + 2, TRANSFORM(Vec3f(quadSize,-quadSize,0.0)));
          setAttributeVertex3f(pos.get(), 4*counter + 3, TRANSFORM(Vec3f(quadSize,0.0,0.0)));
#undef TRANSFORM
        }
      }
      if(generateNormal) {
#define TRANSFORM(x) transformVec3(rotMat,x)
        setAttributeVertex3f(nor.get(), 4*counter + 0, TRANSFORM((Vec3f(0.0,0.0,1.0))) );
        setAttributeVertex3f(nor.get(), 4*counter + 1, TRANSFORM((Vec3f(0.0,0.0,1.0))) );
        setAttributeVertex3f(nor.get(), 4*counter + 2, TRANSFORM((Vec3f(0.0,0.0,1.0))) );
        setAttributeVertex3f(nor.get(), 4*counter + 3, TRANSFORM((Vec3f(0.0,0.0,1.0))) );
#undef TRANSFORM
      }
      if(generateTexco){
#define TRANSFORM(x) ( (uvScale*(x + texcoPos)) )
        if(isOrtho) {
          setAttributeVertex2f(uv0.get(), 4*counter + 0, TRANSFORM((Vec2f(0, 0))) );
          setAttributeVertex2f(uv0.get(), 4*counter + 1, TRANSFORM((Vec2f(quadSize, 0))) );
          setAttributeVertex2f(uv0.get(), 4*counter + 2, TRANSFORM((Vec2f(quadSize, quadSize))) );
          setAttributeVertex2f(uv0.get(), 4*counter + 3, TRANSFORM((Vec2f(0, quadSize))) );
        } else {
          setAttributeVertex2f(uv0.get(), 4*counter + 0, TRANSFORM(Vec2f(0, 0)) );
          setAttributeVertex2f(uv0.get(), 4*counter + 1, TRANSFORM(Vec2f(0, -quadSize)) );
          setAttributeVertex2f(uv0.get(), 4*counter + 2, TRANSFORM(Vec2f(quadSize, -quadSize)) );
          setAttributeVertex2f(uv0.get(), 4*counter + 3, TRANSFORM(Vec2f(quadSize, 0)) );
        }
#undef TRANSFORM
      }

      counter += 1;
      if(isOrtho) curPos.z += scale.z*quadSize;
      else         curPos.y -= scale.y*quadSize;
      texcoPos.y -= uvScale.y*quadSize;
    }
    texcoPos.x += uvScale.x*quadSize;
    curPos.x += scale.x*quadSize;
  }

  setFaces(faces, 4);
  setAttribute(pos);
  if(generateNormal) setAttribute(nor);
  if(generateTexco) setAttribute(uv0);
}

void Quad::createOrthoVertexData(
    const Vec3f &scale,
    bool generateTexco)
{
  createVertexData(
      Vec3f(0.5*M_PI,0.0,0.0), // rotation
      scale,
      Vec2f(1.0,1.0),
      0, // lod=0 -> 4 vertices
      generateTexco, // no uv
      false, // no normal
      false,  // do not center at 0,0,0
      true // ortho
      );
}

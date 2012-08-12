/*
 * Quad.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "quad.h"

UnitQuad::UnitQuad(const Config &cfg)
: AttributeState(GL_QUADS)
{
  updateAttributes(cfg);
}

UnitQuad::Config::Config()
: posScale(Vec3f(1.0f)),
  rotation(Vec3f(0.0f)),
  texcoScale(Vec2f(1.0f)),
  isTexcoRequired(true),
  isNormalRequired(true),
  centerAtOrigin(false),
  levelOfDetail(0)
{
}

void UnitQuad::updateAttributes(const Config &cfg)
{
  const GLuint numFaceIndices = 4;

  Mat4f rotMat = xyzRotationMatrix(cfg.rotation.x, cfg.rotation.y, cfg.rotation.z);
  GLuint numQuads = pow(4, cfg.levelOfDetail);
  GLuint numQuadsSide = sqrt(numQuads);
  GLfloat quadSize = 1.0/numQuadsSide;

  // allocate attributes
  ref_ptr<VertexAttributefv> pos = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_POS ));
  ref_ptr<VertexAttributefv> nor;
  ref_ptr<VertexAttributefv> texco;
  pos->setVertexData(numQuads*4);
  if(cfg.isNormalRequired) {
    nor = ref_ptr<VertexAttributefv>::manage(
          new VertexAttributefv( ATTRIBUTE_NAME_NOR ));
    nor->setVertexData(numQuads*4);
  }
  if(cfg.isTexcoRequired) {
    texco = ref_ptr<VertexAttributefv>::manage(
          new TexcoAttribute( 0, 2 ));
    texco->setVertexData(numQuads*4);
  }

  Vec3f startPos;
  if(cfg.centerAtOrigin) {
    startPos = Vec3f(-cfg.posScale.x*0.5f, 0.0f, -cfg.posScale.z*0.5f);
  } else {
    startPos = Vec3f(0.0f, 0.0f, 0.0f);
  }
  Vec2f texcoPos(0.0f, 1.0f);
  Vec3f curPos(startPos.x, 0.0f, 0.0f);
  GLuint vertexIndex = 0;

  for(GLuint x=0; x<numQuadsSide; ++x)
  {
    texcoPos.y = 1.0f;
    curPos.z = startPos.z;

    for(GLuint z=0; z<numQuadsSide; ++z)
    {
#define TRANSFORM(x) (cfg.posScale*transformVec3(rotMat,x + curPos))
      setAttributeVertex3f(pos.get(), vertexIndex + 0, TRANSFORM(Vec3f(0.0,0.0,0.0)));
      setAttributeVertex3f(pos.get(), vertexIndex + 1, TRANSFORM(Vec3f(quadSize,0.0,0.0)));
      setAttributeVertex3f(pos.get(), vertexIndex + 2, TRANSFORM(Vec3f(quadSize,0.0,quadSize)));
      setAttributeVertex3f(pos.get(), vertexIndex + 3, TRANSFORM(Vec3f(0.0,0.0,quadSize)));
#undef TRANSFORM

      if(cfg.isNormalRequired)
      {
#define TRANSFORM(x) transformVec3(rotMat,x)
        setAttributeVertex3f(nor.get(), vertexIndex + 0, TRANSFORM(Vec3f(0.0,0.0,1.0)));
        setAttributeVertex3f(nor.get(), vertexIndex + 1, TRANSFORM(Vec3f(0.0,0.0,1.0)));
        setAttributeVertex3f(nor.get(), vertexIndex + 2, TRANSFORM(Vec3f(0.0,0.0,1.0)));
        setAttributeVertex3f(nor.get(), vertexIndex + 3, TRANSFORM(Vec3f(0.0,0.0,1.0)));
#undef TRANSFORM
      }

      if(cfg.isTexcoRequired)
      {
#define TRANSFORM(x) ( (cfg.texcoScale*(x + texcoPos)) )
        setAttributeVertex2f(texco.get(), vertexIndex + 0, TRANSFORM(Vec2f(0, 0)));
        setAttributeVertex2f(texco.get(), vertexIndex + 1, TRANSFORM(Vec2f(quadSize, 0)));
        setAttributeVertex2f(texco.get(), vertexIndex + 2, TRANSFORM(Vec2f(quadSize, quadSize)));
        setAttributeVertex2f(texco.get(), vertexIndex + 3, TRANSFORM(Vec2f(0, quadSize)));
#undef TRANSFORM
      }

      vertexIndex += 4;
      curPos.z += cfg.posScale.z*quadSize;
      texcoPos.y -= cfg.texcoScale.y*quadSize;
    }
    texcoPos.x += cfg.texcoScale.x*quadSize;
    curPos.x += cfg.posScale.x*quadSize;
  }

  setAttribute(pos);
  if(cfg.isNormalRequired) {
    setAttribute(nor);
  }
  if(cfg.isTexcoRequired) {
    setAttribute(texco);
  }
}

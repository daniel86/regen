/*
 * rectangle.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "rectangle.h"

const ref_ptr<Rectangle>& Rectangle::getUnitQuad()
{
  static ref_ptr<Rectangle> unitQuad;
  if(unitQuad.get()==NULL) {
    Config cfg;
    cfg.centerAtOrigin = GL_FALSE;
    cfg.isNormalRequired = GL_FALSE;
    cfg.isTangentRequired = GL_FALSE;
    cfg.isTexcoRequired = GL_FALSE;
    cfg.levelOfDetail = 0;
    cfg.posScale = Vec3f(2.0f);
    cfg.rotation = Vec3f(0.5*M_PI, 0.0f, 0.0f);
    cfg.texcoScale = Vec2f(1.0);
    cfg.translation = Vec3f(-1.0f,-1.0f,0.0f);
    unitQuad = ref_ptr<Rectangle>::manage(new Rectangle(cfg));
  }
  return unitQuad;
}

Rectangle::Rectangle(const Config &cfg)
: MeshState(GL_TRIANGLES)
{
  updateAttributes(cfg);
}

Rectangle::Config::Config()
: levelOfDetail(0),
  posScale(Vec3f(1.0f)),
  rotation(Vec3f(0.0f)),
  translation(Vec3f(0.0f)),
  texcoScale(Vec2f(1.0f)),
  isNormalRequired(GL_TRUE),
  isTexcoRequired(GL_TRUE),
  isTangentRequired(GL_FALSE),
  centerAtOrigin(GL_FALSE)
{
}

void Rectangle::updateAttributes(Config cfg)
{
  Mat4f rotMat = Mat4f::rotationMatrix(cfg.rotation.x, cfg.rotation.y, cfg.rotation.z);
  GLuint numQuads = pow(4, cfg.levelOfDetail);
  GLuint numQuadsSide = sqrt(numQuads);
  GLfloat quadSize = 1.0/numQuadsSide;

  if(cfg.isTangentRequired) {
    cfg.isNormalRequired = GL_TRUE;
    cfg.isTexcoRequired = GL_TRUE;
  }

  // allocate attributes
  ref_ptr<PositionShaderInput> pos = ref_ptr<PositionShaderInput>::manage(new PositionShaderInput);
  ref_ptr<ShaderInput> nor, tan, texco;
  pos->setVertexData(numQuads*6);
  if(cfg.isNormalRequired) {
    nor = ref_ptr<ShaderInput>::manage(new NormalShaderInput);
    nor->setVertexData(numQuads*6);
  }
  if(cfg.isTexcoRequired) {
    texco = ref_ptr<ShaderInput>::manage(new TexcoShaderInput( 0, 2 ));
    texco->setVertexData(numQuads*6);
  }
  if(cfg.isTangentRequired) {
    tan = ref_ptr<ShaderInput>::manage(new TangentShaderInput);
    tan->setVertexData(numQuads*6);
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
    texcoPos.y = 0.0f;
    curPos.z = startPos.z;

    for(GLuint z=0; z<numQuadsSide; ++z)
    {
#define TRANSFORM(x) (rotMat.transform(cfg.posScale*x + curPos) + cfg.translation)
      Vec3f v0 = TRANSFORM(Vec3f(0.0,0.0,0.0));
      Vec3f v1 = TRANSFORM(Vec3f(quadSize,0.0,0.0));
      Vec3f v2 = TRANSFORM(Vec3f(quadSize,0.0,quadSize));
      Vec3f v3 = TRANSFORM(Vec3f(0.0,0.0,quadSize));
      pos->setVertex3f(vertexIndex + 0, v0);
      pos->setVertex3f(vertexIndex + 1, v1);
      pos->setVertex3f(vertexIndex + 2, v3);
      pos->setVertex3f(vertexIndex + 3, v1);
      pos->setVertex3f(vertexIndex + 4, v2);
      pos->setVertex3f(vertexIndex + 5, v3);
#undef TRANSFORM

      if(cfg.isNormalRequired)
      {
#define TRANSFORM(x) rotMat.transform(x)
        Vec3f n = TRANSFORM(Vec3f(0.0,-1.0,0.0));
        nor->setVertex3f(vertexIndex + 0, n);
        nor->setVertex3f(vertexIndex + 1, n);
        nor->setVertex3f(vertexIndex + 2, n);
        nor->setVertex3f(vertexIndex + 3, n);
        nor->setVertex3f(vertexIndex + 4, n);
        nor->setVertex3f(vertexIndex + 5, n);
#undef TRANSFORM
      }

      if(cfg.isTexcoRequired)
      {
#define TRANSFORM(x) ( cfg.texcoScale - (cfg.texcoScale*x + texcoPos) )
        Vec2f v0 = TRANSFORM(Vec2f(0, 0));
        Vec2f v1 = TRANSFORM(Vec2f(quadSize, 0));
        Vec2f v2 = TRANSFORM(Vec2f(quadSize, quadSize));
        Vec2f v3 = TRANSFORM(Vec2f(0, quadSize));
        texco->setVertex2f(vertexIndex + 0, v0);
        texco->setVertex2f(vertexIndex + 1, v1);
        texco->setVertex2f(vertexIndex + 2, v3);
        texco->setVertex2f(vertexIndex + 3, v1);
        texco->setVertex2f(vertexIndex + 4, v2);
        texco->setVertex2f(vertexIndex + 5, v3);
#undef TRANSFORM
      }

      if(cfg.isTangentRequired)
      {
        Vec3f *vertices = ((Vec3f*)pos->dataPtr())+vertexIndex;
        Vec2f *texcos = ((Vec2f*)texco->dataPtr())+vertexIndex;
        Vec3f *normals = ((Vec3f*)nor->dataPtr())+vertexIndex;
        Vec4f tangent = calculateTangent(vertices, texcos, *normals);
        tan->setVertex4f(vertexIndex + 0, tangent);
        tan->setVertex4f(vertexIndex + 1, tangent);
        tan->setVertex4f(vertexIndex + 2, tangent);
        tan->setVertex4f(vertexIndex + 3, tangent);
        tan->setVertex4f(vertexIndex + 4, tangent);
        tan->setVertex4f(vertexIndex + 5, tangent);
      }

      vertexIndex += 6;
      curPos.z += cfg.posScale.z*quadSize;
      texcoPos.y += cfg.texcoScale.y*quadSize;
    }
    texcoPos.x += cfg.texcoScale.x*quadSize;
    curPos.x += cfg.posScale.x*quadSize;
  }

  setInput(ref_ptr<ShaderInput>::cast(pos));
  if(cfg.isNormalRequired) {
    setInput(ref_ptr<ShaderInput>::cast(nor));
  }
  if(cfg.isTexcoRequired) {
    setInput(ref_ptr<ShaderInput>::cast(texco));
  }
  if(cfg.isTangentRequired) {
    setInput(ref_ptr<ShaderInput>::cast(tan));
  }
}

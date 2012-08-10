/*
 * helper.cpp
 *
 *  Created on: 24.02.2012
 *      Author: daniel
 */

#include "include/helper.h"
#include "include/liquid.h"

#include <volume-texture.h>

void swapBuffer(FluidBuffer &slab)
{
  GLuint fboIndex = slab.tex->bufferIndex();
  slab.tex->nextBuffer();
  slab.fbo->setAllAttachmentsActive(false);
  slab.fbo->setAttachmentActive(fboIndex, true);
}

void clearSlab(FluidBuffer &slab)
{
  GLuint fboIndex = slab.tex->bufferIndex();
  slab.tex->nextBuffer();

  slab.fbo->bind();
  slab.fbo->setAllAttachmentsActive(true);
  slab.fbo->drawBufferMRT();
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  slab.fbo->setAllAttachmentsActive(false);
  slab.fbo->setAttachmentActive(fboIndex, true);
}

FluidBuffer createSlab(EulerianPrimitive *primitive,
    int numComponents, int numTexs, bool useHalfFloats)
{
  ref_ptr<FrameBufferObject> fbo =
      ref_ptr<FrameBufferObject>::manage( new FrameBufferObject() );
  fbo->bind();

  ref_ptr<Texture> tex = createTexture(
      primitive->width(), primitive->height(), primitive->depth(),
      numComponents, numTexs, useHalfFloats);

  for(int i=0; i<numTexs; ++i) {
    tex->bind();
    fbo->addColorAttachment( *tex.get() );
    tex->nextBuffer();
  }

  FluidBuffer surface;
  surface.fbo = fbo;
  surface.tex = tex;
  clearSlab(surface);

  return surface;
}

ref_ptr<Texture> createTexture(
    GLint width, GLint height, GLint depth,
    GLint numComponents,
    GLint numTexs,
    GLboolean useHalfFloats)
{
  ref_ptr<Texture> tex;

  if(depth < 2) {
    tex = ref_ptr<Texture>::manage( new Texture2D(numTexs) );
  } else {
    ref_ptr<Texture3D> tex3D = ref_ptr<Texture3D>::manage( new Texture3D(numTexs) );
    tex3D->set_numTextures(depth);
    tex = tex3D;
  }
  tex->set_size(width, height);

  if (useHalfFloats) {
    tex->set_pixelType(GL_HALF_FLOAT);
    switch (numComponents) {
    case 1: tex->set_internalFormat(GL_R16F); tex->set_format(GL_RED); break;
    case 2: tex->set_internalFormat(GL_RG16F); tex->set_format(GL_RG); break;
    case 3: tex->set_internalFormat(GL_RGB16F); tex->set_format(GL_RGB); break;
    case 4: tex->set_internalFormat(GL_RGBA16F); tex->set_format(GL_RGBA); break;
    }
  } else {
    tex->set_pixelType(GL_FLOAT);
    switch (numComponents) {
    case 1: tex->set_internalFormat(GL_R32F); tex->set_format(GL_RED); break;
    case 2: tex->set_internalFormat(GL_RG32F); tex->set_format(GL_RG); break;
    case 3: tex->set_internalFormat(GL_RGB32F); tex->set_format(GL_RGB); break;
    case 4: tex->set_internalFormat(GL_RGBA32F); tex->set_format(GL_RGBA); break;
    }
  }

  for(int i=0; i<numTexs; ++i) {
    tex->bind();
    GLenum mode = GL_CLAMP_TO_EDGE;
    tex->set_wrappingU(mode);
    tex->set_wrappingV(mode);
    tex->set_wrappingW(mode);
    tex->set_filter(GL_LINEAR, GL_LINEAR);
    tex->texImage();
    tex->nextBuffer();
  }

  return tex;
}

////////// GLSL helper

vector<string> makeShaderArgs() {
  vector<string> shaderArgs_;
  shaderArgs_.push_back( "fragmentColor_" );
  return shaderArgs_;
}

string findNeighborsGLSL(const string &varName, const string &texName, bool is2D) {
  stringstream s;
  if(is2D) {
    s << "     vec4 " << varName << "N = texelFetchOffset(" <<
        texName << ", pos, 0, ivec2(0, 1));" << endl;
    s << "     vec4 " << varName << "S = texelFetchOffset(" <<
        texName << ", pos, 0, ivec2(0, -1));" << endl;
    s << "     vec4 " << varName << "E = texelFetchOffset(" <<
        texName << ", pos, 0, ivec2(1, 0));" << endl;
    s << "     vec4 " << varName << "W = texelFetchOffset(" <<
        texName << ", pos, 0, ivec2(-1, 0));" << endl;
  } else {
    s << "     vec4 " << varName << "N = texelFetchOffset(" <<
        texName << ", pos, 0, ivec3(0, 1, 0));" << endl;
    s << "     vec4 " << varName << "S = texelFetchOffset(" <<
        texName << ", pos, 0, ivec3(0, -1, 0));" << endl;
    s << "     vec4 " << varName << "E = texelFetchOffset(" <<
        texName << ", pos, 0, ivec3(1, 0, 0));" << endl;
    s << "     vec4 " << varName << "W = texelFetchOffset(" <<
        texName << ", pos, 0, ivec3(-1, 0, 0));" << endl;
    s << "     vec4 " << varName << "F = texelFetchOffset(" <<
        texName << ", pos, 0, ivec3(0, 0, 1));" << endl;
    s << "     vec4 " << varName << "B = texelFetchOffset(" <<
        texName << ", pos, 0, ivec3(0, 0, -1));" << endl;
  }
  return s.str();
}

string isOutsideSimulationDomainGLSL(bool is2D) {
  stringstream s;
  if(is2D) {
    s << "bool isOutsideSimulationDomain(vec2 pos) {" << endl;
    s << "    return texture(" << EulerianLiquid::LEVEL_SET << ", pos).r > 0.0;" << endl;
    s << "}" << endl;
  } else {
    s << "bool isOutsideSimulationDomain(vec3 pos) {" << endl;
    s << "    return texture(" << EulerianLiquid::LEVEL_SET << ", pos).r > 0.0;" << endl;
    s << "}" << endl;
  }
  return s.str();
}

string isNonEmptyCellGLSL(bool is2D) {
  stringstream s;
  if(is2D) {
    s << "bool isNonEmptyCell(vec2 pos) {" << endl;
    s << "    return texture(" << EulerianFluid::OBSTACLES << ", pos).r > 0;" << endl;
    s << "}" << endl;
  } else {
    s << "bool isNonEmptyCell(vec3 pos) {" << endl;
    s << "    return texture(" << EulerianFluid::OBSTACLES << ", pos).r > 0;" << endl;
    s << "}" << endl;
  }
  return s.str();
}

string gradientGLSL(bool is2D) {
  stringstream s;
  if(is2D) {
    s << "vec2 gradient(sampler2D tex, ivec2 pos) {" << endl;
    s << findNeighborsGLSL("gradient", "tex", is2D);
    s << "    // tex is supposed to be a scalar texture" << endl;
    s << "    return 0.5f * vec2(gradientE.x - gradientW.x,  gradientN.x - gradientS.x);" << endl;
    s << "}" << endl;
  } else {
    s << "vec2 gradient(sampler3D tex, ivec3 pos) {" << endl;
    s << findNeighborsGLSL("gradient", "tex", is2D);
    s << "    // tex is supposed to be a scalar texture" << endl;
    s << "    return 0.5f * vec2(gradientE.x - gradientW.x,  gradientN.x - gradientS.x,  gradientF.x - gradientB.x);" << endl;
    s << "}" << endl;
  }
  return s.str();
}

string gradientLiquidGLSL(bool is2D) {
  stringstream s;
  if(is2D) {
    s << "vec2 gradientLiquid(sampler2D tex, ivec2 pos, float minSlope, out bool isBoundary, out bool highEnoughSlope) {" << endl;
    s << "    float CC = texelFetchOffset( tex, pos, 0, ivec2( 0,  0 ) ).r;" << endl;
    s << "    float CT = texelFetchOffset( tex, pos, 0, ivec2( 0,  1 ) ).r;" << endl;
    s << "    float CB = texelFetchOffset( tex, pos, 0, ivec2( 0, -1 ) ).r;" << endl;
    s << "    float RC = texelFetchOffset( tex, pos, 0, ivec2( 1,  0 ) ).r;" << endl;
    s << "    float RT = texelFetchOffset( tex, pos, 0, ivec2( 1,  1 ) ).r;" << endl;
    s << "    float RB = texelFetchOffset( tex, pos, 0, ivec2( 1, -1 ) ).r;" << endl;
    s << "    float LC = texelFetchOffset( tex, pos, 0, ivec2(-1,  0 ) ).r;" << endl;
    s << "    float LT = texelFetchOffset( tex, pos, 0, ivec2(-1,  1 ) ).r;" << endl;
    s << "    float LB = texelFetchOffset( tex, pos, 0, ivec2(-1, -1 ) ).r;" << endl;
    s << endl;
    s << "    // is this cell next to the LevelSet boundary" << endl;
    s << "    float product = LC * RC * CB * CT * LB * LT * RB * RT;" << endl;
    s << "    isBoundary = (product < 0 ? true : false);" << endl;
    s << "    // is the slope high enough" << endl;
    s << "    highEnoughSlope = ( (abs(RC - CC) > minSlope) || (abs(LC - CC) > minSlope) || (abs(CT - CC) > minSlope) || (abs(CB - CC) > minSlope) );" << endl;
    s << endl;
    s << "    return 0.5 * vec2(RC - LC,  CT - CB);" << endl;
    s << "}" << endl;
  } else {
    s << "vec2 gradientLiquid(sampler3D tex, ivec3 pos, float minSlope, out bool isBoundary, out bool highEnoughSlope) {" << endl;
    s << "    float LCC = texelFetchOffset( tex, pos, 0, ivec3(-1,  0,  0) ).r;" << endl;
    s << "    float RCC = texelFetchOffset( tex, pos, 0, ivec3( 1,  0,  0) ).r;" << endl;
    s << "    float CBC = texelFetchOffset( tex, pos, 0, ivec3( 0, -1,  0) ).r;" << endl;
    s << "    float CTC = texelFetchOffset( tex, pos, 0, ivec3( 0,  1,  0) ).r;" << endl;
    s << "    float CCD = texelFetchOffset( tex, pos, 0, ivec3( 0,  0, -1) ).r;" << endl;
    s << "    float CCU = texelFetchOffset( tex, pos, 0, ivec3( 0,  0,  1) ).r;" << endl;
    s << "    float LBD = texelFetchOffset( tex, pos, 0, ivec3(-1, -1, -1) ).r;" << endl;
    s << "    float LBC = texelFetchOffset( tex, pos, 0, ivec3(-1, -1,  0) ).r;" << endl;
    s << "    float LBU = texelFetchOffset( tex, pos, 0, ivec3(-1, -1,  1) ).r;" << endl;
    s << "    float LCD = texelFetchOffset( tex, pos, 0, ivec3(-1,  0, -1) ).r;" << endl;
    s << "    float LCU = texelFetchOffset( tex, pos, 0, ivec3(-1,  0,  1) ).r;" << endl;
    s << "    float LTD = texelFetchOffset( tex, pos, 0, ivec3(-1,  1, -1) ).r;" << endl;
    s << "    float LTC = texelFetchOffset( tex, pos, 0, ivec3(-1,  1,  0) ).r;" << endl;
    s << "    float LTU = texelFetchOffset( tex, pos, 0, ivec3(-1,  1,  1) ).r;" << endl;
    s << "    float CBD = texelFetchOffset( tex, pos, 0, ivec3( 0, -1, -1) ).r;" << endl;
    s << "    float CBU = texelFetchOffset( tex, pos, 0, ivec3( 0, -1,  1) ).r;" << endl;
    s << "    float CCC = texelFetchOffset( tex, pos, 0, ivec3( 0,  0,  0) ).r;" << endl;
    s << "    float CTD = texelFetchOffset( tex, pos, 0, ivec3( 0,  1, -1) ).r;" << endl;
    s << "    float CTU = texelFetchOffset( tex, pos, 0, ivec3( 0,  1,  1) ).r;" << endl;
    s << "    float RBD = texelFetchOffset( tex, pos, 0, ivec3( 1, -1, -1) ).r;" << endl;
    s << "    float RBC = texelFetchOffset( tex, pos, 0, ivec3( 1, -1,  0) ).r;" << endl;
    s << "    float RBU = texelFetchOffset( tex, pos, 0, ivec3( 1, -1,  1) ).r;" << endl;
    s << "    float RCD = texelFetchOffset( tex, pos, 0, ivec3( 1,  0, -1) ).r;" << endl;
    s << "    float RCU = texelFetchOffset( tex, pos, 0, ivec3( 1,  0,  1) ).r;" << endl;
    s << "    float RTD = texelFetchOffset( tex, pos, 0, ivec3( 1,  1, -1) ).r;" << endl;
    s << "    float RTC = texelFetchOffset( tex, pos, 0, ivec3( 1,  1,  0) ).r;" << endl;
    s << "    float RTU = texelFetchOffset( tex, pos, 0, ivec3( 1,  1,  1) ).r;" << endl;
    s << endl;
    s << "    // is this cell next to the LevelSet boundary" << endl;
    s << "    float product = LCC * RCC * CBC * CTC * CCD * CCU;" << endl;
    s << "    product *= LBD * LBC * LBU * LCD * LCU * LTD * LTC * LTU" << endl;
    s << "             * CBD * CBU * CTD * CTU" << endl;
    s << "             * RBD * RBC * RBU * RCD * RCU * RTD * RTC * RTU;" << endl;
    s << "    isBoundary = product < 0;" << endl;
    s << "    // is the slope high enough" << endl;
    s << "    highEnoughSlope = (abs(R - CCC) > minSlope) || (abs(L - CCC) > minSlope) ||" << endl;
    s << "             (abs(T - CCC) > minSlope) || (abs(B - CCC) > minSlope) ||" << endl;
    s << "             (abs(U - CCC) > minSlope) || (abs(D - CCC) > minSlope);" << endl;
    s << "    return 0.5 * vec3(RCC - LCC,  CTC - CBC,  CCU - CCD);" << endl;
    s << endl;
    s << "}" << endl;
  }
  return s.str();
}

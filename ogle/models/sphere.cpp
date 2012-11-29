/*
 * Cube.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "sphere.h"

typedef struct {
  Vec3f p1;
  Vec3f p2;
  Vec3f p3;
}SphereFace;

vector<SphereFace>* makeSphere(GLuint levelOfDetail)
{
  GLuint numFaces_ = pow(4.0,(GLint)levelOfDetail)*8;
  vector<SphereFace> *faces = new vector<SphereFace>(numFaces_);
  vector<SphereFace> &f = *faces;
  GLuint i, j, numFaces=0, numNewFaces;
  Vec3f pa,pb,pc;

  { // setup initial level
    GLfloat a = 1.0 / sqrt(2.0) + 0.001;
    Vec3f p[6] = {
        Vec3f( 0.0f,0.0f,1.0f),
        Vec3f( 0.0f,0.0f,-1.0f),
        Vec3f( -a,-a,0.0f),
        Vec3f( a,-a,0.0f),
        Vec3f( a,a,0.0f),
        Vec3f( -a,a,0.0f)
    };
    f[0] = (SphereFace) { p[0], p[3], p[4] };
    f[1] = (SphereFace) { p[0], p[4], p[5] };
    f[2] = (SphereFace) { p[0], p[5], p[2] };
    f[3] = (SphereFace) { p[0], p[2], p[3] };
    f[4] = (SphereFace) { p[1], p[4], p[3] };
    f[5] = (SphereFace) { p[1], p[5], p[4] };
    f[6] = (SphereFace) { p[1], p[2], p[5] };
    f[7] = (SphereFace) { p[1], p[3], p[2] };
    numFaces = 8;
  }

  for (j=0; j<levelOfDetail; ++j)
  {
    numNewFaces = numFaces;
    for (i=0; i<numNewFaces; ++i)
    {
      pa = (f[i].p1 + f[i].p2)*0.5; normalize(pa);
      pb = (f[i].p2 + f[i].p3)*0.5; normalize(pb);
      pc = (f[i].p3 + f[i].p1)*0.5; normalize(pc);

      f[numFaces] = (SphereFace) { f[i].p1, pa, pc }; ++numFaces;
      f[numFaces] = (SphereFace) { pa, f[i].p2, pb }; ++numFaces;
      f[numFaces] = (SphereFace) { pb, f[i].p3, pc }; ++numFaces;
      f[i] = (SphereFace) { pa, pb, pc };
    }
  }

  return faces;
}

static void sphereUV(const Vec3f &p, GLfloat *s, GLfloat *t)
{
  *s = atan2(p.x, p.z) / (2. * M_PI) + 0.5;
  *t = asin(p.y) / M_PI + 0.5;
}


UnitSphere::UnitSphere(const Config &cfg)
: MeshState(GL_TRIANGLES)
{
  updateAttributes(cfg);
}

UnitSphere::Config::Config()
: posScale(Vec3f(1.0f)),
  texcoScale(Vec2f(1.0f)),
  levelOfDetail(4),
  texcoMode(TEXCO_MODE_UV),
  isNormalRequired(true)
{
}

void UnitSphere::updateAttributes(const Config &cfg)
{
  vector<SphereFace> *faces = makeSphere(cfg.levelOfDetail);
  vector<Vec3f> verts;
  vector<Vec3f> nors;
  vector<Vec2f> texcos;

  // generate arrays of attribute data from faces
  GLuint vertexIndex=0u;
  for(vector<SphereFace>::iterator
      it = faces->begin(); it != faces->end(); ++it)
  {
    verts.push_back( it->p1 * 0.5 );
    verts.push_back( it->p2 * 0.5 );
    verts.push_back( it->p3 * 0.5 );
    if(cfg.isNormalRequired)
    {
      nors.push_back( it->p1 );
      nors.push_back( it->p2 );
      nors.push_back( it->p3 );
    }

    switch(cfg.texcoMode) {
    case TEXCO_MODE_NONE:
      break;
    case TEXCO_MODE_UV:
      texcos.push_back( Vec2f(
        0.5f + verts[vertexIndex].x/cfg.posScale.x,
        0.5f + verts[vertexIndex].y/cfg.posScale.y
      ) );
      texcos.push_back( Vec2f(
        0.5f + verts[vertexIndex+1u].x/cfg.posScale.x,
        0.5f + verts[vertexIndex+1u].y/cfg.posScale.y
      ) );
      texcos.push_back( Vec2f(
        0.5f + verts[vertexIndex+2u].x/cfg.posScale.x,
        0.5f + verts[vertexIndex+2u].y/cfg.posScale.y
      ) );
      break;
    case TEXCO_MODE_SPHERE_MAP: {
      GLfloat s1, s2, s3, t1, t2, t3;

      sphereUV(it->p1, &s1, &t1);
      sphereUV(it->p2, &s2, &t2);
      if(s2 < 0.75 && s1 > 0.75) {
        s2 += 1.0;
      } else if(s2 > 0.75 && s1 < 0.75) {
        s2 -= 1.0;
      }

      sphereUV(it->p3, &s3, &t3);
      if(s3 < 0.75 && s2 > 0.75) {
        s3 += 1.0;
      } else if(s3 > 0.75 && s2 < 0.75) {
        s3 -= 1.0;
      }

      texcos.push_back( Vec2f(s1, t1) );
      texcos.push_back( Vec2f(s2, t2) );
      texcos.push_back( Vec2f(s3, t3) );

      break;
    }}

    vertexIndex += 3u;
  }

  delete faces;

  // initial VertexAttribute's
  ref_ptr<PositionShaderInput> pos =
      ref_ptr<PositionShaderInput>::manage(new PositionShaderInput);
  ref_ptr<NormalShaderInput> nor =
      ref_ptr<NormalShaderInput>::manage(new NormalShaderInput);
  ref_ptr<TexcoShaderInput> texco =
      ref_ptr<TexcoShaderInput>::manage(new TexcoShaderInput( 0, 2 ));

  // allocate RAM for the data
  pos->setVertexData(vertexIndex);
  if(!nors.empty()) {
    nor->setVertexData(vertexIndex);
  }
  if(!texcos.empty()) {
    texco->setVertexData(vertexIndex);
  }
  // copy data from initialed vectors
  for(GLuint i=0; i<vertexIndex; ++i)
  {
    pos->setVertex3f(i, cfg.posScale * verts[i] );
    if(!nors.empty()) {
      nor->setVertex3f(i, nors[i] );
    }
    if(!texcos.empty()) {
      texco->setVertex2f(i, cfg.texcoScale * texcos[i] );
    }
  }

  setInput(ref_ptr<ShaderInput>::cast(pos));
  if(!nors.empty()) {
    setInput(ref_ptr<ShaderInput>::cast(nor));
  }
  if(!texcos.empty()) {
    setInput(ref_ptr<ShaderInput>::cast(texco));
  }
}

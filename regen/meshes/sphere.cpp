/*
 * Cube.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "sphere.h"
using namespace regen;

vector<Sphere::SphereFace>* Sphere::makeSphere(GLuint levelOfDetail)
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
    f[0] = SphereFace( p[0], p[3], p[4] );
    f[1] = SphereFace( p[0], p[4], p[5] );
    f[2] = SphereFace( p[0], p[5], p[2] );
    f[3] = SphereFace( p[0], p[2], p[3] );
    f[4] = SphereFace( p[1], p[4], p[3] );
    f[5] = SphereFace( p[1], p[5], p[4] );
    f[6] = SphereFace( p[1], p[2], p[5] );
    f[7] = SphereFace( p[1], p[3], p[2] );
    numFaces = 8;
  }

  for (j=0; j<levelOfDetail; ++j)
  {
    numNewFaces = numFaces;
    for (i=0; i<numNewFaces; ++i)
    {
      pa = (f[i].p1 + f[i].p2)*0.5; pa.normalize();
      pb = (f[i].p2 + f[i].p3)*0.5; pb.normalize();
      pc = (f[i].p3 + f[i].p1)*0.5; pc.normalize();

      f[numFaces] = SphereFace( f[i].p1, pa, pc ); ++numFaces;
      f[numFaces] = SphereFace( pa, f[i].p2, pb ); ++numFaces;
      f[numFaces] = SphereFace( pb, f[i].p3, pc ); ++numFaces;
      f[i] = SphereFace( pa, pb, pc );
    }
  }

  return faces;
}

static void sphereUV(const Vec3f &p, GLfloat *s, GLfloat *t)
{
  *s = atan2(p.x, p.z) / (2. * M_PI) + 0.5;
  *t = asin(p.y) / M_PI + 0.5;
}


Sphere::Sphere(const Config &cfg)
: Mesh(GL_TRIANGLES)
{
  pos_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f(ATTRIBUTE_NAME_POS));
  nor_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f(ATTRIBUTE_NAME_NOR));
  texco_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("texco0"));
  tan_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f(ATTRIBUTE_NAME_TAN));

  updateAttributes(cfg);
}

Sphere::Config::Config()
: posScale(Vec3f(1.0f)),
  texcoScale(Vec2f(1.0f)),
  levelOfDetail(4),
  texcoMode(TEXCO_MODE_UV),
  isNormalRequired(true),
  isTangentRequired(false)
{
}

void Sphere::updateAttributes(const Config &cfg)
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

  // allocate RAM for the data
  pos_->setVertexData(vertexIndex);
  if(!nors.empty()) {
    nor_->setVertexData(vertexIndex);
  }
  if(!texcos.empty()) {
    texco_->setVertexData(vertexIndex);
  }
  // copy data from initialed vectors
  for(GLuint i=0; i<vertexIndex; ++i)
  {
    pos_->setVertex3f(i, cfg.posScale * verts[i] );
    if(!nors.empty()) {
      nor_->setVertex3f(i, nors[i] );
    }
    if(!texcos.empty()) {
      texco_->setVertex2f(i, cfg.texcoScale * texcos[i] );
    }
  }

  setInput(pos_);
  if(!nors.empty()) {
    setInput(nor_);
  }
  if(!texcos.empty()) {
    setInput(texco_);
  }

  if(cfg.isTangentRequired)
  {
    tan_->setVertexData(vertexIndex);

    for(GLuint i=0; i<verts.size(); ++i)
    {
      Vec3f &v = verts[i];
      Vec3f vAbs = Vec3f((v.x), (v.y), (v.z));
      Vec3f v_;
      if(vAbs.x < vAbs.y && vAbs.x < vAbs.z) {
        v_ = Vec3f(0.0, -v.z, v.y);
      }
      else if (vAbs.y < vAbs.x && vAbs.y < vAbs.z) {
        v_ = Vec3f(-v.z, 0, v.x);
      }
      else {
        v_ = Vec3f(-v.y, v.x, 0);
      }
      v_.normalize();
      Vec3f t = v.cross(v_);
      tan_->setVertex4f(i, Vec4f(t.x, t.y, t.z, 1.0) );
    }

    setInput(tan_);
  }
}

///////////
///////////


SphereSprite::SphereSprite(const Config &cfg)
: Mesh(GL_POINTS), HasShader("sprite.sphere")
{
  updateAttributes(cfg);
  joinStates(shaderState());
}

void SphereSprite::updateAttributes(const Config &cfg)
{
  numVertices_ = cfg.sphereCount;

  ref_ptr<ShaderInput1f> radiusIn =
      ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("sphereRadius"));
  radiusIn->setVertexData(cfg.sphereCount);

  ref_ptr<ShaderInput3f> positionIn =
      ref_ptr<ShaderInput3f>::manage(new ShaderInput3f(ATTRIBUTE_NAME_POS));
  positionIn->setVertexData(cfg.sphereCount);

  for(GLuint i=0; i<cfg.sphereCount; ++i) {
    radiusIn->setVertex1f(i, cfg.radius[i]);
    positionIn->setVertex3f(i, cfg.position[i]);
  }

  setInput(radiusIn);
  setInput(positionIn);
}

/*
 * Cube.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "tessellation.h"
#include "sphere.h"
using namespace regen;

namespace regen {
  ostream& operator<<(ostream &out, const Sphere::TexcoMode &mode)
  {
    switch(mode) {
    case Sphere::TEXCO_MODE_NONE:       return out << "NONE";
    case Sphere::TEXCO_MODE_UV:         return out << "UV";
    case Sphere::TEXCO_MODE_SPHERE_MAP: return out << "SPHERE_MAP";
    }
    return out;
  }
  istream& operator>>(istream &in, Sphere::TexcoMode &mode)
  {
    string val;
    in >> val;
    boost::to_upper(val);
    if(val == "NONE")            mode = Sphere::TEXCO_MODE_NONE;
    else if(val == "UV")         mode = Sphere::TEXCO_MODE_UV;
    else if(val == "SPHERE_MAP") mode = Sphere::TEXCO_MODE_SPHERE_MAP;
    else {
      REGEN_WARN("Unknown sphere texco mode '" << val << "'. Using NONE texco.");
      mode = Sphere::TEXCO_MODE_NONE;
    }
    return in;
  }
}

static void sphereUV(const Vec3f &p, GLfloat *s, GLfloat *t)
{
  *s = atan2(p.x, p.z) / (2. * M_PI) + 0.5;
  *t = asin(p.y) / M_PI + 0.5;
}


Sphere::Sphere(const Config &cfg)
: Mesh(GL_TRIANGLES, cfg.usage)
{
  pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
  nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
  texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
  tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);

  updateAttributes(cfg);
}

Sphere::Config::Config()
: posScale(Vec3f(1.0f)),
  texcoScale(Vec2f(1.0f)),
  levelOfDetail(4),
  texcoMode(TEXCO_MODE_UV),
  isNormalRequired(GL_TRUE),
  isTangentRequired(GL_FALSE),
  isHalfSphere(GL_FALSE),
  usage(VBO::USAGE_DYNAMIC)
{
}

void Sphere::updateAttributes(const Config &cfg)
{
  vector<TriangleFace> *faces; {
    // setup initial level
    GLfloat a = 1.0 / sqrt(2.0) + 0.001;
    Vec3f level0[6] = {
        Vec3f( 0.0f,0.0f,1.0f),
        Vec3f( 0.0f,0.0f,-1.0f),
        Vec3f( -a,-a,0.0f),
        Vec3f( a,-a,0.0f),
        Vec3f( a,a,0.0f),
        Vec3f( -a,a,0.0f)
    };
    vector<TriangleFace> facesLevel0(8);
    facesLevel0[0] = TriangleFace( level0[0], level0[3], level0[4] );
    facesLevel0[1] = TriangleFace( level0[0], level0[4], level0[5] );
    facesLevel0[2] = TriangleFace( level0[0], level0[5], level0[2] );
    facesLevel0[3] = TriangleFace( level0[0], level0[2], level0[3] );
    facesLevel0[4] = TriangleFace( level0[1], level0[4], level0[3] );
    facesLevel0[5] = TriangleFace( level0[1], level0[5], level0[4] );
    facesLevel0[6] = TriangleFace( level0[1], level0[2], level0[5] );
    facesLevel0[7] = TriangleFace( level0[1], level0[3], level0[2] );
    faces = tessellate(cfg.levelOfDetail, facesLevel0);
  }
  vector<Vec3f> verts;
  vector<Vec3f> nors;
  vector<Vec2f> texcos;

  // generate arrays of attribute data from faces
  GLuint vertexIndex=0u;
  for(vector<TriangleFace>::iterator
      it = faces->begin(); it != faces->end(); ++it)
  {
    if(cfg.isHalfSphere) {
      GLuint counter=0;
      if(it->p1.y>0) {
        counter += 1;
        it->p1.y = 0.0;
      }
      if(it->p2.y>0) {
        counter += 1;
        it->p2.y = 0.0;
      }
      if(it->p3.y>0) {
        counter += 1;
        it->p3.y = 0.0;
      }
      if(counter==3) {
        continue;
      }
    }
    it->p1.normalize();
    it->p2.normalize();
    it->p3.normalize();
    // XXX size known in advance
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
    pos_->setVertex(i, cfg.posScale * verts[i] );
    if(!nors.empty()) {
      nor_->setVertex(i, nors[i] );
    }
    if(!texcos.empty()) {
      texco_->setVertex(i, cfg.texcoScale * texcos[i] );
    }
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
      tan_->setVertex(i, Vec4f(t.x, t.y, t.z, 1.0) );
    }
  }

  begin(ShaderInputContainer::INTERLEAVED);
  setInput(pos_);
  if(!nors.empty())
    setInput(nor_);
  if(!texcos.empty())
    setInput(texco_);
  if(cfg.isTangentRequired)
    setInput(tan_);
  end();
}

///////////
///////////

SphereSprite::Config::Config()
: radius(NULL),
  position(NULL),
  sphereCount(0),
  usage(VBO::USAGE_DYNAMIC)
{
}

SphereSprite::SphereSprite(const Config &cfg)
: Mesh(GL_POINTS, cfg.usage), HasShader("regen.models.sprite-sphere")
{
  updateAttributes(cfg);
  joinStates(shaderState());
}

void SphereSprite::updateAttributes(const Config &cfg)
{
  ref_ptr<ShaderInput1f> radiusIn = ref_ptr<ShaderInput1f>::alloc("sphereRadius");
  radiusIn->setVertexData(cfg.sphereCount);

  ref_ptr<ShaderInput3f> positionIn = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
  positionIn->setVertexData(cfg.sphereCount);

  for(GLuint i=0; i<cfg.sphereCount; ++i) {
    radiusIn->setVertex(i, cfg.radius[i]);
    positionIn->setVertex(i, cfg.position[i]);
  }

  begin(ShaderInputContainer::INTERLEAVED);
  setInput(radiusIn);
  setInput(positionIn);
  end();
}

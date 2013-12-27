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

  GLuint faceCounter = 0;
  if(cfg.isHalfSphere) {
    // Count faces of half-sphere.
    for(GLuint faceIndex=0; faceIndex<faces->size(); ++faceIndex) {
      TriangleFace &face = (*faces)[faceIndex];
      Vec3f *f = (Vec3f*)&face;
      GLuint counter=0;
      for(GLuint i=0; i<3; ++i) {
        if(f[i].y>0) {
          counter += 1;
          f[i].y = 0.0;
        }
      }
      if(counter==3) {
        f[0].y = 1.0;
        continue;
      }
      faceCounter += 1;
    }
  } else {
    faceCounter = faces->size();
  }
  GLuint numVertices = faceCounter*3;

  // allocate attributes
  pos_->setVertexData(numVertices);
  if(cfg.isNormalRequired) {
    nor_->setVertexData(numVertices);
  }
  if(cfg.isTangentRequired) {
    tan_->setVertexData(numVertices);
  }

  TexcoMode texcoMode = cfg.texcoMode;
  if(cfg.isTangentRequired && cfg.texcoMode==TEXCO_MODE_NONE) {
    texcoMode = TEXCO_MODE_UV;
  }
  if(texcoMode == TEXCO_MODE_SPHERE_MAP) {
    texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
    texco_->setVertexData(numVertices);
  }
  else if(texcoMode == TEXCO_MODE_UV) {
    texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
    texco_->setVertexData(numVertices);
  }

  faceCounter = 0;
  // generate arrays of attribute data from faces
  for(GLuint faceIndex=0; faceIndex<faces->size(); ++faceIndex)
  {
    GLuint vertexIndex = faceCounter*3;
    TriangleFace &face = (*faces)[faceIndex];
    Vec3f *f = (Vec3f*)&face;

    if(cfg.isHalfSphere) {
      // Discard faces where all vertices are on the +y half sphere.
      GLuint flag=0;
      for(GLuint i=0; i<3; ++i) {
        if(f[i].y>0) {
          flag = 1;
          break;
        }
      }
      if(flag==1) {
        continue;
      }
    }
    faceCounter += 1;

    // Compute positions/normals
    for(GLuint i=0; i<3; ++i) {
      f[i].normalize();
      pos_->setVertex(vertexIndex+i, cfg.posScale * f[i]);
      if(cfg.isNormalRequired) {
        nor_->setVertex(vertexIndex+i, f[i]);
      }
    }

    // Compute texture coordinates
    if(texcoMode==TEXCO_MODE_UV) {
      Vec2f *texco = (Vec2f*) texco_->clientData();
      Vec3f *pos = (Vec3f*) pos_->clientData();
      for(GLuint i=0; i<3; ++i) {
        texco[vertexIndex+i] = Vec2f(
          0.5f + pos[vertexIndex+i].x/cfg.posScale.x,
          0.5f + pos[vertexIndex+i].y/cfg.posScale.y
        );
      }
    }
    else if(texcoMode==TEXCO_MODE_SPHERE_MAP) {
      Vec2f *texco = (Vec2f*) texco_->clientData();
      GLfloat s1, s2, s3, t1, t2, t3;

      sphereUV(f[0], &s1, &t1);
      sphereUV(f[1], &s2, &t2);
      if(s2 < 0.75 && s1 > 0.75) {
        s2 += 1.0;
      } else if(s2 > 0.75 && s1 < 0.75) {
        s2 -= 1.0;
      }

      sphereUV(f[2], &s3, &t3);
      if(s3 < 0.75 && s2 > 0.75) {
        s3 += 1.0;
      } else if(s3 > 0.75 && s2 < 0.75) {
        s3 -= 1.0;
      }

      texco[vertexIndex+0] = Vec2f(s1, t1);
      texco[vertexIndex+1] = Vec2f(s2, t2);
      texco[vertexIndex+2] = Vec2f(s3, t3);
    }

    if(cfg.isTangentRequired) {
      for(GLuint i=0; i<3; ++i) {
        const Vec3f &v = pos_->getVertex(vertexIndex+i);
        Vec3f vAbs = Vec3f(abs(v.x), abs(v.y), abs(v.z));
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
  }
  delete faces;

  begin(ShaderInputContainer::INTERLEAVED);
  setInput(pos_);
  if(cfg.isNormalRequired) {
    setInput(nor_);
  }
  if(cfg.isTangentRequired) {
    setInput(tan_);
  }
  if(texcoMode != TEXCO_MODE_NONE) {
    setInput(texco_);
  }
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

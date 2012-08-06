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

vector<SphereFace>* makeSphere(unsigned int levelOfDetail) {
  unsigned int numFaces_ = pow(4.0,(int)levelOfDetail)*8;
  vector<SphereFace> *faces = new vector<SphereFace>(numFaces_);
  vector<SphereFace> &f = *faces;
  unsigned int i, j, numFaces=0, numNewFaces;
  Vec3f pa,pb,pc;

  { // setup initial level
    float a = 1.0 / sqrt(2.0) + 0.001;
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

  for (j=0; j<levelOfDetail; ++j) {
    numNewFaces = numFaces;
    for (i=0; i<numNewFaces; ++i) {
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

static void sphereUV(const Vec3f &p, float *s, float *t)
{
  *s = atan2(p.x, p.z) / (2. * M_PI) + 0.5;
  *t = asin(p.y) / M_PI + 0.5;
}


Sphere::Sphere()
: AttributeState(GL_TRIANGLES)
{
}

void Sphere::createVertexData(
    unsigned int levelOfDetail,
    const Vec3f &scale,
    const Vec2f &uvScale,
    int uvMode)
{
  vector<SphereFace> *faces = makeSphere(levelOfDetail);
  float s1, s2, s3, t1, t2, t3;
  vector<Vec3f> verts;
  vector<Vec3f> nors;
  vector<Vec2f> uvs;

  vector< ref_ptr<VertexAttribute> > attributes;
  ref_ptr<VertexAttributefv> pos = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_POS ));
  ref_ptr<VertexAttributefv> nor = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_NOR ));
  ref_ptr<VertexAttributefv> uv0 = ref_ptr<VertexAttributefv>::manage(
      new UVAttribute( 0, 2 ));

  ref_ptr< vector<GLuint> > indexes;
  vector<MeshFace> primitiveFaces;
  vector<VertexAttributefv> atts;
  Vec3f v;
  int vertexIndex=0;

  for(vector<SphereFace>::iterator it = faces->begin();
      it != faces->end(); ++it)
  {
    verts.push_back( it->p1 * 0.5 );
    verts.push_back( it->p2 * 0.5 );
    verts.push_back( it->p3 * 0.5 );
    nors.push_back( it->p1 );
    nors.push_back( it->p2 );
    nors.push_back( it->p3 );

    // calculate uv coordinates for vertices
    if(uvMode==1) {
      uvs.push_back( Vec2f(
        0.5f + verts[vertexIndex].x/scale.x,
        0.5f + verts[vertexIndex].y/scale.y
      ) );
      uvs.push_back( Vec2f(
        0.5f + verts[vertexIndex+1].x/scale.x,
        0.5f + verts[vertexIndex+1].y/scale.y
      ) );
      uvs.push_back( Vec2f(
        0.5f + verts[vertexIndex+2].x/scale.x,
        0.5f + verts[vertexIndex+2].y/scale.y
      ) );
    } else if(uvMode==0) {
      sphereUV(it->p1, &s1, &t1);
      sphereUV(it->p2, &s2, &t2);
      if(s2 < 0.75 && s1 > 0.75) s2 += 1.0;
      else if(s2 > 0.75 && s1 < 0.75) s2 -= 1.0;
      sphereUV(it->p3, &s3, &t3);
      if(s3 < 0.75 && s2 > 0.75) s3 += 1.0;
      else if(s3 > 0.75 && s2 < 0.75) s3 -= 1.0;
      uvs.push_back( Vec2f(s1, t1) );
      uvs.push_back( Vec2f(s2, t2) );
      uvs.push_back( Vec2f(s3, t3) );
    }

    vertexIndex += 3;
  }
  delete faces;

  pos->setVertexData(vertexIndex);
  nor->setVertexData(vertexIndex);
  if(uvMode==0 || uvMode==1) uv0->setVertexData(vertexIndex);
  indexes = ref_ptr< vector<GLuint> >::manage(new vector<GLuint>(vertexIndex));

  for(unsigned int i=0; i<vertexIndex; ++i) {
    indexes->data()[i] = i;
    setAttributeVertex3f(pos.get(), i, scale * verts[i] );
    setAttributeVertex3f(nor.get(), i, nors[i] );
    if(uvMode==0 || uvMode==1)
      setAttributeVertex2f(uv0.get(), i, uvScale * uvs[i] );
  }
  primitiveFaces.push_back( (MeshFace){indexes} );

  setFaces(primitiveFaces, 3);
  setAttribute(pos);
  setAttribute(nor);
  if(uvMode==0 || uvMode==1) setAttribute(uv0);
}

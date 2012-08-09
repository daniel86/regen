/*
 * terrain.cpp
 *
 *  Created on: 11.04.2012
 *      Author: daniel
 */

#include "terrain.h"

#include <ogle/textures/image-texture.h>
#include <ogle/states/texture-state.h>


Terrain::Terrain(
    const Vec2f &size,
    const Vec2i &numPatched)
: AttributeState(GL_QUADS)
{
  vector<MeshFace> faces;
  ref_ptr< vector<GLuint> > indexes;
  ref_ptr<VertexAttributefv> pos = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_POS ));
  ref_ptr<VertexAttributefv> nor = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_NOR ));
  ref_ptr<VertexAttributefv> texco = ref_ptr<VertexAttributefv>::manage(new TexcoAttribute( 0, 2 ));
  unsigned int numQuads = numPatched.x*numPatched.y;
  float quadSizeX = size.x/numPatched.x;
  float quadSizeY = size.y/numPatched.y;
  float uvSizeX = quadSizeX/size.x;
  float uvSizeY = quadSizeY/size.y;
  unsigned int counter;

  if(glewIsSupported("GL_ARB_tessellation_shader")) {
    set_primitive(GL_PATCHES);
  }

  indexes = ref_ptr< vector<GLuint> >::manage(new vector<GLuint>(numQuads*4));
  pos->setVertexData(numQuads*4);
  nor->setVertexData(numQuads*4);
  texco->setVertexData(numQuads*4);

  for(unsigned int i=0; i<numQuads; ++i) {
    indexes->data()[i*4 + 0] = i*4 + 3;
    indexes->data()[i*4 + 1] = i*4 + 2;
    indexes->data()[i*4 + 2] = i*4 + 1;
    indexes->data()[i*4 + 3] = i*4 + 0;
  }
  faces.push_back( (MeshFace){indexes} );

  counter = 0;
  for(float x=0; x<numPatched.x; ++x) {
    for(float z=0; z<numPatched.y; ++z) {
      Vec3f quadOffset(
        (x-numPatched.x*0.5)*quadSizeX ,
        0.0,
        (z-numPatched.y*0.5)*quadSizeY );

      {
#define TRANSFORM(x) (x + quadOffset)
        setAttributeVertex3f(pos.get(), 4*counter + 0, TRANSFORM(Vec3f(0.0,0.0,0.0)));
        setAttributeVertex3f(pos.get(), 4*counter + 1, TRANSFORM(Vec3f(quadSizeX,0.0,0.0)));
        setAttributeVertex3f(pos.get(), 4*counter + 2, TRANSFORM(Vec3f(quadSizeX,0.0,quadSizeY)));
        setAttributeVertex3f(pos.get(), 4*counter + 3, TRANSFORM(Vec3f(0.0,0.0,quadSizeY)));
#undef TRANSFORM
      }
      {
#define TRANSFORM(x) x
        setAttributeVertex3f(nor.get(), 4*counter + 0, TRANSFORM(Vec3f(0.0,1.0,0.0)));
        setAttributeVertex3f(nor.get(), 4*counter + 1, TRANSFORM(Vec3f(0.0,1.0,0.0)));
        setAttributeVertex3f(nor.get(), 4*counter + 2, TRANSFORM(Vec3f(0.0,1.0,0.0)));
        setAttributeVertex3f(nor.get(), 4*counter + 3, TRANSFORM(Vec3f(0.0,1.0,0.0)));
#undef TRANSFORM
      }
      {
#define TRANSFORM(x) (x + quadUvOffset)
        Vec2f quadUvOffset(
            (quadOffset.z + numPatched.y*0.5*quadSizeX)/size.y,
            (quadOffset.x + numPatched.x*0.5*quadSizeX)/size.x
            );

        setAttributeVertex2f(texco.get(), 4*counter + 0, TRANSFORM(Vec2f(0, 0)));
        setAttributeVertex2f(texco.get(), 4*counter + 1, TRANSFORM(Vec2f(0, uvSizeY)));
        setAttributeVertex2f(texco.get(), 4*counter + 2, TRANSFORM(Vec2f(uvSizeX, uvSizeY)));
        setAttributeVertex2f(texco.get(), 4*counter + 3, TRANSFORM(Vec2f(uvSizeX, 0)));
#undef TRANSFORM
      }
      counter += 1;
    }
  }

  setFaces(faces, 4);
  setAttribute(pos);
  setAttribute(nor);
  setAttribute(texco);
}

ref_ptr<Texture> Terrain::loadNormalMap(
    const string &normalMapPath)
{
  normalMap_ = ref_ptr<Texture>::manage(new ImageTexture(normalMapPath));
  normalMap_->addMapTo(MAP_TO_NORMAL);
  normalMap_->set_wrapping(GL_MIRRORED_REPEAT);

  ref_ptr<State> texState = ref_ptr<State>::manage(
      new TextureState(normalMap_));
  joinStates( texState );

  return normalMap_;
}
ref_ptr<Texture> Terrain::normalMap()
{
  return normalMap_;
}

ref_ptr<Texture> Terrain::loadHeightMap(
    const string &heightMapPath)
{
  heightMap_ = ref_ptr<Texture>::manage(new ImageTexture(heightMapPath));
  heightMap_->addMapTo(MAP_TO_HEIGHT);
  heightMap_->set_wrapping(GL_MIRRORED_REPEAT);

  ref_ptr<State> texState = ref_ptr<State>::manage(
      new TextureState(heightMap_));
  joinStates( texState );

  return heightMap_;
}
ref_ptr<Texture> Terrain::heightMap()
{
  return heightMap_;
}

ref_ptr<Texture> Terrain::loadColorMap(
    const string &colorMapPath)
{
  colorMap_ = ref_ptr<Texture>::manage(new ImageTexture(colorMapPath));
  colorMap_->addMapTo(MAP_TO_COLOR);
  colorMap_->set_wrapping(GL_MIRRORED_REPEAT);

  ref_ptr<State> texState = ref_ptr<State>::manage(
      new TextureState(colorMap_));
  joinStates( texState );

  return colorMap_;
}
ref_ptr<Texture> Terrain::colorMap()
{
  return colorMap_;
}

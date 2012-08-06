/*
 * assimp-primitive-set.cpp
 *
 *  Created on: 24.10.2011
 *      Author: daniel
 */

#include "assimp-mesh.h"
#include "assimp-loader.h"

#include <ogle/utility/logging.h>
#include <ogle/utility/string-util.h>

///////////////////

AssimpMesh::AssimpMesh()
: AttributeState(GL_TRIANGLES)
{
}

void AssimpMesh::loadMesh(
    const struct aiNode &node,
    const struct aiMesh &mesh,
    struct aiNode &rootNode,
    ref_ptr<Bone> rootBoneNode,
    map< aiNode*, ref_ptr<Bone> > &nodeToNodeMap,
    aiMatrix4x4 &transform,
    vector< ref_ptr<Material> > &materials,
    const Vec3f &translation)
{
  GLenum lastPrimitive = GL_NONE;
  vector<MeshFace> faces;
  ref_ptr< vector<GLuint> > indexes = ref_ptr< vector<GLuint> >::manage(new vector<GLuint>());

  ref_ptr<VertexAttributefv> pos = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_POS ));
  ref_ptr<VertexAttributefv> nor = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_NOR ));
  ref_ptr<VertexAttributefv> tan = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( "tan" ));

  stringstream s;
  int t, n;

  DEBUG_LOG( "loading mesh " << mesh.mName.data << " " <<
      mesh.mNumFaces << " faces " <<
      mesh.mNumVertices << " vertices " <<
      " hasNormals=" << mesh.HasNormals() <<
      " hasTangents=" << mesh.HasTangentsAndBitangents() <<
      " hasBones=" << mesh.HasBones()
      );

  for (t = 0; t < mesh.mNumFaces; ++t) {
    const struct aiFace* face = &mesh.mFaces[t];
    GLenum primitive;

    switch(face->mNumIndices) {
    case 1: primitive_ = GL_POINTS; break;
    case 2: primitive_ = GL_LINES; break;
    case 3: primitive_ = GL_TRIANGLES; break;
    default: primitive_ = GL_POLYGON; break;
    }
    if(lastPrimitive != primitive_ && lastPrimitive != GL_NONE) {
      ERROR_LOG("mesh with different primitives added! aiProcess_SortByPType not working?");
      continue;
    }
    lastPrimitive = primitive_;

    for(n=0; n<face->mNumIndices; ++n) {
      indexes->push_back( face->mIndices[n] );
    }
  }
  GLuint numFaceIndices =0;
  if(mesh.mNumFaces>0) {
    numFaceIndices = mesh.mFaces[0].mNumIndices;
  }
  faces.push_back( (MeshFace){indexes} );
  setFaces(faces, numFaceIndices);

  // vertex positions
  unsigned int numVertices = mesh.mNumVertices;
  {
    // TODO ASSIMP: like this it might be possible to use without copy!
    // byte *bytes = (byte*) &mesh.mVertices[0].x;
    // pos->data = ref_ptr< vector<byte> >::manage(
    //     new vector<byte>( bytes, bytes+(numVertices*3*sizeof(float)) ) );

    pos->setVertexData(numVertices);
    for(n=0; n<numVertices; ++n) {
      aiVector3D aiv = transform * mesh.mVertices[n];
      Vec3f &v = *((Vec3f*) &aiv.x);
      setAttributeVertex3f(pos.get(), n, v + translation );
    }
    setAttribute(pos);
  }

  // per vertex normals
  if(mesh.HasNormals()) {
    nor->setVertexData(numVertices);
    for(n=0; n<numVertices; ++n) {
      Vec3f &v = *((Vec3f*) &mesh.mNormals[n].x);
      setAttributeVertex3f(nor.get(), n, v );
    }
    setAttribute(nor);
  }

  // per vertex colors
  for(t=0; t<AI_MAX_NUMBER_OF_COLOR_SETS; ++t) {
    if(mesh.mColors[t]==NULL) continue;

    ref_ptr<VertexAttributefv> col = ref_ptr<VertexAttributefv>::manage(
        new VertexAttributefv( FORMAT_STRING("col" << t) ));
    col->setVertexData(numVertices);

    Vec4f colVal;
    for(n=0; n<numVertices; ++n) {
      colVal = (Vec4f) {mesh.mColors[t][n].r, mesh.mColors[t][n].g,
        mesh.mColors[t][n].b, mesh.mColors[t][n].a};
      setAttributeVertex4f(col.get(), n, colVal );
    }
    setAttribute(col);
  }

  // uv coordinates
  for(t=0; t<AI_MAX_NUMBER_OF_TEXTURECOORDS; ++t) {
    if(mesh.mTextureCoords[t]==NULL) continue;

    ref_ptr<VertexAttributefv> uv = ref_ptr<VertexAttributefv>::manage(
        new UVAttribute( t, 2 ));
    uv->setVertexData(numVertices);

    for(n=0; n<numVertices; ++n) {
      setAttributeVertex2f(uv.get(), n, *((Vec2f*) &(mesh.mTextureCoords[t][n].x)) );
    }
    setAttribute(uv);
  }

  // tangents for normal mapping
  if(mesh.HasTangentsAndBitangents()) {
    tan->setVertexData(numVertices);
    for(n=0; n<numVertices; ++n) {
      setAttributeVertex3f(tan.get(), n, *((Vec3f*) &mesh.mTangents[n].x) );
    }
    setAttribute(tan);
  }

  // A mesh may have a set of bones in the form of aiBone structures..
  // Bones are a means to deform a mesh according to the movement of a skeleton.
  // Each bone has a name and a set of vertices on which it has influence.
  // Its offset matrix declares the transformation needed to transform from mesh space
  // to the local space of this bone.
  if(mesh.HasBones()) {
    vector< ref_ptr<Bone> > bones = vector< ref_ptr<Bone> >(mesh.mNumBones);
    map< GLuint,vector<GLfloat> > vertexToWeights;
    map< GLuint,vector<GLfloat> > vertexToIndices;

    for(unsigned int boneIndex=0; boneIndex<mesh.mNumBones; ++boneIndex)
    {
      aiBone *assimpBone = mesh.mBones[boneIndex];
      aiNode *assimpBoneNode = rootNode.FindNode(assimpBone->mName);
      ref_ptr<Bone> boneNode = nodeToNodeMap[assimpBoneNode];
      boneNode->set_offsetMatrix( *((Mat4f*) &assimpBone->mOffsetMatrix.a1) );
      bones[boneIndex] = boneNode;

      for(t=0; t<assimpBone->mNumWeights; ++t) {
        aiVertexWeight &weight = assimpBone->mWeights[t];
        vertexToWeights[weight.mVertexId].push_back(weight.mWeight);
        vertexToIndices[weight.mVertexId].push_back(boneIndex);
      }
    }

    unsigned int numWeights = vertexToWeights[0].size();

    if(numWeights > 4) {
      ERROR_LOG("The model has invalid bone weights number " << numWeights << ".");
    } else {
      ref_ptr<VertexAttributefv> boneWeights = ref_ptr<VertexAttributefv>::manage(
          new VertexAttributefv( "boneWeights", 4 ));
      boneWeights->setVertexData(numVertices);

      ref_ptr<VertexAttributeuiv> boneIndices = ref_ptr<VertexAttributeuiv>::manage(
          new VertexAttributeuiv( "boneIndices", 4 ));
      boneIndices->setVertexData(numVertices);

      for (int j = 0; j < numVertices; j++) {
        Vec4f weight = (Vec4f) {0.0f, 0.0f, 0.0f, 0.0f};
        Vec4ui indices = (Vec4ui) {0, 0, 0, 0};

        for(int k=0; k<min(4,(int)vertexToWeights[j].size()); ++k) {
          (&(weight.x))[k] = vertexToWeights[j][k];
          (&(indices.x))[k] = vertexToIndices[j][k];
        }

        setAttributeVertex4f(boneWeights.get(), j, weight );
        setAttributeVertex4ui(boneIndices.get(), j, indices );
      }
      setAttribute(boneWeights);
      setAttribute(boneIndices);

      // TODO: get bones state from assimp
      // setBones(rootBoneNode, bones);
    }
  }

  // set the material..
  if(mesh.mMaterialIndex>=0 && mesh.mMaterialIndex<materials.size()) {
    // TODO: get material state from assimp
    //material_->set( *(materials[mesh.mMaterialIndex].get()) );
  }
}

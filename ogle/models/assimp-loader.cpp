/*
 * assimp-wrapper.cpp
 *
 *  Created on: 24.10.2011
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include "assimp-loader.h"

#include <ogle/utility/logging.h>
#include <ogle/textures/image-texture.h>
#include <ogle/textures/video-texture.h>
#include <ogle/utility/string-util.h>

// TODO: assimp loader for new architechture !!!

class AssimpScene {
public:
  AssimpScene(const aiScene *s) : scene_(s) {}

  const struct aiScene *scene_;
  string texturePath_;
  ref_ptr<Bone> rootBoneNode_;
  map< aiNode*, ref_ptr<Bone> > aiBoneToBone_;
  map< string, aiNode* > nodes_;
};

map<AssimpScene*, unsigned int> AssimpLoader::sceneReferences_ = map<AssimpScene*, unsigned int>();

bool AssimpLoader::isInitialized_ = false;

void AssimpLoader::initializeAssimp()
{
  static struct aiLogStream stream;
  int n, k, l;

  if(!isInitialized_) {
    // get a handle to the predefined STDOUT log stream and attach
    // it to the logging system. It remains active for all further
    // calls to aiImportFile(Ex) and aiApplyPostProcessing.
    stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
    aiAttachLogStream(&stream);
    isInitialized_ = true;
  }
}

AssimpScene& AssimpLoader::loadScene(
    const string &modelPath,
    const string &texturePath,
    int assimpFlags)
throw(AssimpError,FileNotFoundException)
{
  if(access(modelPath.c_str(), F_OK) != 0)
    throw FileNotFoundException(FORMAT_STRING(
        "Unable to model file at '" << modelPath << "'."));

  const struct aiScene *scene = aiImportFile(modelPath.c_str(), 0
      | aiProcessPreset_TargetRealtime_MaxQuality
      | aiProcess_GenUVCoords // convert special texture coords to uv
      | aiProcess_JoinIdenticalVertices
      | aiProcess_SortByPType
      | assimpFlags);
  if(scene == NULL) {
    throw AssimpError(FORMAT_STRING("Can not import model file '" <<
        modelPath << "'. " << aiGetErrorString()));
  }
  AssimpScene *s = new AssimpScene(scene);
  s->texturePath_ = texturePath;
  s->scene_ = scene;

  if(scene->HasAnimations()) {
    bool hasBones = false;
    for(unsigned int i=0; i<scene->mNumAnimations; ++i) {
      if(scene->mAnimations[i]->mNumChannels>0) {
        hasBones = true;
        break;
      }
    }
    if(hasBones) {
      ref_ptr<Bone> nullBone;
      s->rootBoneNode_ = createNodeTree(*s, scene->mRootNode, nullBone);
    }
  }

  sceneReferences_[s] = 1;

  return *s;
}

ref_ptr<Bone> AssimpLoader::createNodeTree(
    AssimpScene &s,
    aiNode* assimpNode,
    ref_ptr<Bone> parent)
{
  ref_ptr<Bone> node = ref_ptr<Bone>::manage(
      new Bone( string(assimpNode->mName.data), parent ) );
  s.aiBoneToBone_[assimpNode] = node;
  s.nodes_[ string(assimpNode->mName.data) ] = assimpNode;

  node->set_localTransform( *((Mat4f*) &assimpNode->mTransformation.a1) );
  node->calculateGlobalTransform();

  // continue for all child nodes and assign the created internal nodes as our children
  for (unsigned int i=0; i<assimpNode->mNumChildren; ++i)
  {
    ref_ptr<Bone> child = createNodeTree(s, assimpNode->mChildren[i], node);
    node->addChild( child );
  }

  return node;
}

void AssimpLoader::deallocate(AssimpScene &scene)
{
  unsigned int numReferences = sceneReferences_[&scene]-1;
  if(numReferences==0) {
    sceneReferences_.erase(&scene);
    aiReleaseImport(scene.scene_);
    delete (&scene);
  } else {
    sceneReferences_[&scene] = numReferences;
  }
}

vector< ref_ptr<Light> > AssimpLoader::getLights(AssimpScene &s)
{
  vector< ref_ptr<Light> > ret;
  if(!s.scene_->HasLights()) return ret;

  for(unsigned int i=0; i<s.scene_->mNumLights; ++i) {
    aiLight *assimpLight = s.scene_->mLights[i];
    ref_ptr<Light> light = ref_ptr< Light >::manage(new Light);

    // node could be animated, but for now it is ignored
    // TODO ASSIMP: light animation
    aiNode *node = s.nodes_ [ string(assimpLight->mName.data) ];
    aiVector3D lightPos = node->mTransformation * assimpLight->mPosition;

    // Position of the light source in space.
    // The position is undefined for directional lights.
    light->set_position( Vec4f(
        lightPos.x, lightPos.y, lightPos.z,
        (assimpLight->mType == aiLightSource_DIRECTIONAL ? 0.0f : 1.0f)
    ));
    light->set_spotDirection( *((Vec3f*) &assimpLight->mDirection.x) );
    light->set_ambient( *((Vec4f*) &assimpLight->mColorAmbient) );
    light->set_diffuse( *((Vec4f*) &assimpLight->mColorDiffuse) );
    light->set_specular( *((Vec4f*) &assimpLight->mColorSpecular) );
    light->set_linearAttenuation( assimpLight->mAttenuationLinear );
    light->set_constantAttenuation( assimpLight->mAttenuationConstant );
    light->set_quadricAttenuation( assimpLight->mAttenuationQuadratic );
    light->set_outerConeAngle( assimpLight->mAngleOuterCone );
    light->set_innerConeAngle( assimpLight->mAngleInnerCone );
  }

  return ret;
}

static AnimationBehaviour animState(aiAnimBehaviour b)
{
  switch(b) {
  case _aiAnimBehaviour_Force32Bit:
  case aiAnimBehaviour_DEFAULT:
    return ANIM_BEHAVIOR_DEFAULT;
  case aiAnimBehaviour_CONSTANT:
    return ANIM_BEHAVIOR_CONSTANT;
  case aiAnimBehaviour_LINEAR:
    return ANIM_BEHAVIOR_LINEAR;
  case aiAnimBehaviour_REPEAT:
    return ANIM_BEHAVIOR_REPEAT;
  }
}

ref_ptr<BoneAnimation> AssimpLoader::getBoneAnimation(
    AssimpScene &s,
    list<AttributeState*> &sets,
    bool forceChannelStates,
    AnimationBehaviour forcedPostState,
    AnimationBehaviour forcedPreState,
    double defaultTicksPerSecond)
{
  ref_ptr<BoneAnimation> boneAnimation;

  if(!s.scene_->HasAnimations()) return boneAnimation;

  bool hasBones = false;
  for(unsigned int i=0; i<s.scene_->mNumAnimations; ++i) {
    if(s.scene_->mAnimations[i]->mNumChannels>0) {
      hasBones = true;
      break;
    }
  }
  if(!hasBones) return boneAnimation;

  boneAnimation = ref_ptr<BoneAnimation>::manage(
      new BoneAnimation(sets, s.rootBoneNode_) );

  for(unsigned int i=0; i<s.scene_->mNumAnimations; ++i) {
    aiAnimation *assimpAnim = s.scene_->mAnimations[i];

    DEBUG_LOG("load animation " <<
        assimpAnim->mName.data <<
        " mDuration=" << assimpAnim->mDuration <<
        " mTicksPerSecond=" << assimpAnim->mTicksPerSecond <<
        " mNumChannels=" << assimpAnim->mNumChannels <<
        " mNumMeshChannels=" << assimpAnim->mNumMeshChannels
        )

    if(assimpAnim->mNumChannels <= 0) continue;

    ref_ptr< vector< BoneAnimationChannel> > channels =
        ref_ptr< vector< BoneAnimationChannel> >::manage(
            new vector< BoneAnimationChannel>(assimpAnim->mNumChannels) );
    vector< BoneAnimationChannel> &channelsPtr = *channels.get();

    for(unsigned int j=0; j<assimpAnim->mNumChannels; ++j)
    {
      aiNodeAnim *nodeAnim = assimpAnim->mChannels[j];

      ref_ptr< vector< BoneScalingKey > > scalingKeys =
          ref_ptr< vector< BoneScalingKey > >::manage(
              new vector< BoneScalingKey >(nodeAnim->mNumScalingKeys));
      vector< BoneScalingKey > &scalingKeys_ = *scalingKeys.get();
      bool useScale = false;
      for(unsigned int k=0; k<nodeAnim->mNumScalingKeys; ++k) {
        BoneScalingKey &key = scalingKeys_[k];
        key.time = nodeAnim->mScalingKeys[k].mTime;
        key.value = *((Vec3f*)&(nodeAnim->mScalingKeys[k].mValue.x));
        if(key.time > 0.0001) useScale = true;
      }
      if(!useScale && scalingKeys_.size() > 0) {
        if( isApprox(scalingKeys_[0].value, (Vec3f) {1.0f, 1.0f, 1.0f}) ) {
          scalingKeys_.resize( 0 );
        } else {
          scalingKeys_.resize( 1, scalingKeys_[0] );
        }
      }

      ref_ptr< vector< BonePositionKey > > positionKeys =
          ref_ptr< vector< BonePositionKey > >::manage(
              new vector< BonePositionKey >(nodeAnim->mNumPositionKeys));
      vector< BonePositionKey > &positionKeys_ = *positionKeys.get();
      bool usePosition = false;
      for(unsigned int k=0; k<nodeAnim->mNumPositionKeys; ++k) {
        BonePositionKey &key = positionKeys_[k];
        key.time = nodeAnim->mPositionKeys[k].mTime;
        key.value = *((Vec3f*)&(nodeAnim->mPositionKeys[k].mValue.x));
        if(key.time > 0.0001) usePosition = true;
      }
      if(!usePosition && positionKeys_.size() > 0) {
        if( isApprox(positionKeys_[0].value, (Vec3f) {0.0f, 0.0f, 0.0f}) ) {
          positionKeys_.resize( 0 );
        } else {
          positionKeys_.resize( 1, positionKeys_[0] );
        }
      }

      ref_ptr< vector< BoneQuaternionKey > > rotationKeys =
          ref_ptr< vector< BoneQuaternionKey > >::manage(
              new vector< BoneQuaternionKey >(nodeAnim->mNumRotationKeys));
      vector< BoneQuaternionKey > &rotationKeyss_ = *rotationKeys.get();
      bool useRotation = false;
      for(unsigned int k=0; k<nodeAnim->mNumRotationKeys; ++k) {
        BoneQuaternionKey &key = rotationKeyss_[k];
        key.time = nodeAnim->mRotationKeys[k].mTime;
        key.value = *((Quaternion*)&(nodeAnim->mRotationKeys[k].mValue.w));
        if(key.time > 0.0001) useRotation = true;
      }
      if(!useRotation && rotationKeyss_.size() > 0) {
        if(rotationKeyss_[0].value == (Quaternion) {1, 0, 0, 0}) {
          rotationKeyss_.resize( 0 );
        } else {
          rotationKeyss_.resize( 1, rotationKeyss_[0] );
        }
      }

      BoneAnimationChannel &channel = channelsPtr[j];
      channel.nodeName_ = string(nodeAnim->mNodeName.data);
      if(forceChannelStates) {
        channel.postState = forcedPostState;
        channel.preState = forcedPreState;
      } else {
        channel.postState = animState( nodeAnim->mPostState );
        channel.preState = animState( nodeAnim->mPreState );
      }
      channel.scalingKeys_ = scalingKeys;
      channel.isScalingCompleted = (scalingKeys->size()<2);
      channel.positionKeys_ = positionKeys;
      channel.isPositionCompleted = (positionKeys->size()<2);
      channel.rotationKeys_ = rotationKeys;
      channel.isRotationCompleted = (rotationKeys->size()<2);
    }

    // extract ticks per second. Assume default value if not given
    double ticksPerSecond = (assimpAnim->mTicksPerSecond != 0.0 ?
        assimpAnim->mTicksPerSecond : defaultTicksPerSecond);

    boneAnimation->addChannels(
        string( assimpAnim->mName.data ),
        channels,
        assimpAnim->mDuration,
        ticksPerSecond
        );
  }

  return boneAnimation;
}

ref_ptr<Animation> AssimpLoader::getMeshAnimation(
    AssimpScene &s,
    list<AttributeState*> &meshes)
{
  ref_ptr<Animation> anim;

  for(unsigned int i=0; i<s.scene_->mNumAnimations; ++i)
  {
    aiAnimation *assimpAnim = s.scene_->mAnimations[i];
    if(assimpAnim->mNumMeshChannels <= 0) continue;

    WARN_LOG("ignoring animation with mesh animation channels.");
    // TODO ASSIMP: animation with mesh animation channels

    for(unsigned int j=0; j<assimpAnim->mNumMeshChannels; ++j) {
      // Describes vertex-based animations for a single mesh or a group of meshes.
      // Meshes carry the animation data for each frame in their aiMesh::mAnimMeshes array.
      // The purpose of aiMeshAnim is to define keyframes
      // linking each mesh attachment to a particular point in time.
      aiMeshAnim *meshAnim = assimpAnim->mMeshChannels[j];
      // Name of the mesh to be animated. An empty string is not allowed
      string meshName(meshAnim->mName.data);

      // Binds a anim mesh to a specific point in time.
      for(unsigned int keyIndex=0; keyIndex<meshAnim->mNumKeys; ++keyIndex)
      {
        aiMeshKey &key = meshAnim->mKeys[keyIndex];
        // The time of this key
        double time = key.mTime;
        // Index into the aiMesh::mAnimMeshes array of the mesh
        unsigned int animMeshIndex = key.mValue;
      }
    }
  }

  return anim;
}

list<AttributeState*> AssimpLoader::getPrimitiveSets(
    AssimpScene &s,
    vector< ref_ptr< Material > > &materials,
    const Vec3f &translation)
{
  aiMatrix4x4 transform;
  return getPrimitiveSets(s, materials,
      translation,
      *(s.scene_->mRootNode), transform);
}
list<AttributeState*> AssimpLoader::getPrimitiveSets(
    AssimpScene &s,
    vector< ref_ptr< Material > > &materials,
    const Vec3f &translation,
    const struct aiNode &node,
    aiMatrix4x4 transform)
{
  list<AttributeState*> models;
  AttributeState *model;
  unsigned int n;

  aiMatrix4x4 nodeTransform = node.mTransformation*transform;

  // walk through meshes, add primitive set for each mesh
  for (n=0; n < node.mNumMeshes; ++n) {
    const struct aiMesh* mesh = s.scene_->mMeshes[node.mMeshes[n]];
    if(mesh==NULL) continue;

    // create instanced or regular AssimpModel
    AssimpMesh* model = new AssimpMesh();
    model->loadMesh(node, *mesh,
        *s.scene_->mRootNode,
        s.rootBoneNode_, s.aiBoneToBone_,
        nodeTransform, materials, translation);
    models.push_back( model );
  }

  // same for all children
  for (n = 0; n < node.mNumChildren; ++n) {
    const struct aiNode *child = node.mChildren[n];
    if(child==NULL) continue;
    list<AttributeState*> childModels =
        AssimpLoader::getPrimitiveSets(s,
            materials,
            translation,
            *child,
            nodeTransform);
    models.insert( models.end(), childModels.begin(), childModels.end() );
  }

  return models;
}

static unsigned int numTextureTyps = 11;
static aiTextureType textureTypes[] = {
    aiTextureType_DIFFUSE, aiTextureType_SPECULAR,
    aiTextureType_AMBIENT, aiTextureType_EMISSIVE,
    aiTextureType_HEIGHT, aiTextureType_NORMALS,
    aiTextureType_SHININESS, aiTextureType_OPACITY,
    aiTextureType_DISPLACEMENT, aiTextureType_LIGHTMAP,
    aiTextureType_REFLECTION
};

static void loadTexture(
    AssimpScene &s,
    ref_ptr< Material > &mat, aiMaterial *aiMat,
    aiString &stringVal,
    GLuint l, GLuint k)
{
  ref_ptr<Texture> tex;
  string filePath = "";
  char proceduralType[60];
  int proceduralNum;
  unsigned int maxElements;
  int intVal;
  float floatVal;

  if(stringVal.data == NULL) return;

  if(sscanf(stringVal.data, "Procedural,num=%d,type=%s",
      &proceduralNum, proceduralType))
  {
    WARN_LOG("ignoring procedural texture " << stringVal.data);
    return;
  } else {
    if (access(stringVal.data, F_OK) == 0) {
      filePath = stringVal.data;
    } else {
      vector<string> names;
      filePath = stringVal.data;
      boost::split(names, filePath, boost::is_any_of("/\\"));
      filePath = names[ names.size()-1 ];

      string buf = FORMAT_STRING(s.texturePath_ << "/" << filePath);
      if(access(buf.c_str(), F_OK) == 0) {
        filePath = buf;
      } else {
        throw FileNotFoundException(FORMAT_STRING(
            "Unable to load texture '" << buf << "'."));
      }
    }
  }

  try {
    // try image texture
    tex = ref_ptr< Texture >::manage(new ImageTexture(filePath));
  } catch(ImageError ie) {
    // try video texture
    ref_ptr<VideoTexture> vid = ref_ptr<VideoTexture>::manage( new VideoTexture );
    try {
      vid->set_file(filePath);
      tex = vid;
    } catch(VideoError ve) {
      ERROR_LOG("Failed to load texture '" << stringVal.data << "'.");
    }
    return;
  }

  // Defines miscellaneous flag for the n'th texture on the stack 't'.
  // This is a bitwise combination of the aiTextureFlags enumerated values.
  maxElements = 1;
  if(AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
      AI_MATKEY_TEXFLAGS(textureTypes[l],k-1), &intVal, &maxElements))
  {
    if(intVal & aiTextureFlags_Invert) {
      tex->set_invert( true );
    }
    if(intVal & aiTextureFlags_UseAlpha) {
      tex->set_useAlpha(true);
    }
    if(intVal & aiTextureFlags_IgnoreAlpha) {
      tex->set_ignoreAlpha(true);
    }
  }

  // TODO: tex factor!
  if(textureTypes[l] == aiTextureType_HEIGHT ||
      textureTypes[l] == aiTextureType_NORMALS ||
      textureTypes[l] == aiTextureType_DISPLACEMENT)
  {
    // Defines the height scaling of a bump map (for stuff like Parallax Occlusion Mapping)
    maxElements = 1;
    if(aiGetMaterialFloatArray(aiMat, AI_MATKEY_BUMPSCALING,
        &floatVal, &maxElements) == AI_SUCCESS)
    {
      //tex->set_factor( floatVal );
    } else {
      //tex->set_factor( 1.0f );
    }
  } else {
    // Defines the strength the n'th texture on the stack 't'.
    // All color components (rgb) are multipled with this factor *before* any further processing is done.      -
    maxElements = 1;
    if(AI_SUCCESS == aiGetMaterialFloatArray(aiMat,
        AI_MATKEY_TEXBLEND(textureTypes[l],k-1), &floatVal, &maxElements))
    {
      //tex->set_factor(floatVal);
    }
  }

  // One of the aiTextureOp enumerated values. Defines the arithmetic operation to be used
  // to combine the n'th texture on the stack 't' with the n-1'th.
  // TEXOP(t,0) refers to the blend operation between the base color
  // for this stack (e.g. COLOR_DIFFUSE for the diffuse stack) and the first texture.
  maxElements = 1;
  if(AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
      AI_MATKEY_TEXOP(textureTypes[l],k-1), &intVal, &maxElements))
  {
    switch(intVal) {
    case aiTextureOp_Multiply:
      tex->set_blendMode(BLEND_MODE_MULTIPLY);
      break;
    case aiTextureOp_Add:
      tex->set_blendMode(BLEND_MODE_ADD);
      break;
    case aiTextureOp_Subtract:
      tex->set_blendMode(BLEND_MODE_SUBSTRACT);
      break;
    case aiTextureOp_Divide:
      tex->set_blendMode(BLEND_MODE_DIVIDE);
      break;
    case aiTextureOp_SmoothAdd:
      tex->set_blendMode(BLEND_MODE_SMOOTH_ADD);
      break;
    case aiTextureOp_SignedAdd:
      tex->set_blendMode(BLEND_MODE_SIGNED_ADD);
      break;
    }
  }

  // Defines the base axis to to compute the mapping coordinates for the n'th texture
  // on the stack 't' from. This is not required for UV-mapped textures.
  // For instance, if MAPPING(t,n) is aiTextureMapping_SPHERE,
  // U and V would map to longitude and latitude of a sphere around the given axis.
  // The axis is given in local mesh space.
  //TEXMAP_AXIS(t,n)        aiVector3D      n/a

  // Defines how the input mapping coordinates for sampling the n'th texture on the stack 't'
  // are computed. Usually explicit UV coordinates are provided, but some model file formats
  // might also be using basic shapes, such as spheres or cylinders, to project textures onto meshes.
  maxElements = 1;
  if(AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
      AI_MATKEY_MAPPING(textureTypes[l],k-1), &intVal, &maxElements))
  {
    switch(intVal) {
    case aiTextureMapping_UV:
      // The mapping coordinates are taken from an UV channel.
      tex->set_mapping(MAPPING_UV);
      break;
    case aiTextureMapping_SPHERE:
      tex->set_mapping(MAPPING_SPHERE);
      break;
    case aiTextureMapping_CYLINDER:
      tex->set_mapping(MAPPING_TUBE);
      break;
    case aiTextureMapping_BOX:
      tex->set_mapping(MAPPING_CUBE);
      break;
    case aiTextureMapping_PLANE:
      tex->set_mapping(MAPPING_FLAT);
      break;
    case aiTextureMapping_OTHER:
      break;
    }
  }

  // Defines the UV channel to be used as input mapping coordinates for sampling the
  // n'th texture on the stack 't'. All meshes assigned to this material share
  // the same UV channel setup     Presence of this key implies MAPPING(t,n) to be
  // aiTextureMapping_UV. See How to map UV channels to textures (MATKEY_UVWSRC) for more details.
  maxElements = 1;
  if(AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
      AI_MATKEY_UVWSRC(textureTypes[l],k-1), &intVal, &maxElements))
  {
    tex->set_uvChannel( intVal );
  }

  // Any of the aiTextureMapMode enumerated values. Defines the texture wrapping mode on the
  // x axis for sampling the n'th texture on the stack 't'.
  // 'Wrapping' occurs whenever UVs lie outside the 0..1 range.
  maxElements = 1;
  if(AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
      AI_MATKEY_MAPPINGMODE_U(textureTypes[l],k-1), &intVal, &maxElements))
  {
    switch(intVal) {
    case aiTextureMapMode_Wrap:
      tex->set_wrapping(GL_REPEAT);
      break;
    case aiTextureMapMode_Clamp:
      tex->set_wrapping(GL_CLAMP);
      break;
    case aiTextureMapMode_Decal:
      WARN_LOG("ignoring texture map mode decal.");
      break;
    case aiTextureMapMode_Mirror:
      tex->set_wrapping(GL_MIRRORED_REPEAT);
      break;
    }
  } else {
    tex->set_wrapping(GL_REPEAT);
  }
  // Wrap mode on the v axis. See MAPPINGMODE_U.
  maxElements = 1;
  if(AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
      AI_MATKEY_MAPPINGMODE_V(textureTypes[l],k-1), &intVal, &maxElements))
  {
    switch(intVal) {
    case aiTextureMapMode_Wrap:
      tex->set_wrappingV(GL_REPEAT);
      break;
    case aiTextureMapMode_Clamp:
      tex->set_wrappingV(GL_CLAMP);
      break;
    case aiTextureMapMode_Decal:
      WARN_LOG("ignoring texture map mode decal.");
      break;
    case aiTextureMapMode_Mirror:
      tex->set_wrappingV(GL_MIRRORED_REPEAT);
      break;
    }
  }
#ifdef AI_MATKEY_MAPPINGMODE_W
  // Wrap mode on the v axis. See MAPPINGMODE_U.
  maxElements = 1;
  if(AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
      AI_MATKEY_MAPPINGMODE_W(textureTypes[l],k-1), &intVal, &maxElements))
  {
    switch(intVal) {
    case aiTextureMapMode_Wrap:
      tex->set_wrappingW(GL_REPEAT);
      break;
    case aiTextureMapMode_Clamp:
      tex->set_wrappingW(GL_CLAMP);
      break;
    case aiTextureMapMode_Decal:
      WARN_LOG("ignoring texture map mode decal.");
      break;
    case aiTextureMapMode_Mirror:
      tex->set_wrappingW(GL_MIRRORED_REPEAT);
      break;
    }
  }
#endif

  switch(textureTypes[l]) {
  case aiTextureType_DIFFUSE:
    // The texture is combined with the result of the diffuse lighting equation.
    tex->addMapTo(MAP_TO_DIFFUSE);
    break;
  case aiTextureType_AMBIENT:
    // The texture is combined with the result of the ambient lighting equation.
    tex->addMapTo(MAP_TO_AMBIENT);
    break;
  case aiTextureType_SPECULAR:
    // The texture is combined with the result of the specular lighting equation.
    tex->addMapTo(MAP_TO_SPECULAR);
    break;
  case aiTextureType_SHININESS:
    // The texture defines the glossiness of the material.
    // The glossiness is in fact the exponent of the specular (phong) lighting equation.
    // Usually there is a conversion function defined to map the linear color values
    // in the texture to a suitable exponent. Have fun.
    tex->addMapTo(MAP_TO_SHININESS);
    break;
  case aiTextureType_EMISSIVE:
    // The texture is added to the result of the lighting calculation.
    tex->addMapTo(MAP_TO_EMISSION);
    break;
  case aiTextureType_OPACITY:
    // The texture defines per-pixel opacity.
    // Usually 'white' means opaque and 'black' means 'transparency'.
    // Or quite the opposite. Have fun.
    tex->addMapTo(MAP_TO_ALPHA);
    break;
  case aiTextureType_LIGHTMAP:
    // Lightmap texture (aka Ambient Occlusion). Both 'Lightmaps' and
    // dedicated 'ambient occlusion maps' are covered by this material property.
    // The texture contains a scaling value for the final color value of a pixel.
    // Its intensity is not affected by incoming light.
    tex->addMapTo(MAP_TO_LIGHT);
    break;
  case aiTextureType_REFLECTION:
    // Reflection texture. Contains the color of a perfect mirror reflection.
    // Rarely used, almost never for real-time applications.
    tex->addMapTo(MAP_TO_REFLECTION);
    break;
  case aiTextureType_DISPLACEMENT:
    // Displacement texture. The exact purpose and format is application-dependent.
    // Higher color values stand for higher vertex displacements.
    tex->addMapTo(MAP_TO_DISPLACEMENT);
    break;
  case aiTextureType_HEIGHT:
    // The texture is a height map. By convention, higher gray-scale values
    // stand for higher elevations from the base height.
    tex->addMapTo(MAP_TO_HEIGHT);
    break;
  case aiTextureType_NORMALS:
    // The texture is a (tangent space) normal-map.
    tex->addMapTo(MAP_TO_NORMAL);
    tex->set_isInTangentSpace(true);
    break;
  case aiTextureType_NONE:
    // Dummy value. No texture, but the value to be used as 'texture semantic'
    // (aiMaterialProperty::mSemantic) for all material properties *not* related to textures.
    break;
  case aiTextureType_UNKNOWN:
    // Unknown texture. A texture reference that does not match any of the definitions
    // above is considered to be 'unknown'. It is still imported, but is excluded
    // from any further postprocessing.
    break;
  }

  tex->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
  tex->setupMipmaps(GL_DONT_CARE);
  mat->addTexture(tex);

  DEBUG_LOG("loaded assimp texture " << *tex.get());
}

vector< ref_ptr<Material> > AssimpLoader::getMaterials(AssimpScene &s)
{
  vector< ref_ptr<Material> > materials(s.scene_->mNumMaterials);
  int n,l,k;

  for(n=0; n<s.scene_->mNumMaterials; ++n) {
    ref_ptr< Material > mat = ref_ptr< Material >::manage(new Material());
    materials[n] = mat;
    aiMaterial *aiMat = s.scene_->mMaterials[n];
    aiColor4D aiCol;
    float floatVal, floatVal2;
    int intVal;
    aiString stringVal;
    unsigned int maxElements;

    // load textures
    for(l=0; l<numTextureTyps; ++l) {
      k=0;
      while( AI_SUCCESS == aiGetMaterialString(aiMat,
          AI_MATKEY_TEXTURE(textureTypes[l],k),&stringVal) )
      {
        k+=1;
        loadTexture(s, mat, aiMat, stringVal, l, k);
      }
    }

    if(AI_SUCCESS == aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &aiCol)) {
      mat->set_diffuse( *((Vec4f*) &aiCol) );
    }
    if(AI_SUCCESS == aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_SPECULAR, &aiCol)) {
      mat->set_specular( *((Vec4f*) &aiCol) );
    }
    if(AI_SUCCESS == aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_AMBIENT, &aiCol)) {
      mat->set_ambient( *((Vec4f*) &aiCol) );
    }
    if(AI_SUCCESS == aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_EMISSIVE, &aiCol)) {
      mat->set_emission( *((Vec4f*) &aiCol) );
    }
    // Defines the transparent color of the material,
    // this is the color to be multiplied with the color of translucent light to
    // construct the final 'destination color' for a particular position in the screen buffer.
    if(AI_SUCCESS == aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_TRANSPARENT, &aiCol)) {
      // not supposed to be used like this but for now i think this is ok...
      mat->set_alpha( mat->alpha() * (aiCol.r + aiCol.g + aiCol.b)/3.0f );
    }

    maxElements = 1;
    // Defines the base shininess of the material
    // This is the exponent of the phong shading equation.
    if(aiGetMaterialFloatArray(aiMat, AI_MATKEY_SHININESS,
        &floatVal, &maxElements) == AI_SUCCESS)
    {
      mat->set_shininess( floatVal );
    }
    maxElements = 1;
    // Defines the strength of the specular highlight.
    // This is simply a multiplier to the specular color of a material
    if(aiGetMaterialFloatArray(aiMat, AI_MATKEY_SHININESS_STRENGTH,
        &floatVal, &maxElements) == AI_SUCCESS)
    {
      mat->set_shininessStrength( floatVal );
    }

    maxElements = 1;
    // Defines the base opacity of the material
    if(aiGetMaterialFloatArray(aiMat, AI_MATKEY_OPACITY,
        &floatVal, &maxElements) == AI_SUCCESS)
    {
      mat->set_alpha( mat->alpha() * floatVal );
    }

    // Index of refraction of the material. This is used by some shading models,
    // e.g. Cook-Torrance. The value is the ratio of the speed of light in a
    // vacuum to the speed of light in the material (always >= 1.0 in the real world).
    // Might be of interest for raytracing.
    maxElements = 1;
    if(aiGetMaterialFloatArray(aiMat, AI_MATKEY_REFRACTI,
        &floatVal, &maxElements) == AI_SUCCESS)
    {
      mat->set_refractionIndex(floatVal);
    }

#ifdef AI_MATKEY_COOK_TORRANCE_PARAM
    maxElements = 1;
    if(aiGetMaterialFloatArray(aiMat, AI_MATKEY_COOK_TORRANCE_PARAM,
        &floatVal, &maxElements) == AI_SUCCESS)
    {
      mat->set_cookTorranceParam( floatVal );
    }
#endif

#ifdef AI_MATKEY_ORENNAYAR_ROUGHNESS
    maxElements = 1;
    if(aiGetMaterialFloatArray(aiMat, AI_MATKEY_ORENNAYAR_ROUGHNESS,
        &floatVal, &maxElements) == AI_SUCCESS)
    {
      mat->set_roughness( floatVal );
    }
#endif

#ifdef AI_MATKEY_MINNAERT_DARKNESS
    maxElements = 1;
    if(aiGetMaterialFloatArray(aiMat, AI_MATKEY_MINNAERT_DARKNESS,
        &floatVal, &maxElements) == AI_SUCCESS)
    {
      mat->set_darkness( floatVal );
    }
#endif

    maxElements = 1;
    if(AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
        AI_MATKEY_ENABLE_WIREFRAME, &intVal, &maxElements))
    {
      mat->set_fillMode(intVal ? GL_LINE : GL_FILL);
    }
    else
    {
      mat->set_fillMode(GL_FILL);
    }
    maxElements = 1;
    if(AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
        AI_MATKEY_TWOSIDED, &intVal, &maxElements))
    {
      mat->set_twoSided(intVal ? true : false);
    }
    maxElements = 1;
    if(AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
        AI_MATKEY_SHADING_MODEL, &intVal, &maxElements))
    {
      switch(intVal){
      case aiShadingMode_Flat: // flat shading currently not supported
        WARN_LOG("Assimp model file contains mesh with unsupported flat shading. Reverting to Gourad shading.");
        // fall through
      case aiShadingMode_Gouraud:
        mat->set_shading(Material::GOURAD_SHADING);
        break;
      case aiShadingMode_Phong:
        mat->set_shading(Material::PHONG_SHADING);
        break;
      case aiShadingMode_Blinn:
        mat->set_shading(Material::BLINN_SHADING);
        break;
      case aiShadingMode_Toon:
        mat->set_shading(Material::TOON_SHADING);
        break;
      case aiShadingMode_OrenNayar:
        mat->set_shading(Material::ORENNAYER_SHADING);
        break;
      case aiShadingMode_Minnaert:
        mat->set_shading(Material::MINNAERT_SHADING);
        break;
      case aiShadingMode_CookTorrance:
        mat->set_shading(Material::COOKTORRANCE_SHADING);
        break;
      case aiShadingMode_NoShading:
        mat->set_shading(Material::NO_SHADING);
        break;
      case aiShadingMode_Fresnel:
        // TODO: ASSIMP: aiShadingMode_Fresnel
        // is it supposed to be a combination of Phong with view/normal dependend fresnel factor?
        // then what should be affected? diffuse/ambient/specular/reflection ?
        WARN_LOG("Assimp model file contains mesh with unsupported fresnel shading. Reverting to Phong shading.");
        mat->set_shading(Material::PHONG_SHADING);
        break;
      }
    }

    DEBUG_LOG("loaded assimp material " << *mat.get());

    maxElements = 1;
  }

  return materials;
}


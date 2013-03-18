/*
 * assimp-wrapper.cpp
 *
 *  Created on: 24.10.2011
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include <ogle/utility/logging.h>
#include <ogle/utility/string-util.h>
#include <ogle/textures/texture-loader.h>
#include <ogle/av/video-texture.h>
#include <ogle/animations/animation-manager.h>

#include "assimp-importer.h"
using namespace ogle;

static unsigned int numTextureTyps = 11;
static aiTextureType textureTypes[] = {
    aiTextureType_DIFFUSE, aiTextureType_SPECULAR,
    aiTextureType_AMBIENT, aiTextureType_EMISSIVE,
    aiTextureType_HEIGHT, aiTextureType_NORMALS,
    aiTextureType_SHININESS, aiTextureType_OPACITY,
    aiTextureType_DISPLACEMENT, aiTextureType_LIGHTMAP,
    aiTextureType_REFLECTION
};

static const struct aiScene* importFile(
    const string &assimpFile,
    GLint userSpecifiedFlags)
{
  // get a handle to the predefined STDOUT log stream and attach
  // it to the logging system. It remains active for all further
  // calls to aiImportFile(Ex) and aiApplyPostProcessing.
  static struct aiLogStream stream;
  static GLboolean isLoggingInitialled = false;
  if(!isLoggingInitialled) {
    stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
    aiAttachLogStream(&stream);
    isLoggingInitialled = true;
  }

  if(userSpecifiedFlags==-1) {
    return aiImportFile(assimpFile.c_str(),
        aiProcess_Triangulate
        | aiProcess_RemoveRedundantMaterials
        | aiProcess_GenSmoothNormals
        | aiProcess_CalcTangentSpace
        | aiProcess_ImproveCacheLocality
        | aiProcess_Triangulate
        | aiProcess_GenUVCoords // convert special texture coords to uv
        | aiProcess_JoinIdenticalVertices
        | aiProcess_FlipUVs
        | aiProcess_SortByPType
        | 0);
  } else {
    return aiImportFile(assimpFile.c_str(),
        userSpecifiedFlags | 0);
  }
}

static Vec3f& aiToOgle(aiColor3D *v) { return *((Vec3f*)v); }
static Vec3f& aiToOgle3f(aiColor4D *v) { return *((Vec3f*)v); }

AssimpImporter::AssimpImporter(
    const string &assimpFile,
    const string &texturePath,
    GLint userSpecifiedFlags)
: scene_(importFile(assimpFile, userSpecifiedFlags)),
  texturePath_(texturePath)
{
  if(scene_ == NULL) {
    throw Error(FORMAT_STRING("Can not import assimp file '" <<
        assimpFile << "'. " << aiGetErrorString()));
  }

  rootNode_ = loadNodeTree();
  lights_ = loadLights();
  materials_ = loadMaterials();
}

AssimpImporter::~AssimpImporter()
{
  if(scene_ != NULL) {
    aiReleaseImport(scene_);
  }
}

list< ref_ptr<Light> >& AssimpImporter::lights()
{
  return lights_;
}

vector< ref_ptr<Material> >& AssimpImporter::materials()
{
  return materials_;
}

///////////// LIGHTS

static void setLightRadius(aiLight *aiLight, ref_ptr<Light> &light)
{
  GLfloat ax = aiLight->mAttenuationLinear;
  GLfloat ay = aiLight->mAttenuationConstant;
  GLfloat az = aiLight->mAttenuationQuadratic;
  GLfloat z = ay/(2.0*az);

  GLfloat start = 0.01;
  GLfloat stop = 0.99;

  GLfloat inner = -z + sqrt(z*z - (ax/start - 1.0/(start*az)));
  GLfloat outer = -z + sqrt(z*z - (ax/stop - 1.0/(stop*az)));

  light->radius()->setVertex2f(0, Vec2f(inner,outer));
}

list< ref_ptr<Light> > AssimpImporter::loadLights()
{
  list< ref_ptr<Light> > ret;

  for(GLuint i=0; i<scene_->mNumLights; ++i)
  {
    aiLight *assimpLight = scene_->mLights[i];
    // node could be animated, but for now it is ignored
    aiNode *node = nodes_[string(assimpLight->mName.data)];
    aiVector3D lightPos = node->mTransformation * assimpLight->mPosition;

    ref_ptr<Light> light;
    switch(assimpLight->mType) {
    case aiLightSource_DIRECTIONAL: {
      light = ref_ptr<Light>::manage(new Light(Light::DIRECTIONAL));
      light->direction()->setVertex3f(0, *((Vec3f*) &lightPos.x));
      break;
    }
    case aiLightSource_POINT: {
      light = ref_ptr<Light>::manage(new Light(Light::POINT));
      light->position()->setVertex3f(0, *((Vec3f*) &lightPos.x));
      setLightRadius(assimpLight, light);
      break;
    }
    case aiLightSource_SPOT: {
      light = ref_ptr<Light>::manage(new Light(Light::SPOT));
      light->position()->setVertex3f(0, *((Vec3f*) &lightPos.x));
      light->direction()->setVertex3f(0, *((Vec3f*) &assimpLight->mDirection.x) );
      light->set_outerConeAngle(
          acos( assimpLight->mAngleOuterCone )*360.0/(2.0*M_PI) );
      light->set_innerConeAngle(
          acos( assimpLight->mAngleInnerCone )*360.0/(2.0*M_PI) );
      setLightRadius(assimpLight, light);
      break;
    }
    case aiLightSource_UNDEFINED:
    case _aiLightSource_Force32Bit:
      break;
    }
    if(light.get()==NULL) { continue; }

    lightToAiLight_[light.get()] = assimpLight;
    //light->set_ambient( aiToOgle(&assimpLight->mColorAmbient) );
    light->diffuse()->setVertex3f(0, aiToOgle(&assimpLight->mColorDiffuse) );
    light->specular()->setVertex3f(0, aiToOgle(&assimpLight->mColorSpecular) );

    ret.push_back(light);
  }

  return ret;
}

ref_ptr<LightNode> AssimpImporter::loadLightNode(const ref_ptr<Light> &light)
{
  aiLight *assimpLight = lightToAiLight_[light.get()];
  if(assimpLight==NULL) { return ref_ptr<LightNode>(); }

  aiNode *node = nodes_[string(assimpLight->mName.data)];
  if(node==NULL) { return ref_ptr<LightNode>(); }

  ref_ptr<AnimationNode> &animNode = aiNodeToNode_[node];
  if(animNode.get()==NULL) { return ref_ptr<LightNode>(); }

  return ref_ptr<LightNode>::manage(new LightNode(light, animNode));
}

///////////// TEXTURES

static void loadTexture(
    ref_ptr< Material > &mat,
    aiMaterial *aiMat,
    aiString &stringVal,
    GLuint l, GLuint k,
    const string &texturePath)
{
  ref_ptr<Texture> tex;
  string filePath = "";
  GLchar proceduralType[60];
  GLint proceduralNum;
  GLuint maxElements;
  GLint intVal;
  GLfloat floatVal;

  if(stringVal.data == NULL) { return; }

  if(sscanf(stringVal.data, "Procedural,num=%d,type=%s",
      &proceduralNum, proceduralType))
  {
    WARN_LOG("ignoring procedural texture " << stringVal.data);
    return;
  }
  else
  {
    if (access(stringVal.data, F_OK) == 0) {
      filePath = stringVal.data;
    } else {
      vector<string> names;
      filePath = stringVal.data;
      boost::split(names, filePath, boost::is_any_of("/\\"));
      filePath = names[ names.size()-1 ];

      string buf = FORMAT_STRING(texturePath << "/" << filePath);
      if(access(buf.c_str(), F_OK) == 0) {
        filePath = buf;
      } else {
        throw AssimpImporter::Error(FORMAT_STRING(
            "Unable to load texture '" << buf << "'."));
      }
    }
  }

  try
  {
    // try image texture
    tex = TextureLoader::load(filePath);
  }
  catch(TextureLoader::Error ie)
  {
    // try video texture
    ref_ptr<VideoTexture> vid = ref_ptr<VideoTexture>::manage( new VideoTexture );
    try
    {
      vid->set_file(filePath);
      tex = ref_ptr<Texture>::cast(vid);
      AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(vid));
    }
    catch(VideoTexture::Error ve)
    {
      ERROR_LOG("Failed to load texture '" << stringVal.data << "'.");
    }
    return;
  }

  ref_ptr<TextureState> texState =
      ref_ptr<TextureState>::manage(new TextureState(tex));

  // Defines miscellaneous flag for the n'th texture on the stack 't'.
  // This is a bitwise combination of the aiTextureFlags enumerated values.
  maxElements = 1;
  if(AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
      AI_MATKEY_TEXFLAGS(textureTypes[l],k-1), &intVal, &maxElements))
  {
    if(intVal & aiTextureFlags_Invert)
    {
      texState->set_texelTransferFunction(
          "void transferInvert(inout vec4 texel) {\n"
          "    texel.rgb = vec3(1.0) - texel.rgb;\n"
          "}",
          "transferInvert"
      );
    }
    if(intVal & aiTextureFlags_UseAlpha)
    {}
    if(intVal & aiTextureFlags_IgnoreAlpha)
    {  texState->set_ignoreAlpha(GL_TRUE); }
  }

  // Defines the height scaling of a bump map (for stuff like Parallax Occlusion Mapping)
  maxElements = 1;
  if(aiGetMaterialFloatArray(aiMat, AI_MATKEY_BUMPSCALING,
      &floatVal, &maxElements) == AI_SUCCESS)
  {
    texState->set_texelTransferFunction(FORMAT_STRING(
        "void transferFactor(inout vec4 texel) {\n"
        "    texel.rgb = " << floatVal << " * texel.rgb;\n"
        "}"),
        "transferFactor"
    );
  }

  // Defines the strength the n'th texture on the stack 't'.
  // All color components (rgb) are multipled with this factor *before* any further processing is done.      -
  maxElements = 1;
  if(AI_SUCCESS == aiGetMaterialFloatArray(aiMat,
      AI_MATKEY_TEXBLEND(textureTypes[l],k-1), &floatVal, &maxElements))
  {
    texState->set_blendFactor(floatVal);
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
      texState->set_blendMode(BLEND_MODE_MULTIPLY);
      break;
    case aiTextureOp_SignedAdd:
    case aiTextureOp_Add:
      texState->set_blendMode(BLEND_MODE_ADD);
      break;
    case aiTextureOp_Subtract:
      texState->set_blendMode(BLEND_MODE_SUBSTRACT);
      break;
    case aiTextureOp_Divide:
      texState->set_blendMode(BLEND_MODE_DIVIDE);
      break;
    case aiTextureOp_SmoothAdd:
      texState->set_blendMode(BLEND_MODE_SMOOTH_ADD);
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
      texState->set_mapping(TextureState::MAPPING_TEXCO);
      break;
    case aiTextureMapping_SPHERE:
      texState->set_mapping(TextureState::MAPPING_SPHERE);
      break;
    case aiTextureMapping_CYLINDER:
      texState->set_mapping(TextureState::MAPPING_TUBE);
      break;
    case aiTextureMapping_BOX:
      texState->set_mapping(TextureState::MAPPING_CUBE);
      break;
    case aiTextureMapping_PLANE:
      texState->set_mapping(TextureState::MAPPING_FLAT);
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
    texState->set_texcoChannel( intVal );
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
    texState->set_mapTo(TextureState::MAP_TO_DIFFUSE);
    break;
  case aiTextureType_AMBIENT:
    // The texture is combined with the result of the ambient lighting equation.
    texState->set_mapTo(TextureState::MAP_TO_AMBIENT);
    break;
  case aiTextureType_SPECULAR:
    // The texture is combined with the result of the specular lighting equation.
    texState->set_mapTo(TextureState::MAP_TO_SPECULAR);
    break;
  case aiTextureType_SHININESS:
    // The texture defines the glossiness of the material.
    // The glossiness is in fact the exponent of the specular (phong) lighting equation.
    // Usually there is a conversion function defined to map the linear color values
    // in the texture to a suitable exponent. Have fun.
    texState->set_mapTo(TextureState::MAP_TO_SHININESS);
    break;
  case aiTextureType_EMISSIVE:
    // The texture is added to the result of the lighting calculation.
    texState->set_mapTo(TextureState::MAP_TO_EMISSION);
    break;
  case aiTextureType_OPACITY:
    // The texture defines per-pixel opacity.
    // Usually 'white' means opaque and 'black' means 'transparency'.
    // Or quite the opposite. Have fun.
    texState->set_mapTo(TextureState::MAP_TO_ALPHA);
    break;
  case aiTextureType_LIGHTMAP:
    // Lightmap texture (aka Ambient Occlusion). Both 'Lightmaps' and
    // dedicated 'ambient occlusion maps' are covered by this material property.
    // The texture contains a scaling value for the final color value of a pixel.
    // Its intensity is not affected by incoming light.
    texState->set_mapTo(TextureState::MAP_TO_LIGHT);
    break;
  case aiTextureType_REFLECTION:
    // Reflection texture. Contains the color of a perfect mirror reflection.
    // Rarely used, almost never for real-time applications.
    //texState->setMapTo(MAP_TO_REFLECTION);
    break;
  case aiTextureType_DISPLACEMENT:
    // Displacement texture. The exact purpose and format is application-dependent.
    // Higher color values stand for higher vertex displacements.
    texState->set_mapTo(TextureState::MAP_TO_DISPLACEMENT);
    break;
  case aiTextureType_HEIGHT:
    // The texture is a height map. By convention, higher gray-scale values
    // stand for higher elevations from the base height.
    texState->set_mapTo(TextureState::MAP_TO_HEIGHT);
    break;
  case aiTextureType_NORMALS:
    // The texture is a (tangent space) normal-map.
    texState->set_mapTo(TextureState::MAP_TO_NORMAL);
    break;
  case aiTextureType_NONE:
    // Dummy value. No texture, but the value to be used as 'texture semantic'
    // (aiMaterialProperty::mSemantic) for all material properties *not* related to textures.
    break;
  default:
  case aiTextureType_UNKNOWN:
    // Unknown texture. A texture reference that does not match any of the definitions
    // above is considered to be 'unknown'. It is still imported, but is excluded
    // from any further postprocessing.
    break;
  }

  tex->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
  tex->setupMipmaps(GL_DONT_CARE);
  mat->joinStates(ref_ptr<State>::cast(texState));
}

///////////// MATERIAL

vector< ref_ptr<Material> > AssimpImporter::loadMaterials()
{
  vector< ref_ptr<Material> > materials(scene_->mNumMaterials);
  aiColor4D aiCol;
  GLfloat floatVal;
  GLint intVal;
  aiString stringVal;
  GLuint maxElements;
  GLuint l,k;

  for(GLuint n=0; n<scene_->mNumMaterials; ++n)
  {
    ref_ptr< Material > mat = ref_ptr< Material >::manage(new Material());
    materials[n] = mat;
    aiMaterial *aiMat = scene_->mMaterials[n];

    // load textures
    for(l=0; l<numTextureTyps; ++l)
    {
      k=0;
      while( AI_SUCCESS == aiGetMaterialString(aiMat,
          AI_MATKEY_TEXTURE(textureTypes[l],k),&stringVal) )
      {
        k+=1;
        loadTexture(mat, aiMat, stringVal, l, k, texturePath_);
      }
    }

    if(AI_SUCCESS == aiGetMaterialColor(aiMat,
        AI_MATKEY_COLOR_DIFFUSE, &aiCol))
    {
      mat->diffuse()->setUniformData( aiToOgle3f(&aiCol) );
    }
    if(AI_SUCCESS == aiGetMaterialColor(aiMat,
        AI_MATKEY_COLOR_SPECULAR, &aiCol))
    {
      mat->specular()->setUniformData( aiToOgle3f(&aiCol) );
    }
    if(AI_SUCCESS == aiGetMaterialColor(aiMat,
        AI_MATKEY_COLOR_AMBIENT, &aiCol))
    {
      mat->ambient()->setUniformData( aiToOgle3f(&aiCol) );
    }
    if(AI_SUCCESS == aiGetMaterialColor(aiMat,
        AI_MATKEY_COLOR_EMISSIVE, &aiCol))
    {
      //mat->set_emission( *((Vec3f*) &aiCol) );
    }
    // Defines the transparent color of the material,
    // this is the color to be multiplied with the color of translucent light to
    // construct the final 'destination color' for a particular position in the screen buffer.
    if(AI_SUCCESS == aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_TRANSPARENT, &aiCol)) {
      // not supposed to be used like this but for now i think this is ok...
      mat->alpha()->setUniformData( mat->alpha()->getVertex1f(0) * (aiCol.r + aiCol.g + aiCol.b)/3.0f );
    }

    maxElements = 1;
    // Defines the base shininess of the material
    // This is the exponent of the phong shading equation.
    if(aiGetMaterialFloatArray(aiMat, AI_MATKEY_SHININESS,
        &floatVal, &maxElements) == AI_SUCCESS)
    {
      mat->shininess()->setUniformData( floatVal );
    }
    maxElements = 1;
    // Defines the strength of the specular highlight.
    // This is simply a multiplier to the specular color of a material
    if(aiGetMaterialFloatArray(aiMat, AI_MATKEY_SHININESS_STRENGTH,
        &floatVal, &maxElements) == AI_SUCCESS)
    {
      //mat->set_shininessStrength( floatVal );
    }

    maxElements = 1;
    // Defines the base opacity of the material
    if(aiGetMaterialFloatArray(aiMat, AI_MATKEY_OPACITY,
        &floatVal, &maxElements) == AI_SUCCESS)
    {
      mat->alpha()->setUniformData( mat->alpha()->getVertex1f(0) * floatVal );
    }

    // Index of refraction of the material. This is used by some shading models,
    // e.g. Cook-Torrance. The value is the ratio of the speed of light in a
    // vacuum to the speed of light in the material (always >= 1.0 in the real world).
    // Might be of interest for raytracing.
    maxElements = 1;
    if(aiGetMaterialFloatArray(aiMat, AI_MATKEY_REFRACTI,
        &floatVal, &maxElements) == AI_SUCCESS)
    {
      mat->refractionIndex()->setUniformData(floatVal);
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
      mat->set_twoSided(intVal ? GL_TRUE : GL_FALSE);
    }

    maxElements = 1;
  }

  return materials;
}

///////////// MESHES

list< ref_ptr<Mesh> > AssimpImporter::loadMeshes(
    const aiMatrix4x4 &transform)
{
  return loadMeshes(*(scene_->mRootNode), transform);
}
list< ref_ptr<Mesh> > AssimpImporter::loadMeshes(
    const struct aiNode &node,
    const aiMatrix4x4 &transform)
{
  list< ref_ptr<Mesh> > meshes;

  // walk through meshes, add primitive set for each mesh
  for (GLuint n=0; n < node.mNumMeshes; ++n)
  {
    const struct aiMesh* mesh = scene_->mMeshes[node.mMeshes[n]];
    if(mesh==NULL) { continue; }

    ref_ptr<Mesh> meshState = loadMesh(*mesh, transform*node.mTransformation);
    meshes.push_back(meshState);
    // remember mesh material
    meshMaterials_[meshState.get()] = materials_[mesh->mMaterialIndex];
    meshToAiMesh_[meshState.get()] = mesh;
  }

  // same for all children
  for (GLuint n = 0; n < node.mNumChildren; ++n)
  {
    const struct aiNode *child = node.mChildren[n];
    if(child==NULL) { continue; }

    list< ref_ptr<Mesh> > childModels = AssimpImporter::loadMeshes(*child, transform);
    meshes.insert( meshes.end(), childModels.begin(), childModels.end() );
  }

  return meshes;
}

ref_ptr<Mesh> AssimpImporter::loadMesh(
    const struct aiMesh &mesh,
    const aiMatrix4x4 &transform)
{
  ref_ptr<Mesh> meshState = ref_ptr<Mesh>::manage(new Mesh(GL_TRIANGLES));
  stringstream s;

  ref_ptr<ShaderInput3f> pos =
      ref_ptr<ShaderInput3f>::manage(new ShaderInput3f(ATTRIBUTE_NAME_POS));
  ref_ptr<ShaderInput3f> nor =
      ref_ptr<ShaderInput3f>::manage(new ShaderInput3f(ATTRIBUTE_NAME_NOR));
  ref_ptr<ShaderInput4f> tan =
      ref_ptr<ShaderInput4f>::manage(new ShaderInput4f(ATTRIBUTE_NAME_TAN));

  const GLuint numFaceIndices = (mesh.mNumFaces>0 ? mesh.mFaces[0].mNumIndices : 0);
  GLuint numFaces = 0;
  for (GLuint t = 0u; t < mesh.mNumFaces; ++t)
  {
    const struct aiFace* face = &mesh.mFaces[t];
    if(face->mNumIndices != numFaceIndices) { continue; }
    numFaces += 1;
  }
  const GLuint numIndices = numFaceIndices*numFaces;

  switch(numFaceIndices) {
  case 1: meshState->set_primitive( GL_POINTS ); break;
  case 2: meshState->set_primitive( GL_LINES ); break;
  case 3: meshState->set_primitive( GL_TRIANGLES ); break;
  default: meshState->set_primitive( GL_POLYGON ); break;
  }

  {
    ref_ptr<VertexAttribute> indices = ref_ptr<VertexAttribute>::manage(
        new VertexAttribute("i", GL_UNSIGNED_INT, sizeof(GLuint), 1, 1, GL_FALSE));
    indices->setVertexData(numIndices);
    GLuint *faceIndices = (GLuint*)indices->dataPtr();
    GLuint index = 0, maxIndex=0;
    for (GLuint t = 0u; t < mesh.mNumFaces; ++t)
    {
      const struct aiFace* face = &mesh.mFaces[t];
      if(face->mNumIndices != numFaceIndices) { continue; }
      for(GLuint n=0; n<face->mNumIndices; ++n)
      {
        faceIndices[index] = face->mIndices[n];
        if(face->mIndices[n] > maxIndex)
        { maxIndex = face->mIndices[n]; }
        index += 1;
      }
    }
    meshState->setIndices(indices, maxIndex);
  }

  // vertex positions
  GLuint numVertices = mesh.mNumVertices;
  {
    pos->setVertexData(numVertices);
    for(GLuint n=0; n<numVertices; ++n)
    {
      aiVector3D aiv = transform * mesh.mVertices[n];
      pos->setVertex3f(n, *((Vec3f*) &aiv.x));
    }
    meshState->setInput(ref_ptr<ShaderInput>::cast(pos));
  }

  // per vertex normals
  if(mesh.HasNormals())
  {
    nor->setVertexData(numVertices);
    for(GLuint n=0; n<numVertices; ++n)
    {
      Vec3f &v = *((Vec3f*) &mesh.mNormals[n].x);
      nor->setVertex3f(n, v);
    }
    meshState->setInput(ref_ptr<ShaderInput>::cast(nor));
  }

  // per vertex colors
  for(GLuint t=0; t<AI_MAX_NUMBER_OF_COLOR_SETS; ++t)
  {
    if(mesh.mColors[t]==NULL) continue;

    ref_ptr<ShaderInput4f> col = ref_ptr<ShaderInput4f>::manage(
        new ShaderInput4f( FORMAT_STRING("col" << t) ));
    col->setVertexData(numVertices);

    Vec4f colVal;
    for(GLuint n=0; n<numVertices; ++n)
    {
      colVal = Vec4f(
          mesh.mColors[t][n].r,
          mesh.mColors[t][n].g,
          mesh.mColors[t][n].b,
          mesh.mColors[t][n].a);
      col->setVertex4f(n, colVal );
    }
    meshState->setInput(ref_ptr<ShaderInput>::cast(col));
  }

  // load texture coordinates
  for(GLuint t=0; t<AI_MAX_NUMBER_OF_TEXTURECOORDS; ++t)
  {
    if(mesh.mTextureCoords[t]==NULL) { continue; }
    aiVector3D *aiTexcos = mesh.mTextureCoords[t];
    GLuint texcoComponents = mesh.mNumUVComponents[t];
    string texcoName = FORMAT_STRING("texco"<<t);

    ref_ptr<ShaderInput> texco;
    if(texcoComponents==1) {
      texco = ref_ptr<ShaderInput>::manage(new ShaderInput1f(texcoName));
    }
    else if(texcoComponents==3) {
      texco = ref_ptr<ShaderInput>::manage(new ShaderInput3f(texcoName));
    }
    else if(texcoComponents==4) {
      texco = ref_ptr<ShaderInput>::manage(new ShaderInput4f(texcoName));
    }
    else {
      texco = ref_ptr<ShaderInput>::manage(new ShaderInput2f(texcoName));
    }
    texco->setVertexData(numVertices);
    GLfloat *texcoDataPtr = (GLfloat*) texco->dataPtr();

    for(GLuint n=0; n<numVertices; ++n)
    {
      GLfloat *aiTexcoData =  &(aiTexcos[n].x);
      for(GLuint x=0; x<texcoComponents; ++x) texcoDataPtr[x] = aiTexcoData[x];
      texcoDataPtr += texcoComponents;
    }

    meshState->setInput(ref_ptr<ShaderInput>::cast(texco));
  }

  // load tangents
  if(mesh.HasTangentsAndBitangents())
  {
    tan->setVertexData(numVertices);
    for(GLuint i=0; i<numVertices; ++i)
    {
      Vec3f &t = *((Vec3f*) &mesh.mTangents[i].x);
      Vec3f &b = *((Vec3f*) &mesh.mBitangents[i].x);
      Vec3f &n = *((Vec3f*) &mesh.mNormals[i].x);
      // Calculate the handedness of the local tangent space.
      GLfloat handeness;
      if( n.cross(t).dot(b) < 0.0) {
        handeness = -1.0;
      } else {
        handeness = 1.0;
      }
      tan->setVertex4f(i, Vec4f(t.x, t.y, t.z, handeness) );
    }
    meshState->setInput(ref_ptr<ShaderInput>::cast(tan));
  }

  // A mesh may have a set of bones in the form of aiBone structures..
  // Bones are a means to deform a mesh according to the movement of a skeleton.
  // Each bone has a name and a set of vertices on which it has influence.
  // Its offset matrix declares the transformation needed to transform from mesh space
  // to the local space of this bone.
  if(mesh.HasBones())
  {
    typedef list< pair<GLfloat,GLuint> > WeightList;
    map< GLuint, WeightList > vertexToWeights;
    GLuint maxNumWeights = 0;

    // collect weights at vertices
    for(GLuint boneIndex=0; boneIndex<mesh.mNumBones; ++boneIndex)
    {
      aiBone *assimpBone = mesh.mBones[boneIndex];
      for(GLuint t=0; t<assimpBone->mNumWeights; ++t)
      {
        aiVertexWeight &weight = assimpBone->mWeights[t];
        vertexToWeights[weight.mVertexId].push_back(
            pair<GLfloat,GLuint>(weight.mWeight,boneIndex));
        maxNumWeights = max(maxNumWeights,
            (GLuint)vertexToWeights[weight.mVertexId].size());
      }
    }

    // each vertex has maxNumWeights weight and matrix index tuples.
    // the matrix index is converted to float so that the data can be packed
    // in a single buffer.
    GLuint boneDataSize = 2*numVertices*maxNumWeights;
    GLfloat *boneData = new GLfloat[boneDataSize];
    GLfloat *boneDataPtr = boneData;
    for (GLuint j=0; j<numVertices; j++)
    {
      WeightList &vWeights = vertexToWeights[j];
      GLuint k=0;
      for(WeightList::iterator it=vWeights.begin(); it!=vWeights.end(); ++it)
      {
        *boneDataPtr = it->first; boneDataPtr += 1;
        *boneDataPtr = (GLfloat) it->second; boneDataPtr += 1;
        ++k;
      }
      for (;k<maxNumWeights; ++k)
      {
        *boneDataPtr = 0.0f; boneDataPtr += 1;
        *boneDataPtr = 0.0f; boneDataPtr += 1;
      }
    }

    // create VBO containing the data
    GLuint bufferSize = boneDataSize*sizeof(GLfloat);
    ref_ptr<VertexBufferObject> boneDataVBO = ref_ptr<VertexBufferObject>::manage(
        new VertexBufferObject(VertexBufferObject::USAGE_STATIC, bufferSize));
    boneDataVBO->set_data(bufferSize, boneData);
    // create TBO with data attached
    ref_ptr<TextureBufferObject> boneDataTBO =
        ref_ptr<TextureBufferObject>::manage(new TextureBufferObject(GL_RG32F));
    boneDataTBO->bind();
    boneDataTBO->attach(boneDataVBO);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    // bind TBO
    ref_ptr<TextureState> boneDataState = ref_ptr<TextureState>::manage(
        new TextureState(ref_ptr<Texture>::cast(boneDataTBO), "boneVertexData"));
    boneDataState->set_mapping(TextureState::MAPPING_CUSTOM);
    boneDataState->set_mapTo(TextureState::MAP_TO_CUSTOM);
    meshState->joinStates(ref_ptr<State>::cast(boneDataState));

    delete []boneData;
  }
  return ref_ptr<Mesh>::cast(meshState);
}

list< ref_ptr<AnimationNode> > AssimpImporter::loadMeshBones(
    Mesh *meshState, NodeAnimation *anim)
{
  const struct aiMesh *mesh = meshToAiMesh_[meshState];
  if(mesh->mNumBones==0) { return list< ref_ptr<AnimationNode> >(); }

  list< ref_ptr<AnimationNode> > boneNodes;
  for(GLuint boneIndex=0; boneIndex<mesh->mNumBones; ++boneIndex)
  {
    aiBone *assimpBone = mesh->mBones[boneIndex];
    string nodeName = string(assimpBone->mName.data);
    ref_ptr<AnimationNode> animNode = anim->findNode(nodeName);
    // hoping that meshes do not share bones here....
    // is there a usecase for bone sharing between meshes ?
    // if so the offset matrix can only be used in BonesState class
    // and not in the bone class.
    animNode->set_boneOffsetMatrix( *((Mat4f*) &assimpBone->mOffsetMatrix.a1) );
    boneNodes.push_back( animNode );
  }
  return boneNodes;
}

GLuint AssimpImporter::numBoneWeights(Mesh *meshState)
{
  const struct aiMesh *mesh = meshToAiMesh_[meshState];
  if(mesh->mNumBones==0) { return 0; }

  GLuint counter[meshState->numVertices()];
  GLuint numWeights=1;
  for(GLuint i=0; i<meshState->numVertices(); ++i) counter[i]=0u;
  for(GLuint boneIndex=0; boneIndex<mesh->mNumBones; ++boneIndex)
  {
    aiBone *assimpBone = mesh->mBones[boneIndex];
    for(GLuint t=0; t<assimpBone->mNumWeights; ++t)
    {
      aiVertexWeight &weight = assimpBone->mWeights[t];
      counter[weight.mVertexId] += 1;
      numWeights = max(numWeights, counter[weight.mVertexId]);
    }
  }
  return numWeights;
}

ref_ptr<Material> AssimpImporter::getMeshMaterial(Mesh *state)
{
  return meshMaterials_[state];
}

///////////// NODE ANIMATION

static NodeAnimation::Behavior animState(aiAnimBehaviour b)
{
  switch(b) {
  case aiAnimBehaviour_CONSTANT:
    return NodeAnimation::BEHAVIOR_CONSTANT;
  case aiAnimBehaviour_LINEAR:
    return NodeAnimation::BEHAVIOR_LINEAR;
  case aiAnimBehaviour_REPEAT:
    return NodeAnimation::BEHAVIOR_REPEAT;
  case _aiAnimBehaviour_Force32Bit:
  case aiAnimBehaviour_DEFAULT:
  default:
    return NodeAnimation::BEHAVIOR_DEFAULT;
  }
}

ref_ptr<AnimationNode> AssimpImporter::loadNodeTree()
{
  if(scene_->HasAnimations()) {
    GLboolean hasAnimations = false;
    for(GLuint i=0; i<scene_->mNumAnimations; ++i) {
      if(scene_->mAnimations[i]->mNumChannels>0) {
        hasAnimations = true;
        break;
      }
    }
    if(hasAnimations) {
      return loadNodeTree(scene_->mRootNode, ref_ptr<AnimationNode>());
    }
  }
  return ref_ptr<AnimationNode>();
}

ref_ptr<AnimationNode> AssimpImporter::loadNodeTree(aiNode* assimpNode, ref_ptr<AnimationNode> parent)
{
  ref_ptr<AnimationNode> node = ref_ptr<AnimationNode>::manage(
      new AnimationNode( string(assimpNode->mName.data), parent ) );
  aiNodeToNode_[assimpNode] = node;
  nodes_[ string(assimpNode->mName.data) ] = assimpNode;

  node->set_localTransform( *((Mat4f*) &assimpNode->mTransformation.a1) );
  node->calculateGlobalTransform();

  // continue for all child nodes and assign the created internal nodes as our children
  for (GLuint i=0; i<assimpNode->mNumChildren; ++i)
  {
    ref_ptr<AnimationNode> subTree = loadNodeTree(assimpNode->mChildren[i], node);
    node->addChild( subTree );
  }

  return node;
}

ref_ptr<NodeAnimation> AssimpImporter::loadNodeAnimation(
    GLboolean forceChannelStates,
    NodeAnimation::Behavior forcedPostState,
    NodeAnimation::Behavior forcedPreState,
    GLdouble defaultTicksPerSecond)
{
  if(!rootNode_.get())
  {
    return ref_ptr<NodeAnimation>();
  }

  ref_ptr<NodeAnimation> nodeAnimation = ref_ptr<NodeAnimation>::manage(
      new NodeAnimation(rootNode_) );

  ref_ptr< vector< NodeAnimation::Channel> > channels;
  ref_ptr< vector< NodeAnimation::KeyFrame3f > > scalingKeys;
  ref_ptr< vector< NodeAnimation::KeyFrame3f > > positionKeys;
  ref_ptr< vector< NodeAnimation::KeyFrameQuaternion > > rotationKeys;

  for(GLuint i=0; i<scene_->mNumAnimations; ++i)
  {
    aiAnimation *assimpAnim = scene_->mAnimations[i];

    if(assimpAnim->mNumChannels <= 0)
    {
      continue;
    }

    channels = ref_ptr< vector< NodeAnimation::Channel> >::manage(
            new vector< NodeAnimation::Channel>(assimpAnim->mNumChannels) );
    vector< NodeAnimation::Channel> &channelsPtr = *channels.get();

    for(GLuint j=0; j<assimpAnim->mNumChannels; ++j)
    {
      aiNodeAnim *nodeAnim = assimpAnim->mChannels[j];

      ref_ptr< vector< NodeAnimation::KeyFrame3f > > scalingKeys =
          ref_ptr< vector< NodeAnimation::KeyFrame3f > >::manage(
              new vector< NodeAnimation::KeyFrame3f >(nodeAnim->mNumScalingKeys));
      vector< NodeAnimation::KeyFrame3f > &scalingKeys_ = *scalingKeys.get();
      GLboolean useScale = false;
      for(GLuint k=0; k<nodeAnim->mNumScalingKeys; ++k)
      {
        NodeAnimation::KeyFrame3f &key = scalingKeys_[k];
        key.time = nodeAnim->mScalingKeys[k].mTime;
        key.value = *((Vec3f*)&(nodeAnim->mScalingKeys[k].mValue.x));
        if(key.time > 0.0001) useScale = true;
      }

      if(!useScale && scalingKeys_.size() > 0)
      {
        if(scalingKeys_[0].value.isApprox(Vec3f(1.0f)))
        {
          scalingKeys_.resize( 0 );
        }
        else
        {
          scalingKeys_.resize( 1, scalingKeys_[0] );
        }
      }

      ////////////

      positionKeys = ref_ptr< vector< NodeAnimation::KeyFrame3f > >::manage(
              new vector< NodeAnimation::KeyFrame3f >(nodeAnim->mNumPositionKeys));
      vector< NodeAnimation::KeyFrame3f > &positionKeys_ = *positionKeys.get();
      GLboolean usePosition = false;

      for(GLuint k=0; k<nodeAnim->mNumPositionKeys; ++k)
      {
        NodeAnimation::KeyFrame3f &key = positionKeys_[k];
        key.time = nodeAnim->mPositionKeys[k].mTime;
        key.value = *((Vec3f*)&(nodeAnim->mPositionKeys[k].mValue.x));
        if(key.time > 0.0001) usePosition = true;
      }

      if(!usePosition && positionKeys_.size() > 0)
      {
        if(positionKeys_[0].value.isApprox(Vec3f(0.0f)))
        {
          positionKeys_.resize( 0 );
        }
        else
        {
          positionKeys_.resize( 1, positionKeys_[0] );
        }
      }

      ///////////

      rotationKeys = ref_ptr< vector< NodeAnimation::KeyFrameQuaternion > >::manage(
              new vector< NodeAnimation::KeyFrameQuaternion >(nodeAnim->mNumRotationKeys));
      vector< NodeAnimation::KeyFrameQuaternion > &rotationKeyss_ = *rotationKeys.get();
      GLboolean useRotation = false;
      for(GLuint k=0; k<nodeAnim->mNumRotationKeys; ++k)
      {
        NodeAnimation::KeyFrameQuaternion &key = rotationKeyss_[k];
        key.time = nodeAnim->mRotationKeys[k].mTime;
        key.value = *((Quaternion*)&(nodeAnim->mRotationKeys[k].mValue.w));
        if(key.time > 0.0001) useRotation = true;
      }

      if(!useRotation && rotationKeyss_.size() > 0)
      {
        if(rotationKeyss_[0].value == Quaternion(1, 0, 0, 0))
        {
          rotationKeyss_.resize( 0 );
        }
        else
        {
          rotationKeyss_.resize( 1, rotationKeyss_[0] );
        }
      }

      NodeAnimation::Channel &channel = channelsPtr[j];
      channel.nodeName_ = string(nodeAnim->mNodeName.data);
      if(forceChannelStates) {
        channel.postState = forcedPostState;
        channel.preState = forcedPreState;
      } else {
        channel.postState = animState( nodeAnim->mPostState );
        channel.preState = animState( nodeAnim->mPreState );
      }
      channel.scalingKeys_ = scalingKeys;
      channel.positionKeys_ = positionKeys;
      channel.rotationKeys_ = rotationKeys;
    }

    // extract ticks per second. Assume default value if not given
    GLdouble ticksPerSecond = (assimpAnim->mTicksPerSecond != 0.0 ?
        assimpAnim->mTicksPerSecond : defaultTicksPerSecond);

    nodeAnimation->addChannels(
        string( assimpAnim->mName.data ),
        channels,
        assimpAnim->mDuration,
        ticksPerSecond
        );
  }

  return nodeAnimation;
}

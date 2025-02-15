/*
 * assimp-wrapper.cpp
 *
 *  Created on: 24.10.2011
 *      Author: daniel
 */

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <regen/utility/logging.h>
#include <regen/utility/string-util.h>
#include <regen/textures/texture-loader.h>
#include <regen/av/video-texture.h>
#include <regen/animations/animation-manager.h>
#include <regen/config.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <IL/il.h>
#include <IL/ilu.h>

#include "assimp-importer.h"

using namespace regen;
using namespace std;

static unsigned int numTextureTyps = 11;
static aiTextureType textureTypes[] = {
		aiTextureType_DIFFUSE, aiTextureType_SPECULAR,
		aiTextureType_AMBIENT, aiTextureType_EMISSIVE,
		aiTextureType_HEIGHT, aiTextureType_NORMALS,
		aiTextureType_SHININESS, aiTextureType_OPACITY,
		aiTextureType_DISPLACEMENT, aiTextureType_LIGHTMAP,
		aiTextureType_REFLECTION
};

static bool assimpLog_(std::string &msg, const std::string &prefix) {
	if (hasPrefix(msg, prefix)) {
		msg = truncPrefix(msg, prefix);
		msg[msg.size() - 1] = '0';
		return true;
	} else {
		return false;
	}
}

static void assimpLog(const char *msg_, char *) {
	string msg(msg_);
	if (assimpLog_(msg, "Info,")) { REGEN_INFO(msg); }
	else if (assimpLog_(msg, "Warn,")) { REGEN_WARN(msg); }
	else if (assimpLog_(msg, "Error,")) { REGEN_ERROR(msg); }
	else if (assimpLog_(msg, "Debug,")) { REGEN_DEBUG(msg); }
	else
		REGEN_DEBUG(msg);
}

static const struct aiScene *importFile(
		const string &assimpFile,
		GLint userSpecifiedFlags) {
	// get a handle to the predefined STDOUT log stream and attach
	// it to the logging system. It remains active for all further
	// calls to aiImportFile(Ex) and aiApplyPostProcessing.
	static struct aiLogStream stream;
	static GLboolean isLoggingInitialled = false;
	if (!isLoggingInitialled) {
		stream.callback = assimpLog;
		stream.user = nullptr;
		aiDetachAllLogStreams();
		aiAttachLogStream(&stream);
		isLoggingInitialled = true;
	}

	if (userSpecifiedFlags == -1) {
		return aiImportFile(assimpFile.c_str(),
							aiProcess_Triangulate
							// Convert special texture coords to UV
							| aiProcess_GenUVCoords
							| aiProcess_CalcTangentSpace
							| aiProcess_FlipUVs
							| aiProcess_SortByPType
							// Reorders triangles for better vertex cache locality.
							| aiProcess_ImproveCacheLocality
							// Searches for redundant/unreferenced materials and removes them.
							| aiProcess_RemoveRedundantMaterials
							// A postprocessing step to reduce the number of meshes.
							| aiProcess_OptimizeMeshes
							// A postprocessing step to optimize the scene hierarchy.
							| aiProcess_OptimizeGraph
							// If this flag is not specified,
							// no vertices are referenced by more than one face
							| aiProcess_JoinIdenticalVertices
							| 0);
	} else {
		return aiImportFile(assimpFile.c_str(),
							userSpecifiedFlags | 0);
	}
}

static Vec3f &aiToOgle(aiColor3D *v) { return *((Vec3f *) v); }

static Vec3f &aiToOgle3f(aiColor4D *v) { return *((Vec3f *) v); }

AssetImporter::AssetImporter(
		const string &assimpFile,
		const string &texturePath,
		const AssimpAnimationConfig &animConfig,
		GLint userSpecifiedFlags)
		: scene_(importFile(assimpFile, userSpecifiedFlags)),
		  texturePath_(texturePath) {
	if (scene_ == nullptr) {
		throw Error(REGEN_STRING("Can not import assimp file '" <<
																assimpFile << "'. " << aiGetErrorString()));
	}
	if (texturePath_.empty()) {
		boost::filesystem::path p(assimpFile);
		texturePath_ = p.parent_path().filename().string();
	}

	rootNode_ = loadNodeTree();
	lights_ = loadLights();
	materials_ = loadMaterials();
	loadNodeAnimation(animConfig);
	GL_ERROR_LOG();
}

AssetImporter::~AssetImporter() {
	if (scene_ != nullptr) {
		aiReleaseImport(scene_);
	}
}

vector<ref_ptr<Light> > &AssetImporter::lights() { return lights_; }

vector<ref_ptr<Material> > &AssetImporter::materials() { return materials_; }

///////////// LIGHTS

static void setLightRadius(aiLight *aiLight, ref_ptr<Light> &light) {
	GLfloat ax = aiLight->mAttenuationLinear;
	GLfloat ay = aiLight->mAttenuationConstant;
	GLfloat az = aiLight->mAttenuationQuadratic;
	GLfloat z = ay / (2.0 * az);

	GLfloat start = 0.01;
	GLfloat stop = 0.99;

	GLfloat inner = -z + sqrt(z * z - (ax / start - 1.0 / (start * az)));
	GLfloat outer = -z + sqrt(z * z - (ax / stop - 1.0 / (stop * az)));

	light->radius()->setVertex(0, Vec2f(inner, outer));
}

vector<ref_ptr<Light> > AssetImporter::loadLights() {
	vector<ref_ptr<Light> > ret(scene_->mNumLights);

	for (GLuint i = 0; i < scene_->mNumLights; ++i) {
		aiLight *assimpLight = scene_->mLights[i];
		// node could be animated, but for now it is ignored
		aiNode *node = nodes_[string(assimpLight->mName.data)];
		aiVector3D lightPos = node->mTransformation * assimpLight->mPosition;

		ref_ptr<Light> light;
		switch (assimpLight->mType) {
			case aiLightSource_DIRECTIONAL: {
				light = ref_ptr<Light>::alloc(Light::DIRECTIONAL);
				light->direction()->setVertex(0, *((Vec3f *) &lightPos.x));
				break;
			}
			case aiLightSource_POINT: {
				light = ref_ptr<Light>::alloc(Light::POINT);
				light->position()->setVertex(0, *((Vec3f *) &lightPos.x));
				setLightRadius(assimpLight, light);
				break;
			}
			case aiLightSource_SPOT: {
				light = ref_ptr<Light>::alloc(Light::SPOT);
				light->position()->setVertex(0, *((Vec3f *) &lightPos.x));
				light->direction()->setVertex(0, *((Vec3f *) &assimpLight->mDirection.x));
				light->set_outerConeAngle(
						acos(assimpLight->mAngleOuterCone) * 360.0 / (2.0 * M_PI));
				light->set_innerConeAngle(
						acos(assimpLight->mAngleInnerCone) * 360.0 / (2.0 * M_PI));
				light->startAnimation();
				setLightRadius(assimpLight, light);
				break;
			}
			case aiLightSource_AREA:
				REGEN_WARN("Area lights are not supported.");
				break;
			case aiLightSource_AMBIENT:
			case aiLightSource_UNDEFINED:
			case _aiLightSource_Force32Bit:
				break;
		}
		if (light.get() == nullptr) { continue; }

		lightToAiLight_[light.get()] = assimpLight;
		//light->set_ambient( aiToOgle(&assimpLight->mColorAmbient) );
		light->diffuse()->setVertex(0, aiToOgle(&assimpLight->mColorDiffuse));
		light->specular()->setVertex(0, aiToOgle(&assimpLight->mColorSpecular));

		ret[i] = light;
	}

	return ret;
}

ref_ptr<LightNode> AssetImporter::loadLightNode(const ref_ptr<Light> &light) {
	aiLight *assimpLight = lightToAiLight_[light.get()];
	if (assimpLight == nullptr) { return {}; }

	aiNode *node = nodes_[string(assimpLight->mName.data)];
	if (node == nullptr) { return {}; }

	ref_ptr<AnimationNode> &animNode = aiNodeToNode_[node];
	if (animNode.get() == nullptr) { return {}; }

	return ref_ptr<LightNode>::alloc(light, animNode);
}

///////////// TEXTURES

static void loadTexture(
		ref_ptr<Material> &mat,
		aiTexture *aiTexture,
		aiMaterial *aiMat,
		aiString &stringVal,
		GLuint l, GLuint k,
		const string &texturePath) {
	ref_ptr<Texture> tex;
	string filePath = "";
	GLuint maxElements;
	GLint intVal;
	GLfloat floatVal;

	if (aiTexture == nullptr) {
		if (boost::filesystem::exists(stringVal.data)) {
			filePath = stringVal.data;
		} else {
			vector<string> names;
			filePath = stringVal.data;
			boost::split(names, filePath, boost::is_any_of("/\\"));
			filePath = names[names.size() - 1];

			string buf = REGEN_STRING(texturePath << "/" << filePath);
			if (boost::filesystem::exists(buf)) {
				filePath = buf;
			} else {
				REGEN_ERROR("Unable to load texture '" << buf << "'.");
				return;
			}
		}

		try {
			// try image texture
			tex = textures::load(filePath);
		}
		catch (textures::Error &ie) {
			// try video texture
			ref_ptr<VideoTexture> vid = ref_ptr<VideoTexture>::alloc();
			try {
				vid->set_file(filePath);
				tex = vid;
				vid->startAnimation();
			}
			catch (VideoTexture::Error &ve) {
				REGEN_ERROR("Failed to load texture '" << stringVal.data << "'.");
				return;
			}
		}
	} else if (aiTexture->mHeight == 0) {
		// The texture is stored in a "compressed" format such as DDS or PNG

		GLuint numBytes = aiTexture->mWidth;
		string ext(aiTexture->achFormatHint);

		ILenum type;
		if (ext == "jpg" || ext == ".j")
			type = IL_JPG;
		else if (ext == "png")
			type = IL_PNG;
		else if (ext == "tga")
			type = IL_TGA;
		else if (ext == "bmp")
			type = IL_BMP;
		else if (ext == "dds")
			type = IL_DDS;
		else if (ext == "gif")
			type = IL_GIF;
		else if (ext == "tif")
			type = IL_TIF;
		else {
			REGEN_ERROR("Unknown texture type '" << ext << "'.");
			return;
		}

		try {
			tex = textures::load(type, numBytes, aiTexture->pcData);
		}
		catch (textures::Error &ie) {
			REGEN_ERROR("Failed to load texture '" << stringVal.data << "'.");
			return;
		}
	} else { // The texture is NOT compressed
		tex = ref_ptr<Texture2D>::alloc();
		tex->begin(RenderState::get());
		tex->set_rectangleSize(aiTexture->mWidth, aiTexture->mHeight);
		tex->set_textureData((GLubyte *) aiTexture->pcData);
		tex->set_pixelType(GL_UNSIGNED_BYTE);
		tex->set_format(GL_RGBA);
		tex->set_internalFormat(GL_RGBA8);
		tex->filter().push(GL_LINEAR);
		tex->wrapping().push(GL_REPEAT);
		tex->texImage();
		tex->end(RenderState::get());
		tex->set_textureData(nullptr);
	}

	ref_ptr<TextureState> texState = ref_ptr<TextureState>::alloc(tex);
	tex->begin(RenderState::get());

	// Defines miscellaneous flag for the n'th texture on the stack 't'.
	// This is a bitwise combination of the aiTextureFlags enumerated values.
	maxElements = 1;
	if (AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
												AI_MATKEY_TEXFLAGS(textureTypes[l], k - 1), &intVal, &maxElements)) {
		if (intVal & aiTextureFlags_Invert) {
			texState->set_texelTransferKey("regen.states.textures.transfer.texel_invert");
		}
		if (intVal & aiTextureFlags_UseAlpha) {
			REGEN_WARN("aiTextureFlags_UseAlpha is not supported.");
		}
		if (intVal & aiTextureFlags_IgnoreAlpha) {
			texState->set_ignoreAlpha(GL_TRUE);
		}
	}

	// Defines the height scaling of a bump map (for stuff like Parallax Occlusion Mapping)
	maxElements = 1;
	if (aiGetMaterialFloatArray(aiMat, AI_MATKEY_BUMPSCALING,
								&floatVal, &maxElements) == AI_SUCCESS) {
		texState->set_blendFactor(floatVal);
	}

	// Defines the strength the n'th texture on the stack 't'.
	// All color components (rgb) are multipled with this factor *before* any further processing is done.      -
	maxElements = 1;
	if (AI_SUCCESS == aiGetMaterialFloatArray(aiMat,
											  AI_MATKEY_TEXBLEND(textureTypes[l], k - 1), &floatVal, &maxElements)) {
		texState->set_blendFactor(floatVal);
	}

	// One of the aiTextureOp enumerated values. Defines the arithmetic operation to be used
	// to combine the n'th texture on the stack 't' with the n-1'th.
	// TEXOP(t,0) refers to the blend operation between the base color
	// for this stack (e.g. COLOR_DIFFUSE for the diffuse stack) and the first texture.
	maxElements = 1;
	if (AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
												AI_MATKEY_TEXOP(textureTypes[l], k - 1), &intVal, &maxElements)) {
		switch (intVal) {
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
	if (AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
												AI_MATKEY_MAPPING(textureTypes[l], k - 1), &intVal, &maxElements)) {
		switch (intVal) {
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
	if (AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
												AI_MATKEY_UVWSRC(textureTypes[l], k - 1), &intVal, &maxElements)) {
		texState->set_texcoChannel(intVal);
	}

	TextureWrapping wrapping_(GL_REPEAT);
	// Any of the aiTextureMapMode enumerated values. Defines the texture wrapping mode on the
	// x axis for sampling the n'th texture on the stack 't'.
	// 'Wrapping' occurs whenever UVs lie outside the 0..1 range.
	maxElements = 1;
	if (AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
												AI_MATKEY_MAPPINGMODE_U(textureTypes[l], k - 1), &intVal,
												&maxElements)) {
		switch (intVal) {
			case aiTextureMapMode_Wrap:
				wrapping_.x = GL_REPEAT;
				break;
			case aiTextureMapMode_Clamp:
				wrapping_.x = GL_CLAMP;
				break;
			case aiTextureMapMode_Decal:
				REGEN_WARN("ignoring texture map mode decal.");
				break;
			case aiTextureMapMode_Mirror:
				wrapping_.x = GL_MIRRORED_REPEAT;
				break;
		}
	}
	// Wrap mode on the v axis. See MAPPINGMODE_U.
	maxElements = 1;
	if (AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
												AI_MATKEY_MAPPINGMODE_V(textureTypes[l], k - 1), &intVal,
												&maxElements)) {
		switch (intVal) {
			case aiTextureMapMode_Wrap:
				wrapping_.y = GL_REPEAT;
				break;
			case aiTextureMapMode_Clamp:
				wrapping_.y = GL_CLAMP;
				break;
			case aiTextureMapMode_Decal:
				REGEN_WARN("ignoring texture map mode decal.");
				break;
			case aiTextureMapMode_Mirror:
				wrapping_.y = GL_MIRRORED_REPEAT;
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
		wrapping_.z = GL_REPEAT;
		break;
	  case aiTextureMapMode_Clamp:
		wrapping_.z = GL_CLAMP;
		break;
	  case aiTextureMapMode_Decal:
		REGEN_WARN("ignoring texture map mode decal.");
		break;
	  case aiTextureMapMode_Mirror:
		wrapping_.z = GL_MIRRORED_REPEAT;
		break;
	  }
	}
#endif
	tex->wrapping().push(wrapping_);

	switch (textureTypes[l]) {
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
			// enforce multiply blending such that matEmission parameter gives easy control over
			// amount of material emission.
			texState->set_blendMode(BLEND_MODE_MULTIPLY);
			break;
		case aiTextureType_OPACITY:
			// The texture defines per-pixel opacity.
			// Usually 'white' means opaque and 'black' means 'transparency'.
			// Or quite the opposite. Have fun.
			texState->set_mapTo(TextureState::MAP_TO_ALPHA);
			// TODO: make configurable
			//		- alpha discard threshold
			//      - texel invert
			texState->set_discardAlpha(true, 0.25f);
			//texState->set_texelTransferKey("regen.states.textures.transfer.texel_invert");
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
		case aiTextureType_UNKNOWN:
			// Unknown texture. A texture reference that does not match any of the definitions
			// above is considered to be 'unknown'. It is still imported, but is excluded
			// from any further postprocessing.
			REGEN_WARN("Unknown texture type '" << stringVal.data << "' in '" << filePath << "'.");
			break;
		default:
			REGEN_WARN("Unhandled texture type '" << stringVal.data << "' in '" << filePath << "'.");
			break;
	}

	tex->filter().push(TextureFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
	tex->setupMipmaps(GL_DONT_CARE);
	mat->joinStates(texState);

	tex->end(RenderState::get());
	GL_ERROR_LOG();
}

///////////// MATERIAL

vector<ref_ptr<Material> > AssetImporter::loadMaterials() {
	vector<ref_ptr<Material> > materials(scene_->mNumMaterials);
	aiColor4D aiCol;
	GLfloat floatVal;
	GLint intVal;
	aiString stringVal;
	GLuint maxElements;
	GLuint l, k;

	GL_ERROR_LOG();
	for (GLuint n = 0; n < scene_->mNumMaterials; ++n) {
		ref_ptr<Material> mat = ref_ptr<Material>::alloc();
		materials[n] = mat;
		aiMaterial *aiMat = scene_->mMaterials[n];

		// load textures
		for (l = 0; l < numTextureTyps; ++l) {
			k = 0;
			while (AI_SUCCESS == aiGetMaterialString(aiMat,
													 AI_MATKEY_TEXTURE(textureTypes[l], k), &stringVal)) {
				k += 1;
				if (stringVal.length < 1) continue;

				aiTexture *aiTex;
				if (stringVal.data[0] == '*') {
					char *indexStr = stringVal.data + 1;
					GLuint index;

					stringstream ss;
					ss << indexStr;
					ss >> index;
					if (index < scene_->mNumTextures) {
						aiTex = scene_->mTextures[index];
					} else {
						aiTex = nullptr;
					}
				} else {
					aiTex = nullptr;
				}

				loadTexture(mat, aiTex, aiMat, stringVal, l, k, texturePath_);
			}
		}

		if (AI_SUCCESS == aiGetMaterialColor(aiMat,
											 AI_MATKEY_COLOR_DIFFUSE, &aiCol)) {
			mat->diffuse()->setVertex(0, aiToOgle3f(&aiCol));
		}
		if (AI_SUCCESS == aiGetMaterialColor(aiMat,
											 AI_MATKEY_COLOR_SPECULAR, &aiCol)) {
			mat->specular()->setVertex(0, aiToOgle3f(&aiCol));
		}
		if (AI_SUCCESS == aiGetMaterialColor(aiMat,
											 AI_MATKEY_COLOR_AMBIENT, &aiCol)) {
			mat->ambient()->setVertex(0, aiToOgle3f(&aiCol));
		}
		if (AI_SUCCESS == aiGetMaterialColor(aiMat,
											 AI_MATKEY_COLOR_EMISSIVE, &aiCol)) {
			mat->set_emission(*((Vec3f *) &aiCol));
		}
		// Defines the transparent color of the material,
		// this is the color to be multiplied with the color of translucent light to
		// construct the final 'destination color' for a particular position in the screen buffer.
		if (AI_SUCCESS == aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_TRANSPARENT, &aiCol)) {
			// not supposed to be used like this but for now i think this is ok...
			auto alpha = mat->alpha()->getVertex(0).r;
			mat->alpha()->setVertex(0, alpha * (aiCol.r + aiCol.g + aiCol.b) / 3.0f);
		}

		maxElements = 1;
		// Defines the base shininess of the material
		// This is the exponent of the phong shading equation.
		if (aiGetMaterialFloatArray(aiMat, AI_MATKEY_SHININESS,
									&floatVal, &maxElements) == AI_SUCCESS) {
			mat->shininess()->setVertex(0, floatVal);
		}
		maxElements = 1;
		// Defines the strength of the specular highlight.
		// This is simply a multiplier to the specular color of a material
		if (aiGetMaterialFloatArray(aiMat, AI_MATKEY_SHININESS_STRENGTH,
									&floatVal, &maxElements) == AI_SUCCESS) {
			//mat->set_shininessStrength( floatVal );
		}
		if (aiGetMaterialFloatArray(aiMat, AI_MATKEY_EMISSIVE_INTENSITY,
									&floatVal, &maxElements) == AI_SUCCESS) {
			//mat->set_colorBlendFactor(floatVal);
		}

		maxElements = 1;
		// Defines the base opacity of the material
		if (aiGetMaterialFloatArray(aiMat, AI_MATKEY_OPACITY,
									&floatVal, &maxElements) == AI_SUCCESS) {
			auto alpha = mat->alpha()->getVertex(0).r;
			mat->alpha()->setVertex(0, alpha * floatVal);
		}

		// Index of refraction of the material. This is used by some shading models,
		// e.g. Cook-Torrance. The value is the ratio of the speed of light in a
		// vacuum to the speed of light in the material (always >= 1.0 in the real world).
		// Might be of interest for raytracing.
		maxElements = 1;
		if (aiGetMaterialFloatArray(aiMat, AI_MATKEY_REFRACTI,
									&floatVal, &maxElements) == AI_SUCCESS) {
			mat->refractionIndex()->setVertex(0, floatVal);
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
		if (AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
													AI_MATKEY_ENABLE_WIREFRAME, &intVal, &maxElements)) {
			mat->set_fillMode(intVal ? GL_LINE : GL_FILL);
		} else {
			mat->set_fillMode(GL_FILL);
		}
		maxElements = 1;
		if (AI_SUCCESS == aiGetMaterialIntegerArray(aiMat,
													AI_MATKEY_TWOSIDED, &intVal, &maxElements)) {
			mat->set_twoSided(intVal ? GL_TRUE : GL_FALSE);
		}

		maxElements = 1;
	}
	GL_ERROR_LOG();

	return materials;
}

///////////// MESHES

static GLuint getMeshCount(const struct aiNode *node) {
	GLuint count = node->mNumMeshes;
	for (GLuint n = 0; n < node->mNumChildren; ++n) {
		const struct aiNode *child = node->mChildren[n];
		if (child == nullptr) { continue; }
		count += getMeshCount(child);
	}
	return count;
}

vector<ref_ptr<Mesh> > AssetImporter::loadAllMeshes(
		const Mat4f &transform, VBO::Usage usage) {
	GLuint meshCount = getMeshCount(scene_->mRootNode);

	vector<GLuint> meshIndices(meshCount);
	for (GLuint n = 0; n < meshCount; ++n) { meshIndices[n] = n; }

	return loadMeshes(transform, usage, meshIndices);
}

vector<ref_ptr<Mesh> > AssetImporter::loadMeshes(
		const Mat4f &transform, VBO::Usage usage, const vector<GLuint> &meshIndices) {
	vector<ref_ptr<Mesh> > out(meshIndices.size());
	GLuint currentIndex = 0;

	loadMeshes(*scene_->mRootNode, transform, usage, meshIndices, currentIndex, out);

	return out;
}

void AssetImporter::loadMeshes(
		const struct aiNode &node,
		const Mat4f &transform,
		VBO::Usage usage,
		const vector<GLuint> &meshIndices,
		GLuint &currentIndex,
		vector<ref_ptr<Mesh> > &out) {
	const auto *aiTransform = (const aiMatrix4x4 *) &transform.x;

	// walk through meshes, add primitive set for each mesh
	for (GLuint n = 0; n < node.mNumMeshes; ++n) {
		GLuint index = 0;
		for (index = 0; index < meshIndices.size(); ++index) {
			if (meshIndices[index] == n) break;
		}
		if (index == meshIndices.size()) {
			continue;
		}

		const struct aiMesh *mesh = scene_->mMeshes[node.mMeshes[n]];
		if (mesh == nullptr) { continue; }

		aiMatrix4x4 meshTransform = (*aiTransform) * node.mTransformation;
		ref_ptr<Mesh> meshState = loadMesh(*mesh, *((const Mat4f *) &meshTransform.a1), usage);
		// remember mesh material
		meshMaterials_[meshState.get()] = materials_[mesh->mMaterialIndex];
		meshToAiMesh_[meshState.get()] = mesh;

		out[index] = meshState;
	}

	for (GLuint n = 0; n < node.mNumChildren; ++n) {
		const struct aiNode *child = node.mChildren[n];
		if (child == nullptr) { continue; }
		loadMeshes(*child, transform, usage, meshIndices, currentIndex, out);
	}
}

ref_ptr<Mesh> AssetImporter::loadMesh(const struct aiMesh &mesh, const Mat4f &transform, VBO::Usage usage) {
	ref_ptr<Mesh> meshState = ref_ptr<Mesh>::alloc(GL_TRIANGLES, usage);
	stringstream s;

	ref_ptr<ShaderInput3f> pos = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	ref_ptr<ShaderInput3f> nor = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	ref_ptr<ShaderInput4f> tan = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);

	const GLuint numFaceIndices = (mesh.mNumFaces > 0 ? mesh.mFaces[0].mNumIndices : 0);
	GLuint numFaces = 0;
	for (GLuint t = 0u; t < mesh.mNumFaces; ++t) {
		const struct aiFace *face = &mesh.mFaces[t];
		if (face->mNumIndices != numFaceIndices) { continue; }
		numFaces += 1;
	}
	const GLuint numIndices = numFaceIndices * numFaces;

	switch (numFaceIndices) {
		case 1:
			meshState->set_primitive(GL_POINTS);
			break;
		case 2:
			meshState->set_primitive(GL_LINES);
			break;
		case 3:
			meshState->set_primitive(GL_TRIANGLES);
			break;
		default:
			meshState->set_primitive(GL_POLYGON);
			break;
	}

	meshState->begin(ShaderInputContainer::INTERLEAVED);

	{
		ref_ptr<ShaderInput1ui> indices = ref_ptr<ShaderInput1ui>::alloc("i");
		indices->setVertexData(numIndices);
		auto faceIndices = indices->mapClientData<GLuint>(ShaderData::WRITE);
		GLuint index = 0, maxIndex = 0;
		for (GLuint t = 0u; t < mesh.mNumFaces; ++t) {
			const struct aiFace *face = &mesh.mFaces[t];
			if (face->mNumIndices != numFaceIndices) { continue; }
			for (GLuint n = 0; n < face->mNumIndices; ++n) {
				faceIndices.w[index] = face->mIndices[n];
				if (face->mIndices[n] > maxIndex) { maxIndex = face->mIndices[n]; }
				index += 1;
			}
		}
		faceIndices.unmap();
		meshState->setIndices(indices, maxIndex);
	}

	const auto *aiTransform = (const aiMatrix4x4 *) &transform.x;
	// vertex positions
	Vec3f min_(999999.9);
	Vec3f max_(-999999.9);
	GLuint numVertices = mesh.mNumVertices;
	{
		pos->setVertexData(numVertices);
		auto v_pos = pos->mapClientData<Vec3f>(ShaderData::WRITE);
		for (GLuint n = 0; n < numVertices; ++n) {
			aiVector3D aiv = (*aiTransform) * mesh.mVertices[n];
			Vec3f &v = *((Vec3f *) &aiv.x);
			v_pos.w[n] = v;
			min_.setMin(v);
			max_.setMax(v);
		}
		v_pos.unmap();
		meshState->setInput(pos);
	}
	meshState->set_bounds(min_, max_);

	// per vertex normals
	if (mesh.HasNormals()) {
		nor->setVertexData(numVertices);
		auto v_nor = nor->mapClientData<Vec3f>(ShaderData::WRITE);
		for (GLuint n = 0; n < numVertices; ++n) {
			Vec3f &v = *((Vec3f *) &mesh.mNormals[n].x);
			v_nor.w[n] = v;
		}
		v_nor.unmap();
		meshState->setInput(nor);
	}

	// per vertex colors
	for (GLuint t = 0; t < AI_MAX_NUMBER_OF_COLOR_SETS; ++t) {
		if (mesh.mColors[t] == nullptr) continue;

		ref_ptr<ShaderInput4f> col = ref_ptr<ShaderInput4f>::alloc(REGEN_STRING("col" << t));
		col->setVertexData(numVertices);
		auto v_col = col->mapClientData<Vec4f>(ShaderData::WRITE);
		for (GLuint n = 0; n < numVertices; ++n) {
			v_col.w[n] = Vec4f(
					mesh.mColors[t][n].r,
					mesh.mColors[t][n].g,
					mesh.mColors[t][n].b,
					mesh.mColors[t][n].a);
		}
		v_col.unmap();
		meshState->setInput(col);
	}

	// load texture coordinates
	for (GLuint t = 0; t < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++t) {
		if (mesh.mTextureCoords[t] == nullptr) { continue; }
		aiVector3D *aiTexcos = mesh.mTextureCoords[t];
		GLuint texcoComponents = mesh.mNumUVComponents[t];
		string texcoName = REGEN_STRING("texco" << t);

		ref_ptr<ShaderInput> texco;
		if (texcoComponents == 1) {
			texco = ref_ptr<ShaderInput1f>::alloc(texcoName);
		} else if (texcoComponents == 3) {
			texco = ref_ptr<ShaderInput3f>::alloc(texcoName);
		} else if (texcoComponents == 4) {
			texco = ref_ptr<ShaderInput4f>::alloc(texcoName);
		} else {
			texco = ref_ptr<ShaderInput2f>::alloc(texcoName);
		}
		texco->setVertexData(numVertices);
		auto v_texco = texco->mapClientData<float>(ShaderData::WRITE);

		for (GLuint n = 0; n < numVertices; ++n) {
			GLfloat *aiTexcoData = &(aiTexcos[n].x);
			for (GLuint x = 0; x < texcoComponents; ++x) v_texco.w[x] = aiTexcoData[x];
			v_texco.w += texcoComponents;
		}
		v_texco.unmap();
		meshState->setInput(texco);
	}

	// load tangents
	if (mesh.HasTangentsAndBitangents()) {
		tan->setVertexData(numVertices);
		auto v_tan = tan->mapClientData<Vec4f>(ShaderData::WRITE);
		for (GLuint i = 0; i < numVertices; ++i) {
			Vec3f &t = *((Vec3f *) &mesh.mTangents[i].x);
			Vec3f &b = *((Vec3f *) &mesh.mBitangents[i].x);
			Vec3f &n = *((Vec3f *) &mesh.mNormals[i].x);
			// Calculate the handedness of the local tangent space.
			GLfloat handeness;
			if (n.cross(t).dot(b) < 0.0) {
				handeness = -1.0;
			} else {
				handeness = 1.0;
			}
			v_tan.w[i] = Vec4f(t.x, t.y, t.z, handeness);
		}
		v_tan.unmap();
		meshState->setInput(tan);
	}
	GL_ERROR_LOG();

	// A mesh may have a set of bones in the form of aiBone structures..
	// Bones are a means to deform a mesh according to the movement of a skeleton.
	// Each bone has a name and a set of vertices on which it has influence.
	// Its offset matrix declares the transformation needed to transform from mesh space
	// to the local space of this bone.
	if (mesh.HasBones()) {
		typedef list<pair<GLfloat, GLuint> > WeightList;
		map<GLuint, WeightList> vertexToWeights;
		GLuint maxNumWeights = 0;

		// collect weights at vertices
		for (GLuint boneIndex = 0; boneIndex < mesh.mNumBones; ++boneIndex) {
			aiBone *assimpBone = mesh.mBones[boneIndex];
			for (GLuint t = 0; t < assimpBone->mNumWeights; ++t) {
				aiVertexWeight &weight = assimpBone->mWeights[t];
				vertexToWeights[weight.mVertexId].emplace_back(weight.mWeight, boneIndex);
				maxNumWeights = max(maxNumWeights,
									(GLuint) vertexToWeights[weight.mVertexId].size());
			}
		}
		meshState->shaderDefine("NUM_BONE_WEIGHTS", REGEN_STRING(maxNumWeights));

		if (maxNumWeights < 1) {
			REGEN_ERROR("The model has invalid bone weights number " << maxNumWeights << ".");
		} else {
			// create array of bone weights and indices
			auto boneWeights = ref_ptr<ShaderInput1f>::alloc("boneWeights", maxNumWeights);
			auto boneIndices = ref_ptr<ShaderInput1ui>::alloc("boneIndices", maxNumWeights);
			boneWeights->setVertexData(numVertices);
			boneIndices->setVertexData(numVertices);
			auto v_weights = boneWeights->mapClientData<GLfloat>(ShaderData::WRITE);
			auto v_indices = boneIndices->mapClientData<GLuint>(ShaderData::WRITE);

			for (GLuint j = 0; j < numVertices; j++) {
				WeightList &vWeights = vertexToWeights[j];

				GLuint k = 0;
				for (auto it = vWeights.begin(); it != vWeights.end(); ++it) {
					v_weights.w[k] = it->first;
					v_indices.w[k] = it->second;
					++k;
				}
				for (; k < maxNumWeights; ++k) {
					v_weights.w[k] = 0.0f;
					v_indices.w[k] = 0u;
				}

				v_weights.w += maxNumWeights;
				v_indices.w += maxNumWeights;
			}

			v_weights.unmap();
			v_indices.unmap();

			if (maxNumWeights > 1) {
				meshState->setInput(boneWeights);
			}
			meshState->setInput(boneIndices);
		}
	}
	GL_ERROR_LOG();

	meshState->end();

	return meshState;
}

list<ref_ptr<AnimationNode> > AssetImporter::loadMeshBones(
		Mesh *meshState, NodeAnimation *anim) {
	const struct aiMesh *mesh = meshToAiMesh_[meshState];
	if (mesh->mNumBones == 0) { return {}; }

	list<ref_ptr<AnimationNode> > boneNodes;
	for (GLuint boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
		aiBone *assimpBone = mesh->mBones[boneIndex];
		string nodeName = string(assimpBone->mName.data);
		ref_ptr<AnimationNode> animNode = anim->findNode(nodeName);
		// hoping that meshes do not share bones here....
		// is there a usecase for bone sharing between meshes ?
		// if so the offset matrix can only be used in BonesState class
		// and not in the bone class.
		animNode->set_boneOffsetMatrix(*((Mat4f *) &assimpBone->mOffsetMatrix.a1));
		boneNodes.push_back(animNode);
	}
	return boneNodes;
}

GLuint AssetImporter::numBoneWeights(Mesh *meshState) {
	const struct aiMesh *mesh = meshToAiMesh_[meshState];
	if (mesh->mNumBones == 0) { return 0; }
	const ref_ptr<ShaderInputContainer> container = meshState->inputContainer();

	auto *counter = new GLuint[container->numVertices()];
	GLuint numWeights = 1;
	for (GLint i = 0; i < container->numVertices(); ++i) counter[i] = 0u;
	for (GLuint boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
		aiBone *assimpBone = mesh->mBones[boneIndex];
		for (GLuint t = 0; t < assimpBone->mNumWeights; ++t) {
			aiVertexWeight &weight = assimpBone->mWeights[t];
			counter[weight.mVertexId] += 1;
			numWeights = max(numWeights, counter[weight.mVertexId]);
		}
	}
	delete[]counter;
	return numWeights;
}

ref_ptr<Material> AssetImporter::getMeshMaterial(Mesh *state) {
	return meshMaterials_[state];
}

///////////// NODE ANIMATION

static NodeAnimation::Behavior animState(aiAnimBehaviour b) {
	switch (b) {
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

ref_ptr<AnimationNode> AssetImporter::loadNodeTree() {
	if (scene_->HasAnimations()) {
		GLboolean hasAnimations = false;
		for (GLuint i = 0; i < scene_->mNumAnimations; ++i) {
			if (scene_->mAnimations[i]->mNumChannels > 0) {
				hasAnimations = true;
				break;
			}
		}
		if (hasAnimations) {
			return loadNodeTree(scene_->mRootNode, ref_ptr<AnimationNode>());
		}
	}
	return {};
}

ref_ptr<AnimationNode> AssetImporter::loadNodeTree(aiNode *assimpNode, const ref_ptr<AnimationNode> &parent) {
	ref_ptr<AnimationNode> node = ref_ptr<AnimationNode>::alloc(string(assimpNode->mName.data), parent);
	aiNodeToNode_[assimpNode] = node;
	nodes_[string(assimpNode->mName.data)] = assimpNode;

	node->set_localTransform(*((Mat4f *) &assimpNode->mTransformation.a1));
	node->calculateGlobalTransform();

	// continue for all child nodes and assign the created internal nodes as our children
	for (GLuint i = 0; i < assimpNode->mNumChildren; ++i) {
		ref_ptr<AnimationNode> subTree = loadNodeTree(assimpNode->mChildren[i], node);
		node->addChild(subTree);
	}

	return node;
}

const vector<ref_ptr<NodeAnimation> > &AssetImporter::getNodeAnimations() { return nodeAnimations_; }

void AssetImporter::loadNodeAnimation(const AssimpAnimationConfig &animConfig) {
	if (!animConfig.useAnimation || !rootNode_.get()) {
		return;
	}

	ref_ptr<NodeAnimation> anim = ref_ptr<NodeAnimation>::alloc(rootNode_);
	ref_ptr<vector<NodeAnimation::Channel> > channels;
	ref_ptr<vector<NodeAnimation::KeyFrame3f> > scalingKeys;
	ref_ptr<vector<NodeAnimation::KeyFrame3f> > positionKeys;
	ref_ptr<vector<NodeAnimation::KeyFrameQuaternion> > rotationKeys;

	for (GLuint i = 0; i < scene_->mNumAnimations; ++i) {
		aiAnimation *assimpAnim = scene_->mAnimations[i];
		// extract ticks per second. Assume default value if not given
		GLdouble ticksPerSecond = (assimpAnim->mTicksPerSecond != 0.0 ?
								   assimpAnim->mTicksPerSecond : animConfig.ticksPerSecond);

		auto animName = assimpAnim->mName.C_Str();
		double duration = assimpAnim->mDuration;

		REGEN_DEBUG("Loading animation " << animName <<
			" with duration " << duration <<
			" and ticks per second " << ticksPerSecond <<
			" duration in seconds " << duration / ticksPerSecond);

		if (assimpAnim->mNumChannels <= 0) continue;

		channels = ref_ptr<vector<NodeAnimation::Channel> >::alloc(assimpAnim->mNumChannels);
		vector<NodeAnimation::Channel> &channelsPtr = *channels.get();

		for (GLuint j = 0; j < assimpAnim->mNumChannels; ++j) {
			aiNodeAnim *nodeAnim = assimpAnim->mChannels[j];

			auto scalingKeys = ref_ptr<vector<NodeAnimation::KeyFrame3f> >::alloc(nodeAnim->mNumScalingKeys);
			vector<NodeAnimation::KeyFrame3f> &scalingKeys_ = *scalingKeys.get();
			GLboolean useScale = false;
			for (GLuint k = 0; k < nodeAnim->mNumScalingKeys; ++k) {
				NodeAnimation::KeyFrame3f &key = scalingKeys_[k];
				key.time = nodeAnim->mScalingKeys[k].mTime;
				key.value = *((Vec3f *) &(nodeAnim->mScalingKeys[k].mValue.x));
				if (key.time > 0.0001) useScale = true;
			}

			if (!useScale && !scalingKeys_.empty()) {
				if (scalingKeys_[0].value.isApprox(Vec3f::one(), 1e-6)) {
					scalingKeys_.resize(0);
				} else {
					scalingKeys_.resize(1, scalingKeys_[0]);
				}
			}

			////////////

			positionKeys = ref_ptr<vector<NodeAnimation::KeyFrame3f> >::alloc(nodeAnim->mNumPositionKeys);
			vector<NodeAnimation::KeyFrame3f> &positionKeys_ = *positionKeys.get();
			GLboolean usePosition = false;

			for (GLuint k = 0; k < nodeAnim->mNumPositionKeys; ++k) {
				NodeAnimation::KeyFrame3f &key = positionKeys_[k];
				key.time = nodeAnim->mPositionKeys[k].mTime;
				key.value = *((Vec3f *) &(nodeAnim->mPositionKeys[k].mValue.x));
				if (key.time > 0.0001) usePosition = true;
			}

			if (!usePosition && !positionKeys_.empty()) {
				if (positionKeys_[0].value.isApprox(Vec3f::zero(), 1e-6)) {
					positionKeys_.resize(0);
				} else {
					positionKeys_.resize(1, positionKeys_[0]);
				}
			}

			///////////

			rotationKeys = ref_ptr<vector<NodeAnimation::KeyFrameQuaternion> >::alloc(nodeAnim->mNumRotationKeys);
			vector<NodeAnimation::KeyFrameQuaternion> &rotationKeyss_ = *rotationKeys.get();
			GLboolean useRotation = false;
			for (GLuint k = 0; k < nodeAnim->mNumRotationKeys; ++k) {
				NodeAnimation::KeyFrameQuaternion &key = rotationKeyss_[k];
				key.time = nodeAnim->mRotationKeys[k].mTime;
				key.value = *((Quaternion *) &(nodeAnim->mRotationKeys[k].mValue.w));
				if (key.time > 0.0001) useRotation = true;
			}

			if (!useRotation && !rotationKeyss_.empty()) {
				if (rotationKeyss_[0].value == Quaternion(1, 0, 0, 0)) {
					rotationKeyss_.resize(0);
				} else {
					rotationKeyss_.resize(1, rotationKeyss_[0]);
				}
			}

			NodeAnimation::Channel &channel = channelsPtr[j];
			channel.nodeName_ = string(nodeAnim->mNodeName.data);
			if (animConfig.forceStates) {
				channel.postState = animConfig.postState;
				channel.preState = animConfig.preState;
			} else {
				channel.postState = animState(nodeAnim->mPostState);
				channel.preState = animState(nodeAnim->mPreState);
			}
			channel.scalingKeys_ = scalingKeys;
			channel.positionKeys_ = positionKeys;
			channel.rotationKeys_ = rotationKeys;
		}

		anim->addChannels(
				string(assimpAnim->mName.data),
				channels,
				assimpAnim->mDuration,
				ticksPerSecond
		);
	}

	nodeAnimations_ = vector<ref_ptr<NodeAnimation> >(
			animConfig.numInstances > 1 ? animConfig.numInstances : 1);
	nodeAnimations_[0] = anim;
	for (GLuint i = 1; i < nodeAnimations_.size(); ++i) {
		nodeAnimations_[i] = anim->copy(GL_FALSE);
	}
}

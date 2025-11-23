#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <regen/utility/logging.h>
#include <regen/textures/texture-loader.h>
#include <regen/av/video-texture.h>
#include <regen/animation/animation-manager.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <IL/il.h>

#include "assimp-importer.h"
#include "regen/utility/filesystem.h"

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

static void assimpLog(const char *msg, char *) {
	std::string_view s(msg);
	if (s.size() > 0) {
		// remove trailing \n
		if (s[s.size() - 1] == '\n') {
			s.remove_suffix(1);
		}
		// Note: Assimp prepends logging level in the string. But it is too verbose,
		//       also reports some errors that don't seem to be relevant.
		//       So for now ust print all to debug output.
		REGEN_DEBUG(s);
	}
}

static const struct aiScene *importFile(
		const string &assimpFile,
		int userSpecifiedFlags) {
	// get a handle to the predefined STDOUT log stream and attach
	// it to the logging system. It remains active for all further
	// calls to aiImportFile(Ex) and aiApplyPostProcessing.
	static struct aiLogStream stream;
	static bool isLoggingInitialled = false;
	if (!isLoggingInitialled) {
		stream.callback = assimpLog;
		stream.user = nullptr;
		aiDetachAllLogStreams();
		aiAttachLogStream(&stream);
		isLoggingInitialled = true;
	}
	return aiImportFile(assimpFile.c_str(), userSpecifiedFlags);
}

static Vec3f &aiToOgle(aiColor3D *v) { return *((Vec3f *) v); }

static Vec3f &aiToOgle3f(aiColor4D *v) { return *((Vec3f *) v); }

AssetImporter::AssetImporter(const string &assetFile)
		: assetFile_(assetFile),
		  scene_(nullptr) {
	boost::filesystem::path p(assetFile);
	texturePath_ = p.parent_path().string();
	setAiProcessFlags_Regen();
}

AssetImporter::~AssetImporter() {
	if (scene_ != nullptr) {
		aiReleaseImport(scene_);
	}
}

void AssetImporter::importAsset() {
	scene_ = importFile(assetFile_, aiProcessFlags_);
	if (scene_ == nullptr) {
		throw Error(REGEN_STRING("Can not import assimp file '" <<
																assetFile_ << "'. " << aiGetErrorString()));
	}
	lights_ = loadLights();
	materials_ = loadMaterials();
	loadNodeAnimation(animationCfg_);
	GL_ERROR_LOG();
}

void AssetImporter::setAiProcessFlags_Fast() {
	aiProcessFlags_ = aiProcessPreset_TargetRealtime_Fast;
}

void AssetImporter::setAiProcessFlags_Quality() {
	aiProcessFlags_ = aiProcessPreset_TargetRealtime_Quality;
}

void AssetImporter::setAiProcessFlags_Regen() {
	aiProcessFlags_ = aiProcess_Triangulate
			| aiProcess_GenUVCoords
			| aiProcess_CalcTangentSpace
			| aiProcess_SortByPType
			| aiProcess_ImproveCacheLocality
			| aiProcess_RemoveRedundantMaterials
			| aiProcess_OptimizeMeshes
			| aiProcess_OptimizeGraph
			| aiProcess_JoinIdenticalVertices
			| aiProcess_LimitBoneWeights
			| 0;
}

///////////// LIGHTS

static void setLightRadius(aiLight *aiLight, ref_ptr<Light> &light) {
	float ax = aiLight->mAttenuationLinear;
	float ay = aiLight->mAttenuationConstant;
	float az = aiLight->mAttenuationQuadratic;
	float z = ay / (2.0f * az);

	float start = 0.01;
	float stop = 0.99;

	float inner = -z + sqrt(z * z - (ax / start - 1.0f / (start * az)));
	float outer = -z + sqrt(z * z - (ax / stop - 1.0f / (stop * az)));

	light->setRadius(0, Vec2f(inner, outer));
}

vector<ref_ptr<Light> > AssetImporter::loadLights() {
	vector<ref_ptr<Light> > ret(scene_->mNumLights);

	for (uint32_t i = 0; i < scene_->mNumLights; ++i) {
		aiLight *assimpLight = scene_->mLights[i];
		// node could be animated, but for now it is ignored
		aiNode *node = aiBoneNodes_[string(assimpLight->mName.data)];
		if (node == nullptr) {
			REGEN_WARN("Light '" << assimpLight->mName.data << "' has no corresponding node.");
			continue;
		}
		aiVector3D lightPos = node->mTransformation * assimpLight->mPosition;

		ref_ptr<Light> light;
		switch (assimpLight->mType) {
			case aiLightSource_DIRECTIONAL: {
				light = ref_ptr<Light>::alloc(Light::DIRECTIONAL);
				light->setDirection(0, *((Vec3f *) &lightPos.x));
				break;
			}
			case aiLightSource_POINT: {
				light = ref_ptr<Light>::alloc(Light::POINT);
				light->setPosition(0, *((Vec3f *) &lightPos.x));
				setLightRadius(assimpLight, light);
				break;
			}
			case aiLightSource_SPOT: {
				light = ref_ptr<Light>::alloc(Light::SPOT);
				light->setPosition(0, *((Vec3f *) &lightPos.x));
				light->setDirection(0, *((Vec3f *) &assimpLight->mDirection.x));
				light->setConeAngles(
						acos(assimpLight->mAngleOuterCone) * 360.0f / (2.0f * M_PI),
						acos(assimpLight->mAngleInnerCone) * 360.0f / (2.0f * M_PI));
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
		light->setDiffuse(0, aiToOgle(&assimpLight->mColorDiffuse));
		light->setSpecular(0, aiToOgle(&assimpLight->mColorSpecular));

		ret[i] = light;
	}

	return ret;
}

///////////// TEXTURES

static void loadTexture(
		ref_ptr<Material> &mat,
		aiTexture *aiTexture,
		aiMaterial *aiMat,
		aiString &stringVal,
		uint32_t l, uint32_t k,
		const string &texturePath) {
	ref_ptr<Texture> tex;
	std::string filePath;
	int intVal;
	float floatVal;

	if (aiTexture == nullptr) {
		if (boost::filesystem::exists(stringVal.data)) {
			filePath = stringVal.data;
		} else {
			std::vector<string> names;
			filePath = stringVal.data;
			boost::split(names, filePath, boost::is_any_of("/\\"));
			filePath = names[names.size() - 1];

			std::string buf = REGEN_STRING(texturePath << "/" << filePath);
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

		uint32_t numBytes = aiTexture->mWidth;
		std::string ext(aiTexture->achFormatHint);

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
	} else {
		// The texture is NOT compressed
		tex = ref_ptr<Texture2D>::alloc();
		tex->set_rectangleSize(aiTexture->mWidth, aiTexture->mHeight);
		tex->set_pixelType(GL_UNSIGNED_BYTE);
		tex->set_format(GL_RGBA);
		tex->set_internalFormat(GL_RGBA8);
		tex->allocTexture();
		tex->set_filter(TextureFilter::create(GL_LINEAR));
		tex->set_wrapping(TextureWrapping::create(GL_REPEAT));
		tex->updateImage((GLubyte *) aiTexture->pcData);
	}

	ref_ptr<TextureState> texState = ref_ptr<TextureState>::alloc(tex);

	// Defines miscellaneous flag for the n'th texture on the stack 't'.
	// This is a bitwise combination of the aiTextureFlags enumerated values.
	if (AI_SUCCESS == aiMat->Get(AI_MATKEY_TEXFLAGS(textureTypes[l], k - 1), intVal)) {
		if (intVal & aiTextureFlags_Invert) {
			texState->set_texelTransfer(TextureState::TEXEL_TRANSFER_INVERT);
		}
		if (intVal & aiTextureFlags_UseAlpha) {
			REGEN_WARN("aiTextureFlags_UseAlpha is not supported.");
		}
		if (intVal & aiTextureFlags_IgnoreAlpha) {
			texState->set_ignoreAlpha(true);
		}
	}

	// Defines the height scaling of a bump map (for stuff like Parallax Occlusion Mapping)
	if (aiMat->Get(AI_MATKEY_BUMPSCALING, floatVal) == AI_SUCCESS) {
		texState->set_blendFactor(floatVal);
	}

	// Defines the strength the n'th texture on the stack 't'.
	// All color components (rgb) are multipled with this factor *before* any further processing is done.      -
	if (AI_SUCCESS == aiMat->Get(AI_MATKEY_TEXBLEND(textureTypes[l], k - 1), floatVal)) {
		texState->set_blendFactor(floatVal);
	}

	// One of the aiTextureOp enumerated values. Defines the arithmetic operation to be used
	// to combine the n'th texture on the stack 't' with the n-1'th.
	// TEXOP(t,0) refers to the blend operation between the base color
	// for this stack (e.g. COLOR_DIFFUSE for the diffuse stack) and the first texture.
	if (AI_SUCCESS == aiMat->Get(AI_MATKEY_TEXOP(textureTypes[l], k - 1), intVal)) {
		switch (intVal) {
			case aiTextureOp_Multiply:
				texState->set_blendMode(BLEND_MODE_MULTIPLY);
				break;
			case aiTextureOp_SignedAdd:
			case aiTextureOp_Add:
				texState->set_blendMode(BLEND_MODE_ADD);
				break;
			case aiTextureOp_Subtract:
				texState->set_blendMode(BLEND_MODE_SUBTRACT);
				break;
			case aiTextureOp_Divide:
				texState->set_blendMode(BLEND_MODE_DIVIDE);
				break;
			case aiTextureOp_SmoothAdd:
				texState->set_blendMode(BLEND_MODE_SMOOTH_ADD);
				break;
			default:
				REGEN_WARN("Unknown texture operation '" << intVal << "'.");
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
	if (AI_SUCCESS == aiMat->Get(AI_MATKEY_MAPPING(textureTypes[l], k - 1), intVal)) {
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
			default:
				REGEN_WARN("Unknown texture mapping '" << intVal << "'.");
				break;
		}
	}

	// Defines the UV channel to be used as input mapping coordinates for sampling the
	// n'th texture on the stack 't'. All meshes assigned to this material share
	// the same UV channel setup     Presence of this key implies MAPPING(t,n) to be
	// aiTextureMapping_UV. See How to map UV channels to textures (MATKEY_UVWSRC) for more details.
	if (AI_SUCCESS == aiMat->Get(AI_MATKEY_UVWSRC(textureTypes[l], k - 1), intVal)) {
		texState->set_texcoChannel(intVal);
	}

	TextureWrapping wrapping_(TextureWrapping::create(GL_REPEAT));
	// Any of the aiTextureMapMode enumerated values. Defines the texture wrapping mode on the
	// x axis for sampling the n'th texture on the stack 't'.
	// 'Wrapping' occurs whenever UVs lie outside the 0..1 range.
	if (AI_SUCCESS == aiMat->Get(AI_MATKEY_MAPPINGMODE_U(textureTypes[l], k - 1), intVal)) {
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
			default:
				REGEN_WARN("Unknown texture map mode '" << intVal << "'.");
				break;
		}
	}
	// Wrap mode on the v axis. See MAPPINGMODE_U.
	if (AI_SUCCESS == aiMat->Get(AI_MATKEY_MAPPINGMODE_V(textureTypes[l], k - 1), intVal)) {
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
			default:
				REGEN_WARN("Unknown texture map mode '" << intVal << "'.");
				break;
		}
	}
#ifdef AI_MATKEY_MAPPINGMODE_W
	// Wrap mode on the v axis. See MAPPINGMODE_U.
	if(AI_SUCCESS == aiMat->Get(AI_MATKEY_MAPPINGMODE_W(textureTypes[l],k-1), intVal)) {
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
	tex->set_wrapping(wrapping_);

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
			REGEN_WARN("Unknown texture type 'NONE' in '" << filePath << "'.");
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

	if (texState->isNormalMap()) {
		// Normal maps should use linear filtering, but no mipmaps.
		tex->set_filter(TextureFilter(GL_LINEAR, GL_LINEAR));
	} else {
		// Other textures should use linear mipmap filtering.
		// Note: Assimp does not provide a way to specify the filter type.
		//       So we assume that all textures are mipmapped.
		tex->updateMipmaps();
	}
	mat->joinStates(texState);
	GL_ERROR_LOG();
}

///////////// MATERIAL

std::vector<ref_ptr<Material> > AssetImporter::loadMaterials() {
	std::vector<ref_ptr<Material> > materials(scene_->mNumMaterials);
	aiColor4D aiCol;
	float floatVal;
	int intVal;
	aiString stringVal;
	uint32_t l, k;

	for (uint32_t n = 0; n < scene_->mNumMaterials; ++n) {
		ref_ptr<Material> mat = ref_ptr<Material>::alloc();
		materials[n] = mat;
		aiMaterial *aiMat = scene_->mMaterials[n];

		// load textures
		for (l = 0; l < numTextureTyps; ++l) {
			k = 0;
			while (AI_SUCCESS == aiMat->Get(AI_MATKEY_TEXTURE(textureTypes[l], k), stringVal)) {
				k += 1;
				if (stringVal.length < 1) continue;

				aiTexture *aiTex;
				if (stringVal.data[0] == '*') {
					char *indexStr = stringVal.data + 1;
					uint32_t index;

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

				// skip normal map if requested
				if (textureTypes[l] == aiTextureType_NORMALS && importFlags_ & IGNORE_NORMAL_MAP) {
					continue;
				}

				loadTexture(mat, aiTex, aiMat, stringVal, l, k, texturePath_);
			}
		}

		float opacity = mat->alpha()->getVertex(0).r;
		Vec3f diffuseColor = Vec3f::one();
		if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, aiCol)) {
			diffuseColor = aiToOgle3f(&aiCol);
		}
		// AI_MATKEY_COLOR_TRANSPARENT is a bit confusing, it is meant as the color that
		// should be fully transparent. e.g. if it is (1,0,0,1) then red parts should
		// be fully transparent. However, AFAIK this is not supposed to be applied per-pixel,
		// but rather as an overall material property.
		if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_TRANSPARENT, aiCol)) {
			// Apply color-key transparency
			if (fabs(diffuseColor.x - aiCol.r) < 0.01f &&
				fabs(diffuseColor.y - aiCol.g) < 0.01f &&
				fabs(diffuseColor.z - aiCol.b) < 0.01f) {
				opacity = 0.0f;
			}
		}
		mat->diffuse()->setVertex(0, diffuseColor);

		if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_SPECULAR, aiCol)) {
			mat->specular()->setVertex(0, aiToOgle3f(&aiCol));
		}

		float emissionStrength = emissionStrength_;
		if (aiMat->Get(AI_MATKEY_EMISSIVE_INTENSITY, floatVal) == AI_SUCCESS) {
			if (floatVal > 0.0f && floatVal < 1.0f - 1e-5f) {
				mat->setEmissionMultiplier(floatVal);
			}
		}
		if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, aiCol)) {
			mat->set_emission(aiToOgle3f(&aiCol) * emissionStrength);
		}

		// Defines the base shininess of the material
		// This is the exponent of the phong shading equation.
		if (aiMat->Get(AI_MATKEY_SHININESS, floatVal) == AI_SUCCESS) {
			mat->shininess()->setVertex(0, floatVal);
		}
		// Defines the strength of the specular highlight.
		// This is simply a multiplier to the specular color of a material
		if (aiMat->Get(AI_MATKEY_SHININESS_STRENGTH, floatVal) == AI_SUCCESS) {
			mat->setSpecularMultiplier(floatVal);
		}

		// Defines the base opacity of the material
		if (aiMat->Get(AI_MATKEY_OPACITY, floatVal) == AI_SUCCESS) {
			opacity *= floatVal;
		}
		mat->alpha()->setVertex(0, opacity);

		// Index of refraction of the material. This is used by some shading models,
		// e.g. Cook-Torrance. The value is the ratio of the speed of light in a
		// vacuum to the speed of light in the material (always >= 1.0 in the real world).
		// Might be of interest for raytracing.
		if (aiMat->Get(AI_MATKEY_REFRACTI, floatVal) == AI_SUCCESS) {
			mat->refractionIndex()->setVertex(0, floatVal);
		}

#ifdef AI_MATKEY_COOK_TORRANCE_PARAM
		if(aiMat->Get(AI_MATKEY_COOK_TORRANCE_PARAM, floatVal) == AI_SUCCESS) {
		  mat->set_cookTorranceParam( floatVal );
		}
#endif

#ifdef AI_MATKEY_ORENNAYAR_ROUGHNESS
		if(aiMat->Get(aiMat, AI_MATKEY_ORENNAYAR_ROUGHNESS, floatVal) == AI_SUCCESS) {
		  mat->set_roughness( floatVal );
		}
#endif

#ifdef AI_MATKEY_MINNAERT_DARKNESS
		if(aiMat->Get(AI_MATKEY_MINNAERT_DARKNESS, floatVal) == AI_SUCCESS) {
		  mat->set_darkness( floatVal );
		}
#endif

		if (AI_SUCCESS == aiMat->Get(AI_MATKEY_ENABLE_WIREFRAME, intVal)) {
			mat->set_fillMode(intVal ? GL_LINE : GL_FILL);
		} else {
			mat->set_fillMode(GL_FILL);
		}
		if (AI_SUCCESS == aiMat->Get(AI_MATKEY_TWOSIDED, intVal)) {
			mat->set_twoSided(intVal ? true : false);
		}
	}

	return materials;
}

///////////// MESHES

static uint32_t getMeshCount(const struct aiNode *node) {
	uint32_t count = node->mNumMeshes;
	for (uint32_t n = 0; n < node->mNumChildren; ++n) {
		const struct aiNode *child = node->mChildren[n];
		if (child == nullptr) { continue; }
		count += getMeshCount(child);
	}
	return count;
}

vector<ref_ptr<Mesh> > AssetImporter::loadAllMeshes(
		const Mat4f &transform, const BufferFlags &bufferFlags) {
	uint32_t meshCount = getMeshCount(scene_->mRootNode);

	vector<uint32_t> meshIndices(meshCount);
	for (uint32_t n = 0; n < meshCount; ++n) { meshIndices[n] = n; }

	return loadMeshes(transform, bufferFlags, meshIndices);
}

vector<ref_ptr<Mesh> > AssetImporter::loadMeshes(
		const Mat4f &transform, const BufferFlags &bufferFlags, const vector<uint32_t> &meshIndices) {
	vector<ref_ptr<Mesh> > out(meshIndices.size());
	uint32_t currentIndex = 0;

	loadMeshes(*scene_->mRootNode, transform, bufferFlags, meshIndices, currentIndex, out);

	return out;
}

void AssetImporter::loadMeshes(
		const struct aiNode &node,
		const Mat4f &transform,
		const BufferFlags &bufferFlags,
		const vector<uint32_t> &meshIndices,
		uint32_t &currentIndex,
		vector<ref_ptr<Mesh> > &out) {
	const auto *aiTransform = (const aiMatrix4x4 *) &transform.x;

	// walk through meshes, add primitive set for each mesh
	for (uint32_t n = 0; n < node.mNumMeshes; ++n) {
		uint32_t index = 0;
		for (index = 0; index < meshIndices.size(); ++index) {
			if (meshIndices[index] == n) break;
		}
		if (index == meshIndices.size()) {
			continue;
		}

		const struct aiMesh *mesh = scene_->mMeshes[node.mMeshes[n]];
		if (mesh == nullptr) { continue; }

		aiMatrix4x4 meshTransform = (*aiTransform) * node.mTransformation;
		ref_ptr<Mesh> meshState = loadMesh(*mesh, *((const Mat4f *) &meshTransform.a1),  bufferFlags);
		// remember mesh material
		meshMaterials_[meshState.get()] = materials_[mesh->mMaterialIndex];
		meshToAiMesh_[meshState.get()] = mesh;

		out[index] = meshState;
	}

	for (uint32_t n = 0; n < node.mNumChildren; ++n) {
		const struct aiNode *child = node.mChildren[n];
		if (child == nullptr) { continue; }
		loadMeshes(*child, transform, bufferFlags, meshIndices, currentIndex, out);
	}
}

ref_ptr<Mesh> AssetImporter::loadMesh(const struct aiMesh &mesh, const Mat4f &transform, const BufferFlags &bufferFlags) {
	ref_ptr<Mesh> meshState = ref_ptr<Mesh>::alloc(GL_TRIANGLES, bufferFlags.updateHints);
	if (bufferFlags.accessMode == BUFFER_GPU_ONLY) {
		meshState->setClientAccessMode(BUFFER_CPU_WRITE);
	} else if (bufferFlags.accessMode == BUFFER_CPU_READ) {
		meshState->setClientAccessMode(BUFFER_CPU_READ);
	} else {
		meshState->setClientAccessMode(bufferFlags.accessMode);
	}
	meshState->setBufferMapMode(bufferFlags.mapMode);

	ref_ptr<ShaderInput3f> pos = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	ref_ptr<ShaderInput3f> nor = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	ref_ptr<ShaderInput4f> tan = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);

	const uint32_t numFaceIndices = (mesh.mNumFaces > 0 ? mesh.mFaces[0].mNumIndices : 0);
	uint32_t numFaces = 0;
	for (uint32_t t = 0u; t < mesh.mNumFaces; ++t) {
		const struct aiFace *face = &mesh.mFaces[t];
		if (face->mNumIndices != numFaceIndices) { continue; }
		numFaces += 1;
	}
	const uint32_t numIndices = numFaceIndices * numFaces;

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

	{
		ref_ptr<ShaderInput> indices = createIndexInput(numIndices, mesh.mNumVertices);
		auto faceIndices = (byte*)indices->clientBuffer()->clientData(0);
		auto indexType = indices->baseType();
		uint32_t index = 0, maxIndex = 0;
		for (uint32_t t = 0u; t < mesh.mNumFaces; ++t) {
			const struct aiFace *face = &mesh.mFaces[t];
			if (face->mNumIndices != numFaceIndices) { continue; }
			for (uint32_t n = 0; n < face->mNumIndices; ++n) {
				setIndexValue(faceIndices, indexType, index, face->mIndices[n]);
				if (face->mIndices[n] > maxIndex) { maxIndex = face->mIndices[n]; }
				index += 1;
			}
		}
		auto ref = meshState->setIndices(indices, maxIndex);
		indices->set_offset(ref->address());
	}

	const auto *aiTransform = (const aiMatrix4x4 *) &transform.x;
	// vertex positions
	Vec3f min_ = Vec3f::create(999999.9);
	Vec3f max_ = Vec3f::create(-999999.9);
	uint32_t numVertices = mesh.mNumVertices;
	{
		pos->setVertexData(numVertices);
		auto v_pos = (Vec3f*) pos->clientBuffer()->clientData(0);
		for (uint32_t n = 0; n < numVertices; ++n) {
			aiVector3D aiv = (*aiTransform) * mesh.mVertices[n];
			Vec3f &v = *((Vec3f *) &aiv.x);
			v_pos[n] = v;
			min_.setMin(v);
			max_.setMax(v);
		}
		meshState->setInput(pos);
	}
	meshState->set_bounds(min_, max_);

	// per vertex normals
	if (mesh.HasNormals()) {
		nor->setVertexData(numVertices);
		auto v_nor = (Vec3f*) nor->clientBuffer()->clientData(0);
		for (uint32_t n = 0; n < numVertices; ++n) {
			Vec3f &v = *((Vec3f *) &mesh.mNormals[n].x);
			v_nor[n] = v;
		}
		meshState->setInput(nor);
	}

	// per vertex colors
	for (uint32_t t = 0; t < AI_MAX_NUMBER_OF_COLOR_SETS; ++t) {
		if (mesh.mColors[t] == nullptr) continue;

		ref_ptr<ShaderInput4f> col = ref_ptr<ShaderInput4f>::alloc(REGEN_STRING("col" << t));
		col->setVertexData(numVertices);
		auto v_col = (Vec4f*) col->clientBuffer()->clientData(0);
		for (uint32_t n = 0; n < numVertices; ++n) {
			v_col[n] = Vec4f(
					mesh.mColors[t][n].r,
					mesh.mColors[t][n].g,
					mesh.mColors[t][n].b,
					mesh.mColors[t][n].a);
		}
		meshState->setInput(col);
	}

	// load texture coordinates
	for (uint32_t t = 0; t < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++t) {
		if (mesh.mTextureCoords[t] == nullptr) { continue; }
		aiVector3D *aiTexcos = mesh.mTextureCoords[t];
		uint32_t texcoComponents = mesh.mNumUVComponents[t];
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
		auto v_texco = (float*) texco->clientBuffer()->clientData(0);
		for (uint32_t n = 0; n < numVertices; ++n) {
			float *aiTexcoData = &(aiTexcos[n].x);
			for (uint32_t x = 0; x < texcoComponents; ++x) v_texco[x] = aiTexcoData[x];
			v_texco += texcoComponents;
		}
		meshState->setInput(texco);
	}

	// load tangents
	if (mesh.HasTangentsAndBitangents()) {
		tan->setVertexData(numVertices);
		auto v_tan = (Vec4f*) tan->clientBuffer()->clientData(0);
		for (uint32_t i = 0; i < numVertices; ++i) {
			Vec3f &t = *((Vec3f *) &mesh.mTangents[i].x);
			Vec3f &b = *((Vec3f *) &mesh.mBitangents[i].x);
			Vec3f &n = *((Vec3f *) &mesh.mNormals[i].x);
			// Calculate the handedness of the local tangent space.
			float handeness;
			if (n.cross(t).dot(b) < 0.0) {
				handeness = -1.0;
			} else {
				handeness = 1.0;
			}
			v_tan[i] = Vec4f(t.x, t.y, t.z, handeness);
		}
		meshState->setInput(tan);
	}

	// A mesh may have a set of bones in the form of aiBone structures..
	// Bones are a means to deform a mesh according to the movement of a skeleton.
	// Each bone has a name and a set of vertices on which it has influence.
	// Its offset matrix declares the transformation needed to transform from mesh space
	// to the local space of this bone.
	if (mesh.HasBones()) {
		typedef list<pair<float, uint32_t> > WeightList;
		map<uint32_t, WeightList> vertexToWeights;
		uint32_t maxNumWeights = 0;

		// collect weights at vertices
		for (uint32_t boneIndex = 0; boneIndex < mesh.mNumBones; ++boneIndex) {
			aiBone *assimpBone = mesh.mBones[boneIndex];
			for (uint32_t t = 0; t < assimpBone->mNumWeights; ++t) {
				aiVertexWeight &weight = assimpBone->mWeights[t];
				vertexToWeights[weight.mVertexId].emplace_back(weight.mWeight, boneIndex);
				maxNumWeights = max(maxNumWeights,
									(uint32_t) vertexToWeights[weight.mVertexId].size());
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
			auto v_weights = (float*) boneWeights->clientBuffer()->clientData(0);
			auto v_indices = (uint32_t*) boneIndices->clientBuffer()->clientData(0);

			for (uint32_t j = 0; j < numVertices; j++) {
				WeightList &vWeights = vertexToWeights[j];

				uint32_t k = 0;
				for (auto & vWeight : vWeights) {
					v_weights[k] = vWeight.first;
					v_indices[k] = vWeight.second;
					++k;
				}
				for (; k < maxNumWeights; ++k) {
					v_weights[k] = 0.0f;
					v_indices[k] = 0u;
				}

				v_weights += maxNumWeights;
				v_indices += maxNumWeights;
			}

			if (maxNumWeights > 1) {
				meshState->setInput(boneWeights);
			}
			meshState->setInput(boneIndices);
		}
	}
	GL_ERROR_LOG();

	meshState->updateVertexData();
	meshState->ensureLOD();

	return meshState;
}

list<ref_ptr<BoneNode>> AssetImporter::loadMeshBones(
		Mesh *meshState, BoneTree *anim) {
	const struct aiMesh *mesh = meshToAiMesh_[meshState];
	if (mesh->mNumBones == 0) { return {}; }

	list<ref_ptr<BoneNode>> boneNodes;
	for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
		aiBone *assimpBone = mesh->mBones[boneIndex];
		string nodeName = string(assimpBone->mName.data);
		ref_ptr<BoneNode> animNode = anim->findNode(nodeName);
		anim->setOffsetMatrix(animNode->nodeIdx, *((Mat4f *) &assimpBone->mOffsetMatrix.a1));
		anim->setIsBoneNode(animNode->nodeIdx, true);
		boneNodes.push_back(animNode);
	}
	return boneNodes;
}

uint32_t AssetImporter::numBoneWeights(Mesh *meshState) {
	const struct aiMesh *mesh = meshToAiMesh_[meshState];
	if (mesh->mNumBones == 0) { return 0; }

	auto *counter = new uint32_t[meshState->numVertices()];
	uint32_t numWeights = 1;
	for (int i = 0; i < meshState->numVertices(); ++i) counter[i] = 0u;
	for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
		aiBone *assimpBone = mesh->mBones[boneIndex];
		for (uint32_t t = 0; t < assimpBone->mNumWeights; ++t) {
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

static AnimationChannelBehavior animState(aiAnimBehaviour b) {
	switch (b) {
		case aiAnimBehaviour_CONSTANT:
			return AnimationChannelBehavior::CONSTANT;
		case aiAnimBehaviour_LINEAR:
			return AnimationChannelBehavior::LINEAR;
		case aiAnimBehaviour_REPEAT:
			return AnimationChannelBehavior::REPEAT;
		case _aiAnimBehaviour_Force32Bit:
		case aiAnimBehaviour_DEFAULT:
		default:
			return AnimationChannelBehavior::DEFAULT;
	}
}

ref_ptr<BoneNode> AssetImporter::loadNodeTree(std::vector<ref_ptr<BoneNode>> &nodes,
			aiNode *assimpNode, const ref_ptr<BoneNode> &parent) {
	auto node = ref_ptr<BoneNode>::alloc(string(assimpNode->mName.data), parent);
	aiNodeToNode_[assimpNode] = node;
	aiBoneNodes_[string(assimpNode->mName.data)] = assimpNode;
	nodes.push_back(node);

	// continue for all child nodes and assign the created internal nodes as our children
	for (uint32_t i = 0; i < assimpNode->mNumChildren; ++i) {
		ref_ptr<BoneNode> subTree = loadNodeTree(nodes, assimpNode->mChildren[i], node);
		node->addChild(subTree);
	}

	return node;
}

ref_ptr<BoneNode> AssetImporter::loadNodeTree(std::vector<ref_ptr<BoneNode>> &nodes) {
	if (scene_->HasAnimations()) {
		bool hasAnimations = false;
		for (uint32_t i = 0; i < scene_->mNumAnimations; ++i) {
			if (scene_->mAnimations[i]->mNumChannels > 0) {
				hasAnimations = true;
				break;
			}
		}
		if (hasAnimations) {
			return loadNodeTree(nodes, scene_->mRootNode, ref_ptr<BoneNode>());
		}
	}
	return {};
}

void AssetImporter::loadNodeAnimation(const AssimpAnimationConfig &animConfig) {
	if (!animConfig.useAnimation) { return; }

	std::vector<ref_ptr<BoneNode>> nodes;
	loadNodeTree(nodes);
	if (nodes.empty()) { return; }

	rootNode_ = nodes[0];
	ref_ptr<BoneTree> anim = ref_ptr<BoneTree>::alloc(nodes, animConfig.numInstances);

	for (auto &node : nodes) {
		auto &aiNode = aiBoneNodes_[node->name];
		anim->setLocalTransform(node->nodeIdx,
			*((Mat4f *) &aiNode->mTransformation.a1));
	}

	for (uint32_t i = 0; i < scene_->mNumAnimations; ++i) {
		aiAnimation *assimpAnim = scene_->mAnimations[i];
		// extract ticks per second. Assume default value if not given
		double ticksPerSecond = (assimpAnim->mTicksPerSecond != 0.0 ? assimpAnim->mTicksPerSecond : animConfig.ticksPerSecond);
		std::string animName(assimpAnim->mName.C_Str());
		// Some exporters like blender tend to prepend path into the names like "Armature|Hyenas_A15_Atk".
		// We are not interested in the path for now, so let's strip it.
		size_t sepPos = animName.find_last_of("|/");
		if (sepPos != std::string::npos) {
			animName = animName.substr(sepPos + 1);
		}
		double duration = assimpAnim->mDuration;

		REGEN_DEBUG("Loading animation " << animName <<
										 " with duration " << duration <<
										 " and ticks per second " << ticksPerSecond <<
										 " duration in seconds " << duration / ticksPerSecond);

		if (assimpAnim->mNumChannels <= 0) continue;

		ref_ptr<StaticAnimationData> animData = ref_ptr<StaticAnimationData>::alloc();
		animData->channels.resize(assimpAnim->mNumChannels);

		for (uint32_t j = 0; j < assimpAnim->mNumChannels; ++j) {
			aiNodeAnim *nodeAnim = assimpAnim->mChannels[j];
			auto &channel = animData->channels[j];

			channel.scalingKeys_.resize(nodeAnim->mNumScalingKeys);
			auto &scalingKeys = channel.scalingKeys_;
			bool useScale = false;
			for (uint32_t k = 0; k < nodeAnim->mNumScalingKeys; ++k) {
				auto &key = scalingKeys[k];
				key.time = nodeAnim->mScalingKeys[k].mTime;
				key.value = *((Vec3f *) &(nodeAnim->mScalingKeys[k].mValue.x));
				if (key.time > 0.0001) useScale = true;
			}

			if (!useScale && !scalingKeys.empty()) {
				if (scalingKeys[0].value.isApprox(Vec3f::one(), 1e-6)) {
					scalingKeys.resize(0);
				} else {
					scalingKeys.resize(1, scalingKeys[0]);
				}
			}

			////////////
			channel.positionKeys_.resize(nodeAnim->mNumPositionKeys);
			auto &positionKeys = channel.positionKeys_;
			bool usePosition = false;

			for (uint32_t k = 0; k < nodeAnim->mNumPositionKeys; ++k) {
				auto &key = positionKeys[k];
				key.time = nodeAnim->mPositionKeys[k].mTime;
				key.value = *((Vec3f *) &(nodeAnim->mPositionKeys[k].mValue.x));
				if (key.time > 0.0001) usePosition = true;
			}

			if (!usePosition && !positionKeys.empty()) {
				if (positionKeys[0].value.isApprox(Vec3f::zero(), 1e-6)) {
					positionKeys.resize(0);
				} else {
					positionKeys.resize(1, positionKeys[0]);
				}
			}

			///////////
			channel.rotationKeys_.resize(nodeAnim->mNumRotationKeys);
			auto &rotationKeys = channel.rotationKeys_;
			bool useRotation = false;
			for (uint32_t k = 0; k < nodeAnim->mNumRotationKeys; ++k) {
				auto &key = rotationKeys[k];
				key.time = nodeAnim->mRotationKeys[k].mTime;
				key.value = *((Quaternion *) &(nodeAnim->mRotationKeys[k].mValue.w));
				if (key.time > 0.0001) useRotation = true;
			}

			if (!useRotation && !rotationKeys.empty()) {
				if (rotationKeys[0].value == Quaternion(1, 0, 0, 0)) {
					rotationKeys.resize(0);
				} else {
					rotationKeys.resize(1, rotationKeys[0]);
				}
			}

			channel.nodeName_ = string(nodeAnim->mNodeName.data);
			if (animConfig.forceStates) {
				channel.postState = animConfig.postState;
				channel.preState = animConfig.preState;
			} else {
				channel.postState = animState(nodeAnim->mPostState);
				channel.preState = animState(nodeAnim->mPreState);
			}
		}

		anim->addAnimationTrack(
				animName,
				animData,
				assimpAnim->mDuration,
				ticksPerSecond);
	}
	nodeAnimation_ = anim;
}

ref_ptr<AssetImporter> AssetImporter::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	if (!input.hasAttribute("file")) {
		REGEN_WARN("Ignoring Asset '" << input.getDescription() << "' without file.");
		return {};
	}
	const std::string assetPath = resourcePath(input.getValue("file"));

	AssimpAnimationConfig animConfig;
	animConfig.numInstances =
			input.getValue<uint32_t>("animation-instances", 1u);
	animConfig.useAnimation = (animConfig.numInstances > 0) &&
							  input.getValue<bool>("use-animation", true);
	animConfig.forceStates =
			input.getValue<bool>("animation-force-states", true);
	animConfig.ticksPerSecond =
			input.getValue<float>("animation-tps", 20.0);
	animConfig.postState = input.getValue<AnimationChannelBehavior>(
			"animation-post-state",
			AnimationChannelBehavior::LINEAR);
	animConfig.preState = input.getValue<AnimationChannelBehavior>(
			"animation-pre-state",
			AnimationChannelBehavior::LINEAR);

	try {
		auto asset = ref_ptr<AssetImporter>::alloc(assetPath);
		if (input.hasAttribute("texture-path")) {
			asset->setTexturePath(resourcePath(input.getValue("texture-path")));
		}
		if (input.hasAttribute("emission-strength")) {
			asset->setEmissionStrength(input.getValue<float>("emission-strength", 1.0f));
		}
		auto preset = input.getValue<std::string>("preset", "");
		if (preset == "fast") {
			asset->setAiProcessFlags_Fast();
		} else if (preset == "quality") {
			asset->setAiProcessFlags_Quality();
		}
		if (input.getValue<bool>("debone", false)) {
			asset->setAiProcessFlag(aiProcess_Debone);
		}
		if (input.getValue<bool>("limit-bone-weights", false)) {
			asset->setAiProcessFlag(aiProcess_LimitBoneWeights);
		}
		if (input.getValue<bool>("gen-normals", false)) {
			asset->setAiProcessFlag(aiProcess_GenNormals);
		} else if (input.getValue<bool>("gen-smooth-normals", false)) {
			asset->setAiProcessFlag(aiProcess_GenSmoothNormals);
		}
		if (input.getValue<bool>("force-gen-normals", false)) {
			asset->setAiProcessFlag(aiProcess_ForceGenNormals);
		}
		if (input.getValue<bool>("gen-uv-coords", false) ||
					input.getValue<bool>("gen-uv", false)) {
			asset->setAiProcessFlag(aiProcess_GenUVCoords);
		}
		if (input.getValue<bool>("flip-uvs", false)) {
			asset->setAiProcessFlag(aiProcess_FlipUVs);
		}
		if (input.getValue<bool>("fix-infacing-normals", false)) {
			asset->setAiProcessFlag(aiProcess_FixInfacingNormals);
		}
		if (input.getValue<bool>("flip-winding-order", false)) {
			asset->setAiProcessFlag(aiProcess_FlipWindingOrder);
		}
		if (input.getValue<bool>("optimize-graph", false)) {
			asset->setAiProcessFlag(aiProcess_OptimizeGraph);
		}
		if (input.getValue<bool>("optimize-meshes", false)) {
			asset->setAiProcessFlag(aiProcess_OptimizeMeshes);
		}
		if (input.getValue<bool>("triangulate", true)) {
			asset->setAiProcessFlag(aiProcess_Triangulate);
		}
		if (input.getValue<bool>("calc-tangent-space", false) ||
					input.getValue<bool>("gen-tangents", false)) {
			asset->setAiProcessFlag(aiProcess_CalcTangentSpace);
		}
		asset->setAnimationConfig(animConfig);
		if (input.getValue<bool>("ignore-normal-map", false)) {
			asset->setImportFlags(AssetImporter::IGNORE_NORMAL_MAP);
		}
		asset->importAsset();

		REGEN_INFO("Loaded Asset '" << assetPath << "' with " <<
			getMeshCount(asset->scene_->mRootNode) << " meshes, " <<
			asset->materials().size() << " materials, " <<
			asset->lights().size() << " lights, " <<
			" has animation: " << (asset->getNodeAnimation().get() != nullptr));

		return asset;
	}
	catch (AssetImporter::Error &e) {
		REGEN_WARN("Unable to open Asset file: " << e.what() << ".");
		return {};
	}
}

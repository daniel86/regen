#include <regen/states/texture-state.h>
#include "mask-mesh.h"

using namespace regen;

MaskMesh::MaskMesh(const ref_ptr<Texture2D> &maskTexture, const Config &cfg)
		: Rectangle(cfg.quad),
		  maskTexture_(maskTexture),
		  meshSize_(cfg.meshSize)
{
	auto ts = ref_ptr<TextureState>::alloc(maskTexture_, "maskTexture");
	ts->set_mapTo(TextureState::MAP_TO_VERTEX_MASK);
	ts->set_mapping(TextureState::MAPPING_XZ_PLANE);
	joinStates(ts);

	modelOffset_ = ref_ptr<ShaderInput3f>::alloc("modelOffset");
	updateMask(cfg);
}

MaskMesh::MaskMesh(const ref_ptr<MaskMesh> &other)
		: Rectangle(other) {
	maskTexture_ = other->maskTexture_;
	modelOffset_ = other->modelOffset_;
	meshSize_ = other->meshSize_;
	joinShaderInput(modelOffset_);

	auto ts = ref_ptr<TextureState>::alloc(maskTexture_, "maskTexture");
	ts->set_mapTo(TextureState::MAP_TO_VERTEX_MASK);
	ts->set_mapping(TextureState::MAPPING_XZ_PLANE);
	joinStates(ts);
}

MaskMesh::Config::Config()
		: meshSize(Vec2f(10.0f)),
		  height(0.0f) {
}

void MaskMesh::updateMask(const Config &cfg) {
	unsigned int quadCountX = std::ceil(cfg.meshSize.x / cfg.quad.posScale.x);
	unsigned int quadCountY = std::ceil(cfg.meshSize.y / cfg.quad.posScale.z);
	Vec2f quadSize_ts = Vec2f(
			cfg.quad.posScale.x / cfg.meshSize.x,
			cfg.quad.posScale.z / cfg.meshSize.y);
	Vec2f quadHalfSize = Vec2f(cfg.quad.posScale.x, cfg.quad.posScale.z) * 0.5f;
	std::vector<Vec3f> instanceData(quadCountX * quadCountY);

	unsigned int numInstances = 0;

	maskTexture_->ensureTextureData();
	auto *maskTextureData = maskTexture_->textureData();

	Vec2f maskUV = quadSize_ts * 0.5f;

	for (unsigned int y = 0; y < quadCountY; ++y) {
		for (unsigned int x = 0; x < quadCountX; ++x) {
			float maskDensity = maskTexture_->sampleMax(
				maskUV,
				quadSize_ts,
				maskTextureData,
				1
			);
			maskUV.x += quadSize_ts.x;
			if (maskDensity > 0.1) {
				// TODO: generate better fitting quads, but geometry cannot be changed as instancing is used
				//          could use scaling instead though to make instances smaller. This might be fine for some cases.
				//auto corrected_x = static_cast<float>(masked.second.min.x + masked.second.max.x) * 0.5f;
				//auto corrected_y = static_cast<float>(masked.second.min.y + masked.second.max.y) * 0.5f;
				instanceData[numInstances++] = Vec3f(
					static_cast<float>( x ) * cfg.quad.posScale.x + quadHalfSize.x - cfg.meshSize.x * 0.5f,
					cfg.height,
					static_cast<float>( y ) * cfg.quad.posScale.z + quadHalfSize.y - cfg.meshSize.y * 0.5f);
			}
		}
		maskUV.x = quadSize_ts.x * 0.5f;
		maskUV.y += quadSize_ts.y;
	}

	// update the model offset attribute
	GLuint instanceDivisor = 1;
	instanceData.resize(numInstances);
	modelOffset_->setInstanceData(numInstances, instanceDivisor, (byte *) instanceData.data());
	disjoinShaderInput(modelOffset_);
	joinShaderInput(modelOffset_);
}

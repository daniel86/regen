#include "blanket-trail.h"

#include "regen/objects/composite-mesh.h"
#include "regen/shapes/indexed-shape.h"

using namespace regen;

static uint32_t computeMaxInstances(const BlanketTrail::TrailConfig &cfg) {
	uint32_t maxInstances = cfg.numTrails;
	if (cfg.blanketLifetime > 0.0f && cfg.blanketFrequency > 0.0f) {
		// compute number of instances based on walk speed and lifetime
		maxInstances += static_cast<uint32_t>(cfg.numTrails *
			std::ceil(2.0 * cfg.blanketLifetime / cfg.blanketFrequency));
	}
	return maxInstances;
}

BlanketTrail::BlanketTrail(
	const ref_ptr<Ground> &groundMesh,
	const TrailConfig &cfg) : Blanket(cfg, computeMaxInstances(cfg)) {
	groundMesh_ = groundMesh;
	insertHeight_ = groundMesh_->mapCenter()->getVertex(0).r.y -
			0.5f*groundMesh_->mapSize()->getVertex(0).r.y;
}

void BlanketTrail::setBlanketMask(const ref_ptr<Texture> &tex) {
	blanketMask_ = tex;
	if (!blanketMask_) {
		return;
	}
	blanketMaskState_ = ref_ptr<TextureState>::alloc(blanketMask_, "blanketMask");
	blanketMaskState_->set_mapTo(TextureState::MAP_TO_CUSTOM);
	blanketMaskState_->set_mapping(TextureState::MAPPING_CUSTOM);
	joinStates(blanketMaskState_);

	uint32_t numMaskTextures = tex->depth();
	if (numMaskTextures > 1) {
		maskIndex_.resize(numBlankets_, 0);
		u_maskIndex_ = ref_ptr<ShaderInput1ui>::alloc("maskIndex");
		u_maskIndex_->setInstanceData(numBlankets_, 1, (byte*)maskIndex_.data());
		setInput(u_maskIndex_);
	} else {
		maskIndex_.clear();
		u_maskIndex_ = {};
	}
}

void BlanketTrail::setModelTransform(const ref_ptr<ModelTransformation> &tf) {
	tf_ = tf;

	// initialize TF to num instances
	if (tf_.get()) {
		tf_->set_numInstances(numBlankets_);
		auto modelMat = tf_->modelMat();
		modelMat->setInstanceData(numBlankets_, 1, nullptr);

		Mat4f *mat0 = (Mat4f*) modelMat->clientData(0);
		Mat4f *mat1 = (Mat4f*) modelMat->clientData(1);
		for (uint32_t j = 0; j < numBlankets_; j += 1) {
			mat0[j] = Mat4f::identity();
			if(mat1) mat1[j] = Mat4f::identity();
		}

		// distribute initially randomly
		for (uint32_t i = 0; i < numBlankets_; ++i) {
			float randX = (math::random<float>() - 0.5f) * 2.0f * 1000.0f;
			float randZ = (math::random<float>() - 0.5f) * 2.0f * 1000.0f;
			mat0[i].translate(Vec3f(randX, 2000.0f, randZ));
			if(mat1) mat1[i].translate(Vec3f(randX, 2000.0f, randZ));
		}
	}

	joinStates(tf);
}

void BlanketTrail::insertBlanket(const Vec3f &pos, const Vec3f &dir, uint32_t maskIndex) {
	uint32_t instanceIdx = reviveBlanket();

	// update TF
	Quaternion q(0.0, 0.0, 0.0, 1.0);
	q.setEuler(dir.x, dir.y, dir.z);
	tmpMat_ = q.calculateMatrix();
	tmpMat_.translate(Vec3f(pos.x, insertHeight_, pos.z));
	tf_->setModelMat(instanceIdx, tmpMat_);

	if(indexedShapes_.get()) {
		// Also update bounding shape position in spatial index.
		auto &iShape = indexedShape(instanceIdx);
		iShape->setBaseOffset(Vec3f(0.0f, pos.y - insertHeight_, 0.0f));
		iShape->nextLocalStamp();
	}

	// update mask index
	if (!maskIndex_.empty() && maskIndex_[instanceIdx] != maskIndex) {
		maskIndex_[instanceIdx] = maskIndex;
		u_maskIndex_->setVertex(instanceIdx, maskIndex);
	}
}

ref_ptr<BlanketTrail> BlanketTrail::load(LoadingContext &ctx,
			scene::SceneInputNode &input,
			const std::vector<uint32_t> &lodLevels) {
	auto scene = ctx.scene();
	ref_ptr<ModelTransformation> tf;
	ref_ptr<State> dummy = ref_ptr<State>::alloc();

	ref_ptr<Ground> groundMesh;
	auto compositeMesh = ctx.scene()->getResource<CompositeMesh>(input.getValue("ground-mesh"));
	if (compositeMesh.get() != nullptr && compositeMesh->meshes().size() > 0) {
		auto mesh = compositeMesh->meshes()[0];
		groundMesh = ref_ptr<Ground>::dynamicCast(mesh);
	}
	if (!groundMesh) {
		REGEN_ERROR("No valid ground mesh found for blanket trail in " << input.getDescription() << ".");
		return {};
	}

	BufferUpdateFlags updateFlags;
	updateFlags.frequency = input.getValue<BufferUpdateFrequency>("update-frequency", BUFFER_UPDATE_NEVER);
	updateFlags.scope = input.getValue<BufferUpdateScope>("update-scope", BUFFER_UPDATE_FULLY);
	BlanketTrail::TrailConfig meshCfg;
	meshCfg.levelOfDetails = lodLevels;
	meshCfg.texcoScale = input.getValue<Vec2f>("texco-scaling", Vec2f::one());
	meshCfg.blanketSize = input.getValue<Vec2f>("blanket-size", Vec2f(1.0f, 1.0f));
	meshCfg.isInitiallyDead = input.getValue<bool>("initially-dead", false);
	meshCfg.blanketLifetime = input.getValue<float>("blanket-lifetime", 0.0f);
	meshCfg.updateHint = updateFlags;
	meshCfg.mapMode = input.getValue<BufferMapMode>("map-mode", BUFFER_MAP_DISABLED);
	meshCfg.accessMode = input.getValue<ClientAccessMode>("access-mode", BUFFER_CPU_WRITE);
	meshCfg.numTrails = input.getValue<uint32_t>("blanket-trails", 1u);
	meshCfg.blanketFrequency = input.getValue<float>("blanket-frequency", 0.5f);
	auto blanket = ref_ptr<BlanketTrail>::alloc(groundMesh, meshCfg);

	// early add mesh vector to scene for children handling below
	auto out_ = ref_ptr<CompositeMesh>::alloc();
	out_->addMesh(blanket);
	ctx.scene()->putResource<CompositeMesh>(input.getName(), out_);

	auto tfChild = input.getFirstChild("transform");
	if (tfChild.get()) {
		// load the model transformation
		tf = ModelTransformation::load(ctx, *tfChild.get(), dummy);
		scene->putResource("ModelTransformation", tfChild->getValue("id"), tf);
		input.removeChild(tfChild);
	}
	if (!tf.get()) {
		tf = ref_ptr<ModelTransformation>::alloc();
	}
	blanket->setModelTransform(tf);

	auto maskNode = input.getFirstChild("blanket-mask");
	if (maskNode.get()) {
		ref_ptr<Texture> blanketMask;
		// avoid re-processing
		input.removeChild(maskNode);

		if (maskNode->hasAttribute("fbo")) {
			auto fboName = maskNode->getValue<std::string>("fbo", "");
			auto fbo = ctx.scene()->getResource<FBO>(fboName);
			if (fbo.get()) {
				uint32_t attachmentIdx = maskNode->getValue<uint32_t>("attachment", 0);
				blanketMask = fbo->colorTextures()[attachmentIdx];
			}
			if (!blanketMask.get()) {
				REGEN_WARN("No valid FBO/attachment found for blanket mask in " << maskNode->getDescription() << ".");
			}
		}
		if (blanketMask.get()) {
			blanket->setBlanketMask(blanketMask);
		}
	}
	groundMesh->setupGroundBlanket(*blanket.get());

	// Load shapes, this is needed to initially set the traversal mask for all shapes.
	std::vector<ref_ptr<scene::SceneInputNode>> handledChildren;
	for (auto &n: input.getChildren("shape")) {
		handledChildren.push_back(n);
		// Initialize shapes. For this we need to make sure that the shape children are loaded.
		auto processor = ctx.scene()->getStateProcessor(n->getCategory());
		if (processor.get() == nullptr) {
			REGEN_WARN("No processor registered for '" << n->getDescription() << "'.");
		} else {
			ref_ptr<State> dummyState = ref_ptr<State>::alloc();
			processor->processInput(ctx.scene(), *n.get(), ctx.parent(), dummyState);
		}
	}
	for (auto &n: handledChildren) {
		// make sure mesh loading does not attempt to load materials again (this will cause a warning)
		input.removeChild(n);
	}

	blanket->updateAttributes();

	return blanket;
}

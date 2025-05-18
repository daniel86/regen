#include "impostor-billboard.h"
#include "regen/scene/resource-manager.h"

using namespace regen;

#define DEBUG_SNAPSHOT_VIEWS

ImpostorBillboard::ImpostorBillboard()
		: Mesh(GL_POINTS, BUFFER_USAGE_STATIC_DRAW),
		  snapshotState_(ref_ptr<State>::alloc()) {
	depthOffset_ = createUniform<ShaderInput1f>("depthOffset", 0.5f);
	modelOrigin_ = createUniform<ShaderInput3f>("modelOrigin", Vec3f::zero());
	updateAttributes();
}

void ImpostorBillboard::updateAttributes() {
	if (hasAttributes_) return;
	auto positionIn = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	Vec3f posData[1] = {Vec3f(0.0f, 0.0f, 0.0f)};
	positionIn->setVertexData(1, (byte *) posData);

	begin(InputContainer::INTERLEAVED);
	setInput(positionIn);
	end();
	hasAttributes_ = true;
}

void ImpostorBillboard::addMesh(const ref_ptr<Mesh> &mesh, const ref_ptr<State> &drawState) {
	auto &imitation = meshes_.emplace_back();
	imitation.mesh = mesh;
	imitation.drawState = drawState;
	imitation.shaderState = ref_ptr<ShaderState>::alloc();

	if (meshes_.size() == 1) {
		minPosition_ = mesh->minPosition();
		maxPosition_ = mesh->maxPosition();
	} else {
		minPosition_.setMin(mesh->minPosition());
		maxPosition_.setMax(mesh->maxPosition());
	}
	Bounds<Vec3f> meshBounds(minPosition_, maxPosition_);
	meshCenterPoint_ = meshBounds.center();
	modelOrigin_->setVertex(0, meshCenterPoint_);
	meshBoundsRadius_ = meshBounds.radius();
	meshCornerPoints_ = meshBounds.cornerPoints();
	// subtract the center from the mesh corner points
	for (auto &corner: meshCornerPoints_) {
		corner -= meshCenterPoint_;
	}

	REGEN_INFO("ImpostorBillboard mesh center (model space): " << meshCenterPoint_ <<
															   " radius: " << meshBoundsRadius_ <<
															   " min (model space): " << minPosition_ <<
															   " max (model space): " << maxPosition_);
}

void ImpostorBillboard::updateNumberOfViews() {
	// compute the number of snapshots.
	numSnapshotViews_ = 0u;
	if (latitudeSteps_ == 0) {
		numSnapshotViews_ = longitudeSteps_;
	} else {
		if (isHemispherical_) {
			numSnapshotViews_ = latitudeSteps_ * longitudeSteps_;
		} else {
			numSnapshotViews_ = (latitudeSteps_ * 2 - 1) * longitudeSteps_;
		}
	}
	if (hasTopView_) numSnapshotViews_++;
	if (hasBottomView_ && !isHemispherical_) numSnapshotViews_++;
	shaderDefine("NUM_IMPOSTOR_VIEWS", REGEN_STRING(numSnapshotViews_));

	REGEN_INFO("ImpostorBillboard will take " << numSnapshotViews_ << " snapshots of the mesh.");
}

void ImpostorBillboard::ensureResourcesExist() {
	if (!hasInitializedResources_) {
		createResources();
	}
}

void ImpostorBillboard::createResources() {
	hasInitializedResources_ = true;
	updateNumberOfViews();

	{ // create UBO with some parameters for the shader
		billboardUBO_ = ref_ptr<UBO>::alloc("Billboard", BUFFER_USAGE_STATIC_DRAW);
		billboardUBO_->addBlockInput(depthOffset_);
		billboardUBO_->addBlockInput(modelOrigin_);
		billboardUBO_->update();
		joinShaderInput(billboardUBO_);
	}

	{ // create view data arrays
		ssbo_snapshotDirs_ = ref_ptr<SSBO>::alloc("SnapshotDirsData", BUFFER_USAGE_STATIC_DRAW);
		snapshotDirs_ = ref_ptr<ShaderInput3f>::alloc("snapshotDirs", numSnapshotViews_);
		snapshotDirs_->setUniformUntyped();
		ssbo_snapshotDirs_->addBlockInput(snapshotDirs_);
		snapshotState_->joinShaderInput(ssbo_snapshotDirs_);
		joinShaderInput(ssbo_snapshotDirs_);

		ssbo_snapshotOrthoBounds_ = ref_ptr<SSBO>::alloc("SnapshotOrthoBoundsData", BUFFER_USAGE_STATIC_DRAW);
		snapshotOrthoBounds_ = ref_ptr<ShaderInput4f>::alloc("snapshotOrthoBounds", numSnapshotViews_);
		snapshotOrthoBounds_->setUniformUntyped();
		ssbo_snapshotOrthoBounds_->addBlockInput(snapshotOrthoBounds_);
		snapshotState_->joinShaderInput(ssbo_snapshotOrthoBounds_);
		joinShaderInput(ssbo_snapshotOrthoBounds_);

		ssbo_snapshotDepthRanges_ = ref_ptr<SSBO>::alloc("SnapshotDepthRangesData", BUFFER_USAGE_STATIC_DRAW);
		snapshotDepthRanges_ = ref_ptr<ShaderInput2f>::alloc("snapshotDepthRanges", numSnapshotViews_);
		snapshotDepthRanges_->setUniformUntyped();
		ssbo_snapshotDepthRanges_->addBlockInput(snapshotDepthRanges_);
		snapshotState_->joinShaderInput(ssbo_snapshotDepthRanges_);
		joinShaderInput(ssbo_snapshotDepthRanges_);
	}

	{ // create the snapshot FBO
		snapshotFBO_ = ref_ptr<FBO>::alloc(snapshotWidth_, snapshotHeight_, numSnapshotViews_);
		// create albedo texture
		auto albedo = snapshotFBO_->addTexture(1, GL_TEXTURE_2D_ARRAY,
											   GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);
		snapshotAlbedo_ = ref_ptr<Texture2DArray>::dynamicCast(albedo);
		// create normal texture
		auto normal = snapshotFBO_->addTexture(1, GL_TEXTURE_2D_ARRAY,
											   GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
		snapshotNormal_ = ref_ptr<Texture2DArray>::dynamicCast(normal);
		// create depth texture
		snapshotFBO_->createDepthTexture(GL_TEXTURE_2D_ARRAY,
										 GL_DEPTH_COMPONENT16, GL_FLOAT);
		snapshotDepth_ = ref_ptr<Texture2DArrayDepth>::dynamicCast(snapshotFBO_->depthTexture());
	}

	// TODO: create snapshot shader

	{ // add textures to the billboard state
		auto albedo = ref_ptr<TextureState>::alloc(snapshotAlbedo_, "impostorAlbedo");
		albedo->set_mapping(TextureState::MAPPING_TEXCO);
		albedo->set_mapTo(TextureState::MAP_TO_DIFFUSE);
		joinStates(albedo);

		auto normal = ref_ptr<TextureState>::alloc(snapshotNormal_, "impostorNormal");
		normal->set_mapping(TextureState::MAPPING_CUSTOM);
		normal->set_mapTo(TextureState::MAP_TO_CUSTOM);
		joinStates(normal);

		auto depth = ref_ptr<TextureState>::alloc(snapshotDepth_, "impostorDepth");
		depth->set_mapping(TextureState::MAPPING_CUSTOM);
		depth->set_mapTo(TextureState::MAP_TO_CUSTOM);
		joinStates(depth);
	}
}

void ImpostorBillboard::addSnapshotView(uint32_t viewIdx, const Vec3f &dir, const Vec3f &up) {
	auto &center = meshCenterPoint_;
	auto eye = center + dir * meshBoundsRadius_ * 2.0f; // distance can be tuned
	auto view = Mat4f::lookAtMatrix(center, eye, up);

	float minX = +FLT_MAX, maxX = -FLT_MAX;
	float minY = +FLT_MAX, maxY = -FLT_MAX;
	float minZ = +FLT_MAX, maxZ = -FLT_MAX;

	for (const auto &corner: meshCornerPoints_) {
		// TODO: need transposed multiplication?
		auto viewSpace = view * Vec4f(corner, 1.0f);
		//auto viewSpace = view ^ Vec4f(corner, 1.0f);
		minX = std::min(minX, viewSpace.x);
		maxX = std::max(maxX, viewSpace.x);
		minY = std::min(minY, viewSpace.y);
		maxY = std::max(maxY, viewSpace.y);
		minZ = std::min(minZ, viewSpace.z);
		maxZ = std::max(maxZ, viewSpace.z);
	}

	((Vec3f *) snapshotDirs_->clientData())[viewIdx] = dir;
	((Vec4f *) snapshotOrthoBounds_->clientData())[viewIdx] = Vec4f(minX, maxX, minY, maxY);
	((Vec2f *) snapshotDepthRanges_->clientData())[viewIdx] = Vec2f(minZ, maxZ);
}

void ImpostorBillboard::updateSnapshotViews() {
	// make sure resources were created
	ensureResourcesExist();
	uint32_t viewIdx = 0u;

	// Latitude steps in radians
	std::vector<float> latAngles;
	if (latitudeSteps_ == 0) {
		latAngles.push_back(0.0f);
	} else {
		for (uint32_t i = 0; i < latitudeSteps_; ++i) {
			float frac = static_cast<float>(i) / static_cast<float>(latitudeSteps_);
			latAngles.push_back(frac * math::halfPi<float>());
		}
	}

	for (float lat: latAngles) {
		float y = sin(lat);
		float horizontalRadius = cos(lat); // radius on equator ring

		for (uint32_t i = 0; i < longitudeSteps_; ++i) {
			float lon = static_cast<float>(i) /
						static_cast<float>(longitudeSteps_) * math::twoPi<float>();
			Vec3f dir(
					horizontalRadius * cos(lon),
					y,
					horizontalRadius * sin(lon));
			dir.normalize();
			if (isHemispherical_ && dir.y < 0.0f) {
				// skip southern hemisphere
				continue;
			}
			addSnapshotView(viewIdx++, dir);
		}
	}

	if (hasTopView_) {
		addSnapshotView(viewIdx++, Vec3f::down(), Vec3f::up());
	}
	if (hasBottomView_ && !isHemispherical_) {
		addSnapshotView(viewIdx++, Vec3f::up(), Vec3f::down());
	}

#ifdef DEBUG_SNAPSHOT_VIEWS
	for (uint32_t i = 0; i < numSnapshotViews_; ++i) {
		Vec3f &dir = ((Vec3f *) snapshotDirs_->clientData())[i];
		Vec4f &bounds = ((Vec4f *) snapshotOrthoBounds_->clientData())[i];
		Vec2f &depth = ((Vec2f *) snapshotDepthRanges_->clientData())[i];
		REGEN_INFO("Snapshot view " << i << ": dir=" << dir
									<< ", bounds=" << bounds
									<< ", depth=" << depth);
	}
#endif

	snapshotDirs_->nextStamp();
	snapshotOrthoBounds_->nextStamp();
	snapshotDepthRanges_->nextStamp();
	ssbo_snapshotDirs_->update();
	ssbo_snapshotOrthoBounds_->update();
	ssbo_snapshotDepthRanges_->update();
}

void ImpostorBillboard::createSnapshot() {
	// make sure resources were created
	ensureResourcesExist();
	// TODO: implement this
}

ref_ptr<ImpostorBillboard> ImpostorBillboard::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto parser = ctx.scene();
	auto impostor = ref_ptr<ImpostorBillboard>::alloc();
	// find the original mesh
	auto originalMeshVec = parser->getResources()->getMesh(
			parser,
			input.getValue("original-mesh"));
	if (originalMeshVec.get() == nullptr || originalMeshVec->empty()) {
		REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load original mesh.");
		return {};
	}
	auto originalIndex = input.getValue<GLuint>("original-index", 0u);
	if (originalIndex >= originalMeshVec->size()) {
		REGEN_WARN("Invalid original index '" << originalIndex << "' for '" << input.getDescription() << "'.");
		originalIndex = 0u;
	}
	auto originalMesh = (*originalMeshVec.get())[originalIndex];
	if (input.hasAttribute("depth-offset")) {
		impostor->depthOffset_->setVertex(0, input.getValue<float>("depth-offset", 0.0f));
	}
	impostor->addMesh(originalMesh);
	impostor->updateSnapshotViews();
	impostor->createSnapshot();
	return impostor;
}

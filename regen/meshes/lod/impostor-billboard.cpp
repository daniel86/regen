#include "impostor-billboard.h"
#include "regen/scene/resource-manager.h"
#include "regen/states/state-configurer.h"

using namespace regen;

#undef DEBUG_SNAPSHOT_VIEWS

ImpostorBillboard::ImpostorBillboard()
		: Mesh(GL_POINTS, BUFFER_USAGE_STATIC_DRAW),
		  snapshotState_(ref_ptr<State>::alloc()) {
	depthOffset_ = createUniform<ShaderInput1f>("depthOffset", 0.5f);
	modelOrigin_ = createUniform<ShaderInput3f>("modelOrigin", Vec3f::zero());
	updateAttributes();
	// note: we generally do not need culling when rendering impostor billboards
	//       as they only have a two front face.
	//       this is also important for the billboards to be rendered into shadow maps.
	joinStates(ref_ptr<ToggleState>::alloc(RenderState::CULL_FACE, GL_FALSE));
	setShaderKey("regen.models.impostor");
}

void ImpostorBillboard::createShader(const ref_ptr<StateNode> &parentNode) {
	StateConfigurer shaderConfigurer;
	shaderConfigurer.addNode(parentNode.get());

	// join states of the input mesh, besides its material texture state which we
	// want to bake into the array textures.
	// NOTE: might very well be that some texture mapping techniques will cause problems here.
	//       effectively we loose UV coordinates, so we cannot apply any (uv-mapped) textures from
	//       the original mesh to the impostor.
	for (auto &mesh: meshes_) {
		std::stack<ref_ptr<State>> stateStack;
		for (auto &state: mesh.meshOrig->joined()) {
			stateStack.push(state);
		}
		while (!stateStack.empty()) {
			auto state = stateStack.top();
			stateStack.pop();
			auto *textureState = dynamic_cast<TextureState*>(state.get());
			if (textureState) {
				if (textureState->texture()->targetType() == GL_TEXTURE_BUFFER) {
					// TBOs must be joined, they could be used for instancing of uniforms
					joinStates(state);
				} else {
					// skip (material) texture states, we will bake them into the snapshot textures
					continue;
				}
			}
			else {
				auto *hasInput = dynamic_cast<HasInput*>(state.get());
				if (hasInput) {
					for (auto &input: hasInput->inputContainer()->inputs()) {
						if (!input.in_->isVertexAttribute()) {
							joinShaderInput(input.in_, input.name_);
						}
					}
				}
			}
			for (auto &joined: state->joined()) {
				stateStack.push(joined);
			}
		}
	}

	shaderConfigurer.addState(sharedState_.get());
	shaderConfigurer.addState(this);
	Mesh::createShader(parentNode, shaderConfigurer.cfg());
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
	imitation.meshOrig = mesh;
	imitation.meshCopy = ref_ptr<Mesh>::alloc(mesh);
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

	REGEN_DEBUG("Impostor mesh added -- center: " << meshCenterPoint_
		<< ", radius: " << meshBoundsRadius_
		<< ", min: " << minPosition_
		<< ", max: " << maxPosition_);
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

	REGEN_DEBUG("impostor num snapshots: " << numSnapshotViews_);
}

void ImpostorBillboard::ensureResourcesExist() {
	if (!hasInitializedResources_) {
		createResources();
	}
}

void ImpostorBillboard::createResources() {
	hasInitializedResources_ = true;
	updateNumberOfViews();
	// create camera for the update pass
	snapshotCamera_ = ref_ptr<ArrayCamera>::alloc(numSnapshotViews_);

	{ // create parameters for the shader
		joinShaderInput(depthOffset_);
		joinShaderInput(modelOrigin_);
	}

	{ // create view data arrays
		ssbo_snapshotDirs_ = ref_ptr<SSBO>::alloc("SnapshotDirsData", BUFFER_USAGE_STATIC_DRAW);
		snapshotDirs_ = ref_ptr<ShaderInput4f>::alloc("snapshotDirs", numSnapshotViews_);
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
		// TODO: support specular maps, but only if input mesh uses them!
		// TODO: support depth correction
		auto fbo = ref_ptr<FBO>::alloc(snapshotWidth_, snapshotHeight_, numSnapshotViews_);
		std::vector<GLenum> drawAttachments;

		// create depth texture
		fbo->createDepthTexture(GL_TEXTURE_2D_ARRAY, GL_DEPTH_COMPONENT24, GL_UNSIGNED_INT);
		snapshotDepth_ = ref_ptr<Texture2DArrayDepth>::dynamicCast(fbo->depthTexture());

		// create albedo texture
		auto albedo = fbo->addTexture(1, GL_TEXTURE_2D_ARRAY,
											   GL_RGBA, GL_RGBA8, GL_UNSIGNED_BYTE);
		albedo->set_name("diffuse");
		snapshotAlbedo_ = ref_ptr<Texture2DArray>::dynamicCast(albedo);
		snapshotAlbedo_->begin(RenderState::get());
		snapshotAlbedo_->filter().push(TextureFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
		snapshotAlbedo_->wrapping().push(GL_CLAMP_TO_EDGE);
		snapshotAlbedo_->end(RenderState::get());
		drawAttachments.push_back(GL_COLOR_ATTACHMENT0);

		if (useNormalCorrection_) {
			// create normal texture
			auto normal = fbo->addTexture(1, GL_TEXTURE_2D_ARRAY,
												   GL_RGBA, GL_RGBA8, GL_UNSIGNED_BYTE);
			normal->set_name("normal");
			snapshotNormal_ = ref_ptr<Texture2DArray>::dynamicCast(normal);
			drawAttachments.push_back(GL_COLOR_ATTACHMENT0 + drawAttachments.size());
		}

		// create the FBO state
		snapshotFBO_ = ref_ptr<FBOState>::alloc(fbo);
		// render to albedo + normal
		snapshotFBO_->setDrawBuffers(drawAttachments);
		snapshotFBO_->setClearDepth();
		snapshotFBO_->setClearColor({
			Vec4f(0.0, 0.0, 0.0, 0.0),
			drawAttachments });

		GL_ERROR_LOG();
	}

	for (auto &viewMesh : meshes_) {
		StateConfigurer meshConfigurer;
		meshConfigurer.addState(snapshotState_.get());
		meshConfigurer.addState(snapshotFBO_.get());
		meshConfigurer.addState(snapshotCamera_.get());
		if(viewMesh.drawState.get()) {
			meshConfigurer.addState(viewMesh.drawState.get());
		}
		meshConfigurer.addState(viewMesh.meshOrig.get());
		meshConfigurer.addState(viewMesh.meshCopy.get());
		meshConfigurer.define("NUM_IMPOSTOR_VIEWS", REGEN_STRING(numSnapshotViews_));
		viewMesh.shaderState->createShader(meshConfigurer.cfg(), snapshotShaderKey_);

		viewMesh.meshCopy->joinStates(viewMesh.shaderState);
		viewMesh.meshCopy->updateVAO(meshConfigurer.cfg(), viewMesh.shaderState->shader());
	}

	{ // add textures to the billboard state
		auto albedo = ref_ptr<TextureState>::alloc(snapshotAlbedo_, "impostorAlbedo");
		albedo->set_mapping(TextureState::MAPPING_TEXCO);
		// note: map to color is used for alpha discard to work
		albedo->set_mapTo(TextureState::MAP_TO_COLOR);
		albedo->set_blendMode(BLEND_MODE_MULTIPLY);
		joinStates(albedo);

		if (useNormalCorrection_) {
			auto normal = ref_ptr<TextureState>::alloc(snapshotNormal_, "impostorNormal");
			normal->set_mapping(TextureState::MAPPING_TEXCO);
			normal->set_mapTo(TextureState::MAP_TO_NORMAL);
			// note: we store normal in eye space, so we need to use special transfer function
			normal->set_texelTransfer(TextureState::TEXEL_TRANSFER_WORLD_NORMAL);
			normal->set_blendMode(BLEND_MODE_SRC);
			joinStates(normal);
		}

		if (useDepthCorrection_) {
			//auto depth = ref_ptr<TextureState>::alloc(snapshotDepth_, "impostorDepth");
			//depth->set_mapping(TextureState::MAPPING_CUSTOM);
			//depth->set_mapTo(TextureState::MAP_TO_CUSTOM);
			//joinStates(depth);
		}
	}
}

void ImpostorBillboard::addSnapshotView(uint32_t viewIdx, const Vec3f &dir, const Vec3f &up) {
	// map data pointers
	auto *camView    = (Mat4f*)snapshotCamera_->view()->clientData();
	auto *camViewInv = (Mat4f*)snapshotCamera_->viewInverse()->clientData();
	auto *camProj    = (Mat4f*)snapshotCamera_->projection()->clientData();
	auto *camProjInv = (Mat4f*)snapshotCamera_->projectionInverse()->clientData();
	auto *projParams = (ProjectionParams*)snapshotCamera_->projParams()->clientData();
	auto *camPos     = (Vec3f*)snapshotCamera_->position()->clientData();
	auto *viewDir    = (Vec4f*)snapshotDirs_->clientData();
	auto *viewBounds = (Vec4f*)snapshotOrthoBounds_->clientData();
	auto *viewDepth  = (Vec2f*)snapshotDepthRanges_->clientData();

	// this is an offset of the mesh that translates it to origin.
	// in many cases this will be (0,0,0).
	auto eye = meshCenterPoint_ - dir * meshBoundsRadius_ * 1.5f;
	camView[viewIdx] = Mat4f::lookAtMatrix(eye, dir, up);
	camViewInv[viewIdx] = camView[viewIdx].lookAtInverse();
	auto &view = camView[viewIdx];

	float minX = +FLT_MAX, maxX = -FLT_MAX;
	float minY = +FLT_MAX, maxY = -FLT_MAX;
	float minZ = +FLT_MAX, maxZ = -FLT_MAX;

	for (const auto &corner: meshCornerPoints_) {
		auto viewSpace = view ^ Vec4f(corner, 1.0f);
		minX = std::min(minX, viewSpace.x);
		maxX = std::max(maxX, viewSpace.x);
		minY = std::min(minY, viewSpace.y);
		maxY = std::max(maxY, viewSpace.y);
		minZ = std::min(minZ, -viewSpace.z);
		maxZ = std::max(maxZ, -viewSpace.z);
	}
	const float zPadding = 0.1f * (maxZ - minZ);
	minZ -= zPadding;
	maxZ += zPadding;

	viewDir[viewIdx].xyz_() = -dir;
	viewDir[viewIdx].w = 0.0f; // no w-component, this is a direction vector
	viewBounds[viewIdx] = Vec4f(minX, maxX, minY, maxY);
	viewDepth[viewIdx] = Vec2f(minZ, maxZ);
#ifdef DEBUG_SNAPSHOT_VIEWS
	REGEN_INFO("Snapshot view " << viewIdx << ":"
									<< "\n\tmesh-origin=" << meshCenterPoint_
									<< "\n\teye=" << eye
									<< "\n\tdir=" << -dir
									<< "\n\tbounds=" << viewBounds[viewIdx]
									<< "\n\tdepth=" << viewDepth[viewIdx]);
#endif

	camProj[viewIdx] = Mat4f::orthogonalMatrix(minX, maxX, minY, maxY, minZ, maxZ);
	camProjInv[viewIdx] = camProj[viewIdx].orthogonalInverse();
	projParams[viewIdx].near = minZ;
	projParams[viewIdx].far = maxZ;
	projParams[viewIdx].aspect = abs((maxX - minX) / (maxY - minY));
	projParams[viewIdx].fov = 0.0f; // orthographic projection, no fov
	camPos[viewIdx] = eye;
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
			latAngles.push_back(-frac * math::halfPi<float>());
			if (!isHemispherical_ && latAngles[i] < 0.0f) {
				latAngles.push_back(-latAngles[i]);
			}
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
			if (isHemispherical_ && dir.y > 0.0f) {
				// skip southern hemisphere
				continue;
			}
			addSnapshotView(viewIdx++, dir);
		}
	}

	if (hasTopView_) {
		addSnapshotView(viewIdx++, Vec3f::up(), Vec3f::right());
	}
	if (hasBottomView_ && !isHemispherical_) {
		addSnapshotView(viewIdx++, Vec3f::down(), Vec3f::right());
	}

	snapshotDirs_->nextStamp();
	snapshotOrthoBounds_->nextStamp();
	snapshotDepthRanges_->nextStamp();
	ssbo_snapshotDirs_->update();
	ssbo_snapshotOrthoBounds_->update();
	ssbo_snapshotDepthRanges_->update();
	snapshotCamera_->view()->nextStamp();
	snapshotCamera_->viewInverse()->nextStamp();
	snapshotCamera_->projection()->nextStamp();
	snapshotCamera_->projectionInverse()->nextStamp();
	snapshotCamera_->position()->nextStamp();
	snapshotCamera_->direction()->nextStamp();
	snapshotCamera_->updateViewProjection1();
}

void ImpostorBillboard::createSnapshot() {
	auto rs = RenderState::get();
	// make sure resources were created
	ensureResourcesExist();
	snapshotFBO_->enable(rs);
	// render all meshes into the snapshot FBO
	for (auto &view: meshes_) {
		auto oldNumInstances = view.meshCopy->inputContainer()->numVisibleInstances();
		// make sure only one instance is rendered
		view.meshCopy->inputContainer()->set_numVisibleInstances(1);
		view.shaderState->enable(rs);
		view.meshCopy->draw(rs);
		view.shaderState->disable(rs);
		view.meshCopy->inputContainer()->set_numVisibleInstances(oldNumInstances);
	}
	snapshotFBO_->disable(rs);
	// update mipmaps
	snapshotAlbedo_->begin(rs);
	snapshotAlbedo_->setupMipmaps();
	snapshotAlbedo_->end(rs);
}

ref_ptr<ImpostorBillboard> ImpostorBillboard::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto parser = ctx.scene();
	auto impostor = ref_ptr<ImpostorBillboard>::alloc();
	// find the original mesh
	auto originalMeshVec = parser->getResources()->getMesh(
			parser,
			input.getValue("base-mesh"));
	if (originalMeshVec.get() == nullptr || originalMeshVec->empty()) {
		REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load base mesh.");
		return {};
	}
	if (input.hasAttribute("depth-offset")) {
		impostor->depthOffset_->setVertex(0, input.getValue<float>("depth-offset", 0.0f));
	}
	if (input.hasAttribute("texture-size")) {
		auto textureSize = input.getValue<Vec2ui>("texture-size", Vec2ui(256u, 256u));
		impostor->setSnapshotTextureSize(textureSize.x, textureSize.y);
	}
	if (input.hasAttribute("longitude-steps")) {
		impostor->longitudeSteps_ = input.getValue<uint32_t>("longitude-steps", 8u);
	}
	if (input.hasAttribute("latitude-steps")) {
		impostor->latitudeSteps_ = input.getValue<uint32_t>("latitude-steps", 0u);
	}
	if (input.hasAttribute("hemispherical")) {
		impostor->isHemispherical_ = input.getValue<bool>("hemispherical", true);
	}
	if (input.hasAttribute("top-view")) {
		impostor->hasTopView_ = input.getValue<bool>("top-view", false);
	}
	if (input.hasAttribute("bottom-view")) {
		impostor->hasBottomView_ = input.getValue<bool>("bottom-view", false);
	}
	if (input.hasAttribute("normal-correction")) {
		impostor->useNormalCorrection_ = input.getValue<bool>("normal-correction", true);
	}
	if (input.hasAttribute("depth-correction")) {
		impostor->useDepthCorrection_ = input.getValue<bool>("depth-correction", false);
	}
	if (input.hasAttribute("snapshot-shader")) {
		impostor->snapshotShaderKey_ = input.getValue<std::string>("snapshot-shader", "regen.models.impostor.update");
	}

	// load update state
	auto updateStateNode = input.getFirstChild("update-state");
	if(updateStateNode.get()) {
		for (auto &child: updateStateNode->getChildren()) {
			auto processor = ctx.scene()->getStateProcessor(child->getCategory());
			if (processor.get() == nullptr) {
				REGEN_WARN("No processor registered for '" << child->getDescription() << "'.");
			} else {
				processor->processInput(ctx.scene(), *child.get(), ctx.parent(), impostor->snapshotState());
			}
		}
		input.removeChild(updateStateNode);
	}

	// add meshes to the impostor
	auto indexRange = MeshVector::loadIndexRange(input, "base-mesh");
	if (indexRange.empty()) { indexRange.push_back(0); }
	for (auto &index: indexRange) {
		if (index >= originalMeshVec->size()) {
			REGEN_WARN("Invalid mesh index '" << index << "' for '" << input.getDescription() << "'.");
		} else {
			auto originalMesh = (*originalMeshVec.get())[index];
			impostor->addMesh(originalMesh);
		}
	}

	impostor->updateSnapshotViews();
	impostor->createSnapshot();
	return impostor;
}

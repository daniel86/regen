#include "impostor-billboard.h"
#include "regen/scene/resource-manager.h"
#include "regen/states/state-configurer.h"

using namespace regen;

#undef DEBUG_SNAPSHOT_VIEWS
#define USE_POINT_EXTRUSION
#define USE_IMPOSTOR_MIPMAPS

#ifndef USE_POINT_EXTRUSION
static ref_ptr<Rectangle> getImpostorQuad() {
	static ref_ptr<Rectangle> mesh;
	if (mesh.get() == nullptr) {
		Rectangle::Config cfg;
		cfg.centerAtOrigin = false;
		cfg.isNormalRequired = false;
		cfg.isTangentRequired = false;
		cfg.isTexcoRequired = false;
		cfg.levelOfDetails = {0};
		cfg.posScale = Vec3f::one();
		cfg.rotation = Vec3f(0.5 * M_PI, 0.0f, 0.0f);
		cfg.texcoScale = Vec2f(1.0);
		cfg.translation = Vec3f(-0.5f, -0.5f, 0.0f);
		mesh = ref_ptr<Rectangle>::alloc(cfg);
		mesh->updateAttributes();
	}
	return mesh;
}
#endif

ImpostorBillboard::ImpostorBillboard()
#ifdef USE_POINT_EXTRUSION
		: Mesh(GL_POINTS, BufferUpdateFlags::NEVER),
#else
		: Mesh(getImpostorQuad()),
#endif
		  snapshotState_(ref_ptr<State>::alloc()) {
	depthOffset_ = createUniform<ShaderInput1f>("depthOffset", 0.5f);
	modelOrigin_ = createUniform<ShaderInput3f>("modelOrigin", Vec3f::zero());
#ifdef USE_POINT_EXTRUSION
	shaderDefine("USE_POINT_EXTRUSION", "TRUE");
	updateExtrudeAttributes();
#endif
	// note: we generally do not need culling when rendering impostor billboards
	//       as they only have a two front face.
	//       this is also important for the billboards to be rendered into shadow maps.
	joinStates(ref_ptr<ToggleState>::alloc(RenderState::CULL_FACE, false));
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
		for (auto &state: *mesh.meshOrig->joined().get()) {
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
				for (auto &input: state->inputs()) {
					if (!input.in_->isVertexAttribute()) {
						setInput(input.in_, input.name_);
					}
				}
			}
			for (auto &joined: *state->joined().get()) {
				stateStack.push(joined);
			}
		}
	}

	shaderConfigurer.addState(sharedState().get());
	shaderConfigurer.addState(this);
	Mesh::createShader(parentNode, shaderConfigurer.cfg());
}

void ImpostorBillboard::updateExtrudeAttributes() {
#ifdef USE_POINT_EXTRUSION
	if (hasAttributes_) return;
	auto positionIn = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	Vec3f posData[1] = {Vec3f(0.0f, 0.0f, 0.0f)};
	positionIn->setVertexData(1, (byte *) posData);

	setInput(positionIn);
	updateVertexData();
	hasAttributes_ = true;
#endif
}

void ImpostorBillboard::addMesh(const ref_ptr<Mesh> &mesh, const ref_ptr<State> &drawState) {
	// TODO: Consider the case of different materials/textures as we allow an impostor to be composed
	//       of multiple meshes below. I think we really need a pass in update for each mesh/material combination.
	auto &imitation = meshes_.emplace_back();
	imitation.meshOrig = mesh;
	imitation.meshCopy = ref_ptr<Mesh>::alloc(mesh);
	if (mesh->hasMaterial()) {
		imitation.meshCopy->joinStates(mesh->material());
	}
	imitation.drawState = drawState;
	imitation.shaderState = ref_ptr<ShaderState>::alloc();

	if (meshes_.size() == 1) {
		minPosition_ = mesh->minPosition();
		maxPosition_ = mesh->maxPosition();
	} else {
		minPosition_.setMin(mesh->minPosition());
		maxPosition_.setMax(mesh->maxPosition());
	}
	Bounds<Vec3f> meshBounds = Bounds<Vec3f>::create(minPosition_, maxPosition_);
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
	if (numLatitudeSteps_ == 0) {
		numSnapshotViews_ = numLongitudeSteps_;
	} else {
		if (isHemispherical_) {
			numSnapshotViews_ = numLatitudeSteps_ * numLongitudeSteps_;
		} else {
			numSnapshotViews_ = (numLatitudeSteps_ * 2 - 1) * numLongitudeSteps_;
		}
	}
	if (hasTopView_) numSnapshotViews_++;
	if (hasBottomView_ && !isHemispherical_) numSnapshotViews_++;
	shaderDefine("NUM_IMPOSTOR_VIEWS", REGEN_STRING(numSnapshotViews_));
	if (isHemispherical_) {
		shaderDefine("IS_HEMISPHERICAL", "TRUE");
	}

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
	snapshotCamera_ = ref_ptr<ArrayCamera>::alloc(numSnapshotViews_, BufferUpdateFlags::NEVER);
	defaultNormals_.resize(numSnapshotViews_);

	longitudeStepSize_ = math::twoPi<float>() / static_cast<float>(numLongitudeSteps_);
	latitudeStepSize_ = math::halfPi<float>() / static_cast<float>(std::max(1u,numLatitudeSteps_));

	{ // create parameters for the shader
		setInput(depthOffset_);
		setInput(modelOrigin_);

		setInput(createUniform<ShaderInput1i>("numLongitudeSteps", numLongitudeSteps_));
		setInput(createUniform<ShaderInput1f>("longitudeStep", longitudeStepSize_));
		setInput(createUniform<ShaderInput1f>("longitudeHalfStep", longitudeStepSize_*0.5f));

		setInput(createUniform<ShaderInput1i>("numLatitudeSteps", std::max(1u,numLatitudeSteps_)));
		setInput(createUniform<ShaderInput1f>("latitudeStep", latitudeStepSize_));
		setInput(createUniform<ShaderInput1f>("latitudeHalfStep", latitudeStepSize_*0.5f));
	}

	{ // create view data arrays
		impostorBuffer_ = ref_ptr<SSBO>::alloc("ImpostorBuffer", BufferUpdateFlags::NEVER);
		snapshotDirs_ = ref_ptr<ShaderInput4f>::alloc("snapshotDirs", numSnapshotViews_);
		snapshotDirs_->setUniformUntyped();
		impostorBuffer_->addStagedInput(snapshotDirs_);

		snapshotOrthoBounds_ = ref_ptr<ShaderInput4f>::alloc("snapshotOrthoBounds", numSnapshotViews_);
		snapshotOrthoBounds_->setUniformUntyped();
		impostorBuffer_->addStagedInput(snapshotOrthoBounds_);

		snapshotDepthRanges_ = ref_ptr<ShaderInput2f>::alloc("snapshotDepthRanges", numSnapshotViews_);
		snapshotDepthRanges_->setUniformUntyped();
		impostorBuffer_->addStagedInput(snapshotDepthRanges_);

		snapshotState_->setInput(impostorBuffer_);
		setInput(impostorBuffer_);
	}

	{ // create the snapshot FBO
		// TODO: Add support for depth correction?
		//       - Depth texture is already created when snapshot FBO is created.
		//       - This should be given to draw shader, there it must be sampled, and gl_FragCoord.z
		//         must be adjusted accordingly, which is very expensive.
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
#ifdef USE_IMPOSTOR_MIPMAPS
		snapshotAlbedo_->set_filter(TextureFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
#else
		snapshotAlbedo_->set_filter(TextureFilter(GL_LINEAR, GL_LINEAR));
#endif
		snapshotAlbedo_->set_wrapping(TextureWrapping::create(GL_CLAMP_TO_EDGE));
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
	}

	for (auto &viewMesh : meshes_) {
		if (viewMesh.meshOrig->hasMaterial()) {
			viewMesh.meshCopy->setMaterial(viewMesh.meshOrig->material());
		}
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
		meshConfigurer.define("USE_GS_LAYERED_RENDERING", "TRUE");
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
	GL_ERROR_LOG();
}

void ImpostorBillboard::addSnapshotView(uint32_t viewIdx, const Vec3f &dir, const Vec3f &up) {
	// this is an offset of the mesh that translates it to origin.
	// in many cases this will be (0,0,0).
	auto eye = meshCenterPoint_ - dir * meshBoundsRadius_ * 1.5f;
	snapshotCamera_->setView(viewIdx, Mat4f::lookAtMatrix(eye, dir, up));
	auto &view = snapshotCamera_->view(viewIdx);
	snapshotCamera_->setViewInverse(viewIdx, view.lookAtInverse());
	Vec4f defaultNormal = view ^ Vec4f::create(up, 0.0f);
	defaultNormals_[viewIdx] = defaultNormal.xyz() * 0.5f + Vec3f::create(0.5f);

	float minX = +FLT_MAX, maxX = -FLT_MAX;
	float minY = +FLT_MAX, maxY = -FLT_MAX;
	float minZ = +FLT_MAX, maxZ = -FLT_MAX;

	for (const auto &corner: meshCornerPoints_) {
		auto viewSpace = view ^ Vec4f::create(corner, 1.0f);
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

	m_viewDir_[viewIdx].xyz() = -dir;
	m_viewDir_[viewIdx].w = 0.0f; // no w-component, this is a direction vector
	m_viewBounds_[viewIdx] = Vec4f(minX, maxX, minY, maxY);
	m_viewDepth_[viewIdx] = Vec2f(minZ, maxZ);
#ifdef DEBUG_SNAPSHOT_VIEWS
	REGEN_INFO("Snapshot view " << viewIdx << ":"
									<< "\n\tmesh-origin=" << meshCenterPoint_
									<< "\n\teye=" << eye
									<< "\n\tdir=" << -dir
									<< "\n\tbounds=" << viewBounds[viewIdx]
									<< "\n\tdepth=" << viewDepth[viewIdx]);
#endif

	snapshotCamera_->setProjection(viewIdx,
			Mat4f::orthogonalMatrix(minX, maxX, minY, maxY, minZ, maxZ));
	snapshotCamera_->setProjectionInverse(viewIdx,
			snapshotCamera_->projection(viewIdx).orthogonalInverse());
	ProjectionParams params = snapshotCamera_->projParams(viewIdx);
	params.near = minZ;
	params.far = maxZ;
	params.aspect = abs((maxX - minX) / (maxY - minY));
	params.fov = 0.0f; // orthographic projection, no fov
	snapshotCamera_->setProjParams(viewIdx, params);
	snapshotCamera_->setPosition(viewIdx, eye);
}

int ImpostorBillboard::getViewIdx(const Vec3f &dir) {
	// Compute the longitude segment
    float lonAngle = atan2(dir.z, dir.x); // returns [-π, π]
    lonAngle = fmod((lonAngle + math::twoPi<float>()), math::twoPi<float>());
    int lonIdx = int((lonAngle + 0.5 * longitudeStepSize_) / longitudeStepSize_);
    lonIdx = math::clamp<int>(lonIdx, 0, numLongitudeSteps_ - 1);
	// Compute the latitude segment
    float latAngle = atan2(-dir.y, Vec2f(dir.x, dir.z).length());
	auto latIdx = static_cast<int>(((latAngle + latitudeStepSize_ * 0.5f) / latitudeStepSize_));
	if (isHemispherical_) {
		// map southern hemisphere to the equator
		latIdx = math::clamp<int>(latIdx, 0, static_cast<int>(numLatitudeSteps_)-1);
	} else if (latIdx < 0) {
		// southern hemisphere
		latIdx = (numLatitudeSteps_-1) + std::min(-latIdx, static_cast<int>(numLatitudeSteps_-1));
	}
	// Finally compute view index
	return latIdx * numLongitudeSteps_ + lonIdx;
}

void ImpostorBillboard::updateSnapshotViews() {
	// make sure resources were created
	ensureResourcesExist();
	uint32_t viewIdx = 0u;

	// Latitude steps in radians
	std::vector<float> latAngles;
	if (numLatitudeSteps_ == 0) {
		latAngles.push_back(0.0f);
		numLatitudeSteps_ = 1;
	} else {
		for (uint32_t i = 0; i < numLatitudeSteps_; ++i) {
			float frac = static_cast<float>(i) / static_cast<float>(numLatitudeSteps_);
			latAngles.push_back(-frac * math::halfPi<float>());
		}
		if (!isHemispherical_) {
			for (uint32_t i = 0; i < numLatitudeSteps_; ++i) {
				if (latAngles[i] < 0.0f) {
					latAngles.push_back(-latAngles[i]);
				}
			}
		}
	}

	// map data pointers
	auto mappedBuffer1 = snapshotDirs_->mapClientData<Vec4f>(BUFFER_GPU_WRITE);
	auto mappedBuffer2 = snapshotOrthoBounds_->mapClientData<Vec4f>(BUFFER_GPU_WRITE);
	auto mappedBuffer3 = snapshotDepthRanges_->mapClientData<Vec2f>(BUFFER_GPU_WRITE);
	m_viewDir_ = mappedBuffer1.w.data();
	m_viewBounds_ = mappedBuffer2.w.data();
	m_viewDepth_ = mappedBuffer3.w.data();

	for (float lat: latAngles) {
		float y = sin(lat);
		float horizontalRadius = cos(lat); // radius on equator ring

		for (uint32_t i = 0; i < numLongitudeSteps_; ++i) {
			float lon = static_cast<float>(i) /
						static_cast<float>(numLongitudeSteps_) * math::twoPi<float>();
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

	mappedBuffer1.unmap();
	mappedBuffer2.unmap();
	mappedBuffer3.unmap();
	impostorBuffer_->clientBuffer()->swapData();

	impostorBuffer_->update();
	snapshotCamera_->updateViewProjection1();
	snapshotCamera_->updateShaderData(0.0f);
}

void ImpostorBillboard::createSnapshot() {
	auto rs = RenderState::get();
	// make sure resources were created
	ensureResourcesExist();
	snapshotFBO_->enable(rs);

	if(snapshotNormal_.get()) {
		// Clear the normal textures individually, as each of them may have a different
		// background color it requires for interpolation.
		for (uint32_t arrayIndex = 0; arrayIndex < numSnapshotViews_; ++arrayIndex) {
			Vec3f &f_clearNormal = defaultNormals_[arrayIndex];
			glClearTexSubImage(snapshotNormal_->textureBind().id_,
				0, 0, 0, arrayIndex, // level, x, y, z offset
				snapshotNormal_->width(),
				snapshotNormal_->height(), 1, // width, height, depth
				GL_RGB, GL_FLOAT,
				&f_clearNormal.x);
		}
	}

	// render all meshes into the snapshot FBO
	for (auto &view: meshes_) {
		auto oldNumInstances = view.meshCopy->numVisibleInstances();
		// make sure only one instance is rendered
		view.meshCopy->set_numVisibleInstances(1);
		view.shaderState->enable(rs);
		view.meshCopy->draw(rs);
		view.shaderState->disable(rs);
		view.meshCopy->set_numVisibleInstances(oldNumInstances);
	}
	snapshotFBO_->disable(rs);
#ifdef USE_IMPOSTOR_MIPMAPS
	snapshotAlbedo_->updateMipmaps();
#endif
}

ref_ptr<ImpostorBillboard> ImpostorBillboard::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto parser = ctx.scene();
	auto impostor = ref_ptr<ImpostorBillboard>::alloc();
	// find the original mesh
	auto originalMeshVec = parser->getResources()->getMesh(
			parser,
			input.getValue("base-mesh"));
	if (originalMeshVec.get() == nullptr || originalMeshVec->meshes().empty()) {
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
		impostor->numLongitudeSteps_ = input.getValue<uint32_t>("longitude-steps", 8u);
	}
	if (input.hasAttribute("latitude-steps")) {
		impostor->numLatitudeSteps_ = input.getValue<uint32_t>("latitude-steps", 0u);
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

	// Add meshes to the impostor
	auto indexRange = CompositeMesh::loadIndexRange(input, "base-mesh");
	if (indexRange.empty()) { indexRange.push_back(0); }
	for (auto &index: indexRange) {
		if (index >= originalMeshVec->meshes().size()) {
			REGEN_WARN("Invalid mesh index '" << index << "' for '" << input.getDescription() << "'.");
		} else {
			auto originalMesh = originalMeshVec->meshes()[index];
			impostor->addMesh(originalMesh);
		}
	}

	impostor->updateSnapshotViews();
	impostor->createSnapshot();
	return impostor;
}

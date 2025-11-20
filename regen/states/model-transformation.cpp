#include <stack>
#include "model-transformation.h"
#include "regen/objects/composite-mesh.h"
#include "../simulation/boids-cpu.h"
#include "../simulation/boids-gpu.h"
#include "regen/animation/transform-animation.h"
#include "regen/scene/value-generator.h"
#include "regen/textures/height-map.h"

using namespace regen;

ModelTransformation::ModelTransformation(int tfMode, const BufferUpdateFlags &tfUpdateFlags)
		: State(),
		  tfMode_(tfMode),
		  tfUpdateFlags_(tfUpdateFlags) {
	modelMat_ = ref_ptr<ShaderInputMat4>::alloc("modelMatrix");
	modelOffset_ = ref_ptr<ShaderInput4f>::alloc("modelOffset");

	modelMat_->setUniformData(Mat4f::identity());
	modelOffset_->setUniformData(Vec4f(0.0f, 0.0f, 0.0f, 1.0f));

	modelMat_->setSchema(InputSchema::transform());
	modelOffset_->setSchema(InputSchema::position());

	initBufferContainer();
}

ModelTransformation::ModelTransformation(const ref_ptr<ShaderInput4f> &offset, const BufferUpdateFlags &tfUpdateFlags)
		: State(),
		  tfMode_(TF_OFFSET),
		  tfUpdateFlags_(tfUpdateFlags) {
	modelOffset_ = offset;
	modelMat_ = ref_ptr<ShaderInputMat4>::alloc("modelMatrix");
	modelMat_->setUniformData(Mat4f::identity());
	modelMat_->setSchema(InputSchema::transform());
	initBufferContainer();
}

ModelTransformation::ModelTransformation(const ref_ptr<ShaderInputMat4> &mat, const BufferUpdateFlags &tfUpdateFlags)
		: State(),
		  tfMode_(TF_MATRIX),
		  tfUpdateFlags_(tfUpdateFlags) {
	modelMat_ = mat;
	modelMat_->setSchema(InputSchema::transform());

	modelOffset_ = ref_ptr<ShaderInput4f>::alloc("modelOffset");
	modelOffset_->setUniformData(Vec4f(0.0f, 0.0f, 0.0f, 1.0f));

	initBufferContainer();
}

void ModelTransformation::initBufferContainer() {
	tfBuffer_ = ref_ptr<BufferContainer>::alloc("ModelTransformation", tfUpdateFlags_);
	if (tfMode_ & TF_MATRIX) {
		tfBuffer_->addInput(modelMat_);
		tfClientBuffer_ = modelMat_->clientBuffer();
	} else {
		tfClientBuffer_ = modelOffset_->clientBuffer();
	}
	if (tfMode_ & TF_OFFSET) {
		tfBuffer_->addInput(modelOffset_);
	}
	joinStates(tfBuffer_);
}

void ModelTransformation::setModelMat(const Mat4f *mat) {
	auto mapped = modelMat_->mapClientData<Mat4f>(BUFFER_GPU_WRITE);
	std::memcpy(mapped.w.data(), mat, modelMat_->clientBuffer()->dataSize());
}

void ModelTransformation::setModelMat(uint32_t idx, const Mat4f &mat) {
	modelMat_->setVertex(idx, mat);
}

void ModelTransformation::setModelOffset(uint32_t idx, const Vec3f &offset) {
	modelOffset_->setVertex3(idx, offset);
}

PositionReader ModelTransformation::position(uint32_t idx) const {
	return {
		hasModelMat() ? modelMat().get() : nullptr,
		hasModelOffset() ? modelOffset().get() : nullptr,
		idx};
}

void ModelTransformation::enable(RenderState *rs) {
	if (isAudioSource()) {
		boost::posix_time::ptime time(
				boost::posix_time::microsec_clock::local_time());
		double dt = ((double) (time - lastTime_).total_microseconds()) / 1000.0;
		lastTime_ = time;

		if (dt > 1e-6) {
			auto val = modelMat_->getVertex(0);
			audioSource_->set3f(AL_POSITION, val.r.position());
		}
	}
	State::enable(rs);
}

static void transformMatrix(
		scene::SceneLoader *scene,
		const ref_ptr<scene::SceneInputNode> &input,
		const std::string &target,
		Mat4f &mat,
		const Vec3f &value) {
	if (target == "translate") {
		mat.x[12] += value.x;
		mat.x[13] += value.y;
		mat.x[14] += value.z;
	} else if (target == "scale") {
		mat.scale(value);
	} else if (target == "rotate") {
		Quaternion q(0.0, 0.0, 0.0, 1.0);
		q.setEuler(value.x, value.y, value.z);
		mat *= q.calculateMatrix();
	} else if (target == "height") {
		if (input->hasAttribute("map")) {
			auto tex2D = scene->getResources()->getTexture2D(scene, input->getValue("map"));
			if (!tex2D.get()) {
				REGEN_WARN("Unable to find height map for " << input->getDescription() << ".");
				return;
			}
			auto heightMap = dynamic_cast<HeightMap*>(tex2D.get());
			if (!heightMap) {
				REGEN_WARN("Texture for height map is not a height map for " << input->getDescription() << ".");
				return;
			}
			heightMap->ensureTextureData();
			mat.x[13] += heightMap->sampleHeight(Vec2f(mat.x[12], mat.x[14]));
		}
	} else {
		REGEN_WARN("Unknown distribute target '" << target << "'.");
	}
}

static void transformOffset(
		const std::string &target, Vec4f &offset, const Vec3f &value) {
	if (target == "translate") {
		offset.x += value.x;
		offset.y += value.y;
		offset.z += value.z;
	} else if (target == "scale") {
		offset.x *= value.x;
		offset.y *= value.y;
		offset.z *= value.z;
	} else if (target == "rotate") {
		REGEN_WARN("Cannot rotate offset '" << target << "'.");
	} else {
		REGEN_WARN("Unknown distribute target '" << target << "'.");
	}
}

struct PlaneCell {
	Vec2f position;
	Vec2f uv;
	Vec2f size;
	float density;
};

struct PlaneCellWeights {
	float left = 0.0f;
	float right = 0.0f;
	float top = 0.0f;
	float bottom = 0.0f;
};

struct InstancePlaneGenerator {
	float areaMaxHeight = 0.0f;
	Vec2f areaSize = Vec2f::create(10.0f);
	float objMinScale = 0.6f;
	float objMaxScale = 1.0f;
	float objPosVariation = 0.0f;
	float objDensity = 1.0f;
	Vec2f objSize = Vec2f::one();
	Vec2f ws_cellSize = Vec2f::one();
	Vec2f ts_cellSize = Vec2f::create(0.1f);
	Vec2f cellHalfSize = Vec2f::create(0.5f);
	Vec3f cellWorldOffset = Vec3f::zero();
	std::vector<Mat4f> instanceData;
	ref_ptr<Texture2D> maskTexture;
	ref_ptr<Texture2D> heightMap;
	ref_ptr<Texture> matWeightMap;
	ref_ptr<ImageData> maskData;
	ref_ptr<ImageData> heightData;
	ref_ptr<ImageData> matWeightData;
	uint32_t maskIndex = 0u;
	float maskThreshold = 0.1f; // threshold for mask texture
	//
	PlaneCell *cells = nullptr;
	unsigned int cellCountX = 0;
	unsigned int cellCountY = 0;
	unsigned int numCells = 0;
	int materialIdx = 0;

	bool hasGroundMaterial() const {
		return matWeightMap.get();
	}
};

static void makeInstance(InstancePlaneGenerator &generator, PlaneCell &cell) {
	float density;
	ref_ptr<Texture> tex;
	if (generator.hasGroundMaterial()) {
		tex = generator.matWeightMap;
	} else if (generator.maskData.get()) {
		tex = generator.maskTexture;
	} else {
		return; // no mask or material map, nothing to do
	}
	if (tex->format() == GL_RGBA) {
		density = tex->sampleLinear<Vec4f,4>(cell.uv, generator.maskData)[generator.maskIndex];
	} else if (tex->format() == GL_RGB) {
		density = tex->sampleLinear<Vec3f,3>(cell.uv, generator.maskData)[generator.maskIndex];
	} else if (tex->format() == GL_RG) {
		density = tex->sampleLinear<Vec2f,2>(cell.uv, generator.maskData)[generator.maskIndex];
	} else {
		density = tex->sampleLinear<float,1>(cell.uv, generator.maskData);
	}
	if (generator.hasGroundMaterial()) {
		if (density < 0.5f) return;
	} else {
		if (density < generator.maskThreshold) return;
	}

	auto &instanceMat = generator.instanceData.emplace_back(Mat4f::identity());

	// apply scaling, randomize between min and max scale
	auto sizeFactor = math::random<float>() * cell.density;
	auto scale = generator.objMinScale + sizeFactor * (generator.objMaxScale - generator.objMinScale);
	instanceMat.scale(Vec3f::create(scale));

	// apply random rotation around y axis
	auto angle = math::random<float>() * 2.0f * M_PI;
	Quaternion q(0.0, 0.0, 0.0, 1.0);
	q.setEuler(angle, 0.0f, 0.0f);
	instanceMat *= q.calculateMatrix();

	// translate to cell position
	Vec3f pos = Vec3f(cell.position.x, 0.0f, cell.position.y);
	Vec2f halfArea = generator.areaSize * 0.5f;
	if (generator.objPosVariation > 0.0f) {
		float dx = generator.objPosVariation * (math::random<float>() - 0.5f) * 2.0f;
		float dz = generator.objPosVariation * (math::random<float>() - 0.5f) * 2.0f;
		pos.x += dx;
		pos.z += dz;
		// clamp to area size
		pos.x = std::max(-halfArea.x, std::min(pos.x, halfArea.x));
		pos.z = std::max(-halfArea.y, std::min(pos.z, halfArea.y));
		cell.uv.x += dx / generator.areaSize.x;
		cell.uv.y += dz / generator.areaSize.y;
		cell.uv.x = std::max(0.0f, std::min(cell.uv.x, 1.0f));
		cell.uv.y = std::max(0.0f, std::min(cell.uv.y, 1.0f));
	}
	pos += generator.cellWorldOffset;
	instanceMat.x[12] += pos.x;
	instanceMat.x[13] += pos.y;
	instanceMat.x[14] += pos.z;
	// translate to height map position
	if (generator.heightData.get()) {
		instanceMat.x[13] += generator.areaMaxHeight * generator.heightMap->sampleLinear<float,1>(
				cell.uv, generator.heightData);
	}
}

static void makeInstances(InstancePlaneGenerator &generator,
						  const PlaneCell &rootCell, const PlaneCellWeights &rootWeights) {
	std::stack<std::pair<PlaneCell, PlaneCellWeights>> stack;
	stack.emplace(rootCell, rootWeights);
	auto clampedDensity = math::clamp(generator.objDensity, 0.1f, generator.objDensity);

	while (!stack.empty()) {
		auto &pair = stack.top();
		auto cell = pair.first;
		auto weights = pair.second;
		stack.pop();

		// make an instance at the cell center
		makeInstance(generator, cell);

		// subdivide the cell into four quadrants in case half of the cell is big enough
		auto subdivideThreshold = generator.objSize * generator.objMaxScale * 2.5f / clampedDensity;
		if (cell.size.x > subdivideThreshold.x && cell.size.y > subdivideThreshold.y) {
			auto &bottomLeft = stack.emplace();
			auto &bottomRight = stack.emplace();
			auto &topRight = stack.emplace();
			auto &topLeft = stack.emplace();
			auto subdividedSize = cell.size * 0.5f;
			auto halfCellDensity = cell.density * 0.5f;
			PlaneCell *subdivideCells[4] = {
					&bottomLeft.first, &bottomRight.first, &topRight.first, &topLeft.first
			};

			// compute center position of subdivide cells
			bottomLeft.first.position = cell.position - cell.size * 0.25f;
			topRight.first.position = cell.position + cell.size * 0.25f;
			bottomRight.first.position = cell.position + Vec2f(cell.size.x * 0.25f, -cell.size.y * 0.25f);
			topLeft.first.position = cell.position + Vec2f(-cell.size.x * 0.25f, cell.size.y * 0.25f);
			// compute density at the center
			bottomLeft.first.density = halfCellDensity + (weights.bottom + weights.left) * 0.25f;
			bottomRight.first.density = halfCellDensity + (weights.bottom + weights.right) * 0.25f;
			topRight.first.density = halfCellDensity + (weights.top + weights.right) * 0.25f;
			topLeft.first.density = halfCellDensity + (weights.top + weights.left) * 0.25f;
			// compute uv coordinate, and set the size to half size of parent cell
			for (auto &i: subdivideCells) {
				auto &subdivideCell = *i;
				subdivideCell.size = subdividedSize;
			}
			auto uvOffset = cell.size / generator.areaSize;
			bottomLeft.first.uv = cell.uv - Vec2f(0.25f, 0.25f) * uvOffset;
			bottomRight.first.uv = cell.uv + Vec2f(0.25f, -0.25f) * uvOffset;
			topRight.first.uv = cell.uv + Vec2f(0.25f, 0.25f) * uvOffset;
			topLeft.first.uv = cell.uv - Vec2f(0.25f, -0.25f) * uvOffset;

			bottomLeft.first.uv.x = std::max(0.0f, std::min(bottomLeft.first.uv.x, 1.0f));
			bottomLeft.first.uv.y = std::max(0.0f, std::min(bottomLeft.first.uv.y, 1.0f));
			bottomRight.first.uv.x = std::max(0.0f, std::min(bottomRight.first.uv.x, 1.0f));
			bottomRight.first.uv.y = std::max(0.0f, std::min(bottomRight.first.uv.y, 1.0f));
			topRight.first.uv.x = std::max(0.0f, std::min(topRight.first.uv.x, 1.0f));
			topRight.first.uv.y = std::max(0.0f, std::min(topRight.first.uv.y, 1.0f));
			topLeft.first.uv.x = std::max(0.0f, std::min(topLeft.first.uv.x, 1.0f));
			topLeft.first.uv.y = std::max(0.0f, std::min(topLeft.first.uv.y, 1.0f));

			// compute weights for subdivide cells
			bottomLeft.second.left = 0.75f * weights.left + 0.25f * weights.bottom;
			bottomLeft.second.bottom = 0.75f * weights.bottom + 0.25f * weights.left;
			bottomLeft.second.top = 0.5f * cell.density + 0.5f * weights.left;
			bottomLeft.second.right = 0.5f * cell.density + 0.5f * weights.bottom;

			bottomRight.second.right = 0.75f * weights.right + 0.25f * weights.bottom;
			bottomRight.second.bottom = 0.75f * weights.bottom + 0.25f * weights.right;
			bottomRight.second.top = 0.5f * cell.density + 0.5f * weights.right;
			bottomRight.second.left = 0.5f * cell.density + 0.5f * weights.bottom;

			topRight.second.right = 0.75f * weights.right + 0.25f * weights.top;
			topRight.second.top = 0.75f * weights.top + 0.25f * weights.right;
			topRight.second.left = 0.5f * cell.density + 0.5f * weights.right;
			topRight.second.bottom = 0.5f * cell.density + 0.5f * weights.top;

			topLeft.second.left = 0.75f * weights.left + 0.25f * weights.top;
			topLeft.second.top = 0.75f * weights.top + 0.25f * weights.left;
			topLeft.second.right = 0.5f * cell.density + 0.5f * weights.left;
			topLeft.second.bottom = 0.5f * cell.density + 0.5f * weights.top;
		}
	}
}

static void makeInstances(InstancePlaneGenerator &generator, unsigned int i, unsigned int j) {
	auto &cell = generator.cells[j * generator.cellCountX + i];
	if (cell.density < generator.maskThreshold) {
		return;
	}
	// read weights of adjacent cells
	PlaneCellWeights weights;
	if (i > 0) {
		weights.left = generator.cells[j * generator.cellCountX + i - 1].density;
	}
	if (i < generator.cellCountX - 1) {
		weights.right = generator.cells[j * generator.cellCountX + i + 1].density;
	}
	if (j > 0) {
		weights.top = generator.cells[(j - 1) * generator.cellCountX + i].density;
	}
	if (j < generator.cellCountY - 1) {
		weights.bottom = generator.cells[(j + 1) * generator.cellCountX + i].density;
	}
	makeInstances(generator, cell, weights);
}

static GLuint transformMatrixPlane(
		scene::SceneLoader *scene,
		scene::SceneInputNode &input,
		const ref_ptr<ModelTransformation> &tf,
		GLuint numInstances) {
	auto areaSize = input.getValue<Vec2f>("area-size", Vec2f::create(10.0f));
	auto areaHalfSize = areaSize * 0.5f;

	InstancePlaneGenerator generator;
	generator.areaSize = areaSize;
	generator.areaMaxHeight = input.getValue<float>("area-max-height", 0.0f);
	generator.objMinScale = input.getValue<float>("obj-min-scale", 0.6f);
	generator.objMaxScale = input.getValue<float>("obj-max-scale", 1.0f);
	generator.objPosVariation = input.getValue<float>("obj-pos-variation", 0.0f);
	generator.objDensity = input.getValue<float>("obj-density", 1.0f);
	generator.ws_cellSize = input.getValue<Vec2f>("cell-size", Vec2f::one());
	generator.cellHalfSize = Vec2f(generator.ws_cellSize.x, generator.ws_cellSize.y) * 0.5f;
	generator.cellWorldOffset = input.getValue<Vec3f>("cell-offset", Vec3f::zero());
	if (input.hasAttribute("area-mask-texture")) {
		generator.maskIndex = input.getValue<uint32_t>("area-mask-index", 0);
		generator.maskThreshold = input.getValue<float>("area-mask-threshold", 0.1f);
		generator.maskTexture = scene->getResource<Texture2D>(input.getValue("area-mask-texture"));
		if (generator.maskTexture.get()) {
			generator.maskTexture->ensureTextureData();
			generator.maskData = generator.maskTexture->textureData();
		} else {
			REGEN_WARN("Unable to load mask texture.");
		}
	}
	if (input.hasAttribute("material-weight-map")) {
		uint32_t materialIdx = input.getValue<uint32_t>("material-index", 0);
		// each weight texture has 4 material channels.
		uint32_t materialTextureIdx = materialIdx / 4;
		auto materialTextureName = REGEN_STRING(
			input.getValue("material-weight-map") << "-" << materialTextureIdx);
		generator.matWeightMap = scene->getResource<Texture>(materialTextureName);
		if (generator.matWeightMap.get()) {
			generator.materialIdx = materialIdx % 4;
			generator.matWeightMap->ensureTextureData();
			generator.matWeightData = generator.matWeightMap->textureData();
		} else {
			REGEN_WARN("Unable to load material weight map texture for '" << input.getDescription() << "'.");
		}
	}
	if (input.hasAttribute("area-height-texture")) {
		generator.heightMap = scene->getResource<Texture2D>(input.getValue("area-height-texture"));
		if (generator.heightMap.get()) {
			generator.heightMap->ensureTextureData();
			generator.heightData = generator.heightMap->textureData();
		} else {
			REGEN_WARN("Unable to load height map texture.");
		}
	}
	// get object bounds
	auto compositeMesh = scene->getResource<CompositeMesh>(input.getValue("obj-mesh"));
	if (compositeMesh.get() && !compositeMesh.get()->meshes().empty()) {
		auto &meshVec = compositeMesh->meshes();
		auto firstMesh = meshVec[0];
		Bounds<Vec3f> bounds;
		bounds.min = firstMesh->minPosition();
		bounds.max = firstMesh->maxPosition();
		for (size_t i = 1; i < meshVec.size(); i++) {
			auto mesh = meshVec[i];
			bounds.min.setMin(mesh->minPosition());
			bounds.max.setMax(mesh->maxPosition());
		}
		generator.objSize.x = bounds.max.x - bounds.min.x;
		generator.objSize.y = bounds.max.z - bounds.min.z;
	}

	// round up to the next cell count
	generator.cellCountX = std::ceil(areaSize.x / generator.ws_cellSize.x);
	generator.cellCountY = std::ceil(areaSize.y / generator.ws_cellSize.y);
	generator.numCells = generator.cellCountX * generator.cellCountY;
	generator.cells = new PlaneCell[generator.cellCountX * generator.cellCountY];
	generator.ts_cellSize = Vec2f(
			generator.ws_cellSize.x / areaSize.x,
			generator.ws_cellSize.y / areaSize.y);

	// initialize cells
	Vec2f cellUV = generator.ts_cellSize * 0.5f;
	for (unsigned int j = 0; j < generator.cellCountY; j++) {
		for (unsigned int i = 0; i < generator.cellCountX; i++) {
			auto index = j * generator.cellCountX + i;
			auto &cell = generator.cells[index];
			cell.position = Vec2f(static_cast<float>(i), static_cast<float>(j)) *
							generator.ws_cellSize + generator.cellHalfSize - areaHalfSize;
			cell.uv = cellUV;
			cellUV.x += generator.ts_cellSize.x;
			cell.density = 1.0f;
			cell.size = generator.ws_cellSize;
		}
		cellUV.x = generator.ts_cellSize.x * 0.5f;
		cellUV.y += generator.ts_cellSize.y;
	}

	if (generator.hasGroundMaterial()) {
		// Use material weight maps of the ground mesh to compute cell density.
		// This requires to set the material index of the ground mesh, for the material
		// where we want to spawn instances.
		for (unsigned int i = 0; i < generator.numCells; i++) {
			auto &cell = generator.cells[i];
			cell.density = generator.matWeightMap->sampleMax<Vec4f>(
					cell.uv,
					generator.ts_cellSize,
					generator.matWeightData,
					generator.materialIdx);
		}
	}
	else if (generator.maskData.get()) {
		// Compute cell density based on mask texture.
		for (unsigned int i = 0; i < generator.numCells; i++) {
			auto &cell = generator.cells[i];
			if (generator.maskTexture->format() == GL_RGBA) {
				cell.density = generator.maskTexture->sampleMax<Vec4f>(cell.uv,
					generator.ts_cellSize, generator.maskData, generator.maskIndex);
			} else if (generator.maskTexture->format() == GL_RGB) {
				cell.density = generator.maskTexture->sampleMax<Vec3f>(cell.uv,
					generator.ts_cellSize, generator.maskData, generator.maskIndex);
			} else if (generator.maskTexture->format() == GL_RG) {
				cell.density = generator.maskTexture->sampleMax<Vec2f>(cell.uv,
					generator.ts_cellSize, generator.maskData, generator.maskIndex);
			} else {
				// assume single channel texture
				cell.density = generator.maskTexture->sampleMax<float>(cell.uv,
					generator.ts_cellSize, generator.maskData);
			}
		}
	}

	for (unsigned int j = 0; j < generator.cellCountY; j++) {
		for (unsigned int i = 0; i < generator.cellCountX; i++) {
			makeInstances(generator, i, j);
		}
	}

	delete[] generator.cells;

	if (generator.instanceData.empty()) {
		REGEN_WARN("No instances created.");
		return numInstances;
	} else {
		numInstances = generator.instanceData.size();
		// TODO: apply previous transform instead of overwriting
		tf->modelMat()->setInstanceData(numInstances, 1,
				(byte*)generator.instanceData.data());
		tf->set_numInstances(numInstances);
	}

	return numInstances;
}

static void transformAnimation(
		scene::SceneLoader *scene,
		const ref_ptr<scene::SceneInputNode> &child,
		const ref_ptr<State> &state,
		const ref_ptr<StateNode> &parent,
		const ref_ptr<ModelTransformation> &tf) {
	auto animType = child->getValue<std::string>("type", "transform");

	if (animType == "boids") {
		LoadingContext boidsConfig(scene, parent);
		ref_ptr<Animation> boidsAnimation;
		if (child->getValue<std::string>("sim-mode", "CPU") == "CPU") {
			boidsAnimation = BoidsCPU::load(boidsConfig, *child.get(), tf);
		} else {
			boidsAnimation = BoidsGPU::load(boidsConfig, *child.get(), tf);
		}
		state->attach(boidsAnimation);
		boidsAnimation->startAnimation();
	} else {
		if (!tf->hasModelMat()) {
			REGEN_WARN("transform animation requires a model matrix, but TF of "
							   << child->getDescription() << " has no model matrix.");
			return;
		}
		auto transformAnimation = ref_ptr<TransformAnimation>::alloc(tf,
			child->getValue<uint32_t>("tf-id", 0u));

		if (child->hasAttribute("mesh-id")) {
			auto meshID = child->getValue("mesh-id");
			auto meshIndex = child->getValue<uint32_t>("mesh-index", 0u);
			auto compositeMesh = scene->getResource<CompositeMesh>(meshID);
			if (compositeMesh.get() != nullptr && compositeMesh->meshes().size() > meshIndex) {
				auto mesh = compositeMesh->meshes()[meshIndex];
				transformAnimation->setMesh(mesh);
			}
		}

		for (auto &keyFrameNode: child->getChildren("key-frame")) {
			std::optional<Vec3f> framePos = std::nullopt;
			std::optional<Vec3f> frameDir = std::nullopt;
			if (keyFrameNode->hasAttribute("position")) {
				framePos = keyFrameNode->getValue<Vec3f>("position", Vec3f::zero());
			}
			if (keyFrameNode->hasAttribute("rotation")) {
				frameDir = keyFrameNode->getValue<Vec3f>("rotation", Vec3f::zero());
			}
			auto dt = keyFrameNode->getValue<double>("dt", 1.0);
			transformAnimation->addTransformKeyframe(framePos, frameDir, dt);
		}

		transformAnimation->setAnimationName(child->getName());
		transformAnimation->setLoopTransformAnimation(child->getValue<bool>("loop", true));
		transformAnimation->startAnimation();
		state->attach(transformAnimation);
	}
}

static void transformMatrix(
		scene::SceneLoader *scene,
		scene::SceneInputNode &input,
		const ref_ptr<State> &state,
		const ref_ptr<StateNode> &parent,
		const ref_ptr<ModelTransformation> &tf,
		uint32_t numInstances) {
	for (auto &child: input.getChildren()) {
		std::list<scene::IndexRange> indices = child->getIndexSequence(numInstances);

		if (child->getCategory() == "set") {
			if (!tf->hasModelMat()) {
				REGEN_WARN("set requires a model matrix, but TF of "
								   << input.getDescription() << " has no model matrix.");
				continue;
			}
			auto mode = child->getValue("mode");
			if (mode == "plane") {
				numInstances = transformMatrixPlane(scene, *child.get(), tf, numInstances);
			} else {
				auto matrices = tf->modelMat()->mapClientData<Mat4f>(BUFFER_GPU_WRITE);
				if (matrices.w.data() != matrices.r.data()) {
					// Make sure we can read most recent data from matrices.w
					std::memcpy((byte*)matrices.w.data(), (const byte*)matrices.r.data(),
							sizeof(Mat4f) * tf->modelMat()->numInstances());
				}
				uint32_t numIndices = 0;
				for (auto &range : indices) {
					numIndices += (range.to - range.from + range.step - 1) / range.step;
				}
				scene::ValueGenerator<Vec3f> generator(child.get(), numIndices,
													   child->getValue<Vec3f>("value", Vec3f::zero()));
				const auto target = child->getValue<std::string>("target", "translate");

				for (auto &range : indices) {
					for (unsigned int j = range.from; j <= range.to; j = j + range.step) {
						transformMatrix(scene, child, target, matrices.w[j], generator.next());
					}
				}
			}
		} else if (child->getCategory() == "animation") {
			transformAnimation(scene, child, state, parent, tf);
		} else {
			if (tf->hasModelMat()) {
				auto matrices = tf->modelMat()->mapClientData<Mat4f>(BUFFER_GPU_WRITE);
				if (matrices.w.data() != matrices.r.data()) {
					// Make sure we can read most recent data from matrices.w
					std::memcpy((byte*)matrices.w.data(), (const byte*)matrices.r.data(),
							sizeof(Mat4f) * tf->modelMat()->numInstances());
				}
				for (auto &range : indices) {
					for (unsigned int j = range.from; j <= range.to; j = j + range.step) {
						transformMatrix(scene, child,
								child->getCategory(), matrices.w[j],
								child->getValue<Vec3f>("value", Vec3f::zero()));
					}
				}
			} else if (tf->hasModelOffset()) {
				auto &modelOffset = tf->modelOffset();
				auto offsets = modelOffset->mapClientData<Vec4f>(BUFFER_GPU_WRITE);
				if (offsets.w.data() != offsets.r.data()) {
					// Make sure we can read most recent data from offsets.w
					std::memcpy((byte*)offsets.w.data(), (const byte*)offsets.r.data(),
							sizeof(Vec4f) * modelOffset->numInstances());
				}
				for (auto &range : indices) {
					for (unsigned int j = range.from; j <= range.to; j = j + range.step) {
						transformOffset(
								child->getCategory(), offsets.w[j],
								child->getValue<Vec3f>("value", Vec3f::zero()));
					}
				}
			}
		}
	}
}

ref_ptr<ModelTransformation>
ModelTransformation::load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<State> &state) {
	auto scene = ctx.scene();

	ref_ptr<ModelTransformation> transform = scene->getResource<ModelTransformation>(input.getName());
	if (transform.get() != nullptr) {
		state->joinStates(transform);
		return transform;
	}
	ref_ptr<scene::SceneInputNode> transformNode = scene->getRoot()->getFirstChild("transform", input.getName());
	if (transformNode.get() != nullptr && transformNode.get() != &input) {
		LoadingContext cfgTransform(scene, ctx.parent());
		return load(cfgTransform, *transformNode.get(), state);
	}

	bool isInstanced = input.getValue<bool>("is-instanced", false);
	auto numInstances = input.getValue<uint32_t>("num-instances", 1u);
	int tfMode = ModelTransformation::TF_MATRIX;
	if (input.hasAttribute("mode")) {
		auto tfMode_str = input.getValue<std::string>("mode", "matrix");
		if (tfMode_str == "offset") {
			tfMode = ModelTransformation::TF_OFFSET;
		} else if (tfMode_str == "both") {
			tfMode = ModelTransformation::TF_OFFSET | ModelTransformation::TF_MATRIX;
		}
	}
	BufferUpdateFlags updateFlags;
	updateFlags.frequency = input.getValue<BufferUpdateFrequency>(
		"update-frequency", BUFFER_UPDATE_PER_FRAME);
	updateFlags.scope = input.getValue<BufferUpdateScope>(
		"update-scope", BUFFER_UPDATE_FULLY);

	transform = ref_ptr<ModelTransformation>::alloc(tfMode, updateFlags);
	// read the gpu-usage flag
	if (input.getValue<std::string>("gpu-usage", "READ") == "WRITE") {
		transform->modelMat()->setServerAccessMode(BUFFER_GPU_WRITE);
		transform->modelOffset()->setServerAccessMode(BUFFER_GPU_WRITE);
	} else {
		transform->modelMat()->setServerAccessMode(BUFFER_GPU_READ);
		transform->modelOffset()->setServerAccessMode(BUFFER_GPU_READ);
	}

	// Handle instanced model matrix
	if (isInstanced && numInstances > 1) {
		transform->set_numInstances(numInstances);
		if (transform->hasModelMat()) {
			// allocate client memory for model matrix
			transform->modelMat()->setInstanceData(numInstances, 1, nullptr);
		} else if (transform->hasModelOffset()) {
			// allocate client memory for model offset
			transform->modelOffset()->setInstanceData(numInstances, 1, nullptr);
		}
	}
	// set identity matrices on both slots
	for (uint32_t i = 0; i < numInstances; i += 1) {
		Mat4f *mat0 = (Mat4f*) transform->modelMat()->clientData(0);
		Mat4f *mat1 = (Mat4f*) transform->modelMat()->clientData(1);
		for (uint32_t j = 0; j < transform->modelMat()->numInstances(); j += 1) {
			mat0[j] = Mat4f::identity();
			if(mat1) mat1[j] = Mat4f::identity();
		}
		Vec4f *offset0 = (Vec4f*) transform->modelOffset()->clientData(0);
		Vec4f *offset1 = (Vec4f*) transform->modelOffset()->clientData(1);
		for (uint32_t j = 0; j < transform->modelOffset()->numInstances(); j += 1) {
			offset0[j] = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
			if (offset1) offset1[j] = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
		}
	}
	transformMatrix(scene, input, state, ctx.parent(), transform, numInstances);

	transform->tfBuffer()->updateBuffer();
	state->joinStates(transform);
	scene->putResource<ModelTransformation>(input.getName(), transform);
	return transform;
}

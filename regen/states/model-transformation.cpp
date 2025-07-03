#include <stack>
#include "model-transformation.h"
#include "regen/meshes/mesh-vector.h"
#include "regen/textures/texture.h"
#include "regen/animations/boids-cpu.h"
#include "regen/animations/boids-gpu.h"
#include "regen/animations/transform-animation.h"
#include "regen/scene/value-generator.h"

using namespace regen;

ModelTransformation::ModelTransformation(int tfMode)
		: State(),
		  tfMode_(tfMode) {
	modelMat_ = ref_ptr<ShaderInputMat4>::alloc("modelMatrix");
	modelOffset_ = ref_ptr<ShaderInput4f>::alloc("modelOffset");
	velocity_ = ref_ptr<ShaderInput3f>::alloc("meshVelocity");

	modelMat_->setUniformData(Mat4f::identity());
	modelOffset_->setUniformData(Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
	velocity_->setUniformData(Vec3f(0.0f));

	modelMat_->setSchema(InputSchema::transform());
	modelOffset_->setSchema(InputSchema::position());

	initBufferContainer();
}

ModelTransformation::ModelTransformation(const ref_ptr<ShaderInput4f> &offset)
		: State(),
		  tfMode_(TF_OFFSET) {
	modelOffset_ = offset;
	modelMat_ = ref_ptr<ShaderInputMat4>::alloc("modelMatrix");
	velocity_ = ref_ptr<ShaderInput3f>::alloc("meshVelocity");
	modelMat_->setUniformData(Mat4f::identity());
	velocity_->setUniformData(Vec3f(0.0f));
	modelMat_->setSchema(InputSchema::transform());
	initBufferContainer();
}

ModelTransformation::ModelTransformation(const ref_ptr<ShaderInputMat4> &mat)
		: State(),
		  tfMode_(TF_MATRIX) {
	modelMat_ = mat;
	modelMat_->setSchema(InputSchema::transform());

	modelOffset_ = ref_ptr<ShaderInput4f>::alloc("modelOffset");
	modelOffset_->setUniformData(Vec4f(0.0f, 0.0f, 0.0f, 1.0f));

	velocity_ = ref_ptr<ShaderInput3f>::alloc("meshVelocity");
	velocity_->setUniformData(Vec3f(0.0f));

	initBufferContainer();
}

void ModelTransformation::initBufferContainer() {
	bufferContainer_ = ref_ptr<BufferContainer>::alloc("ModelTransformation");
	if (tfMode_ & TF_MATRIX) {
		bufferContainer_->addInput(modelMat_);
	}
	if (tfMode_ & TF_OFFSET) {
		bufferContainer_->addInput(modelOffset_);
	}
	bufferContainer_->addInput(velocity_);
	joinStates(bufferContainer_);
}

uint32_t ModelTransformation::stamp() const {
	if (tfMode_ == TF_OFFSET) {
		return modelOffset_->stamp();
	} else if (tfMode_ == TF_MATRIX) {
		return modelMat_->stamp();
	} else {
		return modelOffset_->stamp() + modelMat_->stamp();
	}
}

uint32_t ModelTransformation::numInstances() const {
	if (tfMode_ == TF_OFFSET) {
		return modelOffset_->numInstances();
	} else if (tfMode_ == TF_MATRIX) {
		return modelMat_->numInstances();
	} else {
		return std::max(modelOffset_->numInstances(), modelMat_->numInstances());
	}
}

ShaderInput *PositionReader::getModelMat(const ModelTransformation *tf) {
	return tf->hasModelMat() ? tf->modelMat().get() : nullptr;
}

ShaderInput *PositionReader::getModelOffset(const ModelTransformation *tf) {
	return tf->hasModelOffset() ? tf->modelOffset().get() : nullptr;
}

const Vec3f &PositionReader::getPositionReference(const ModelTransformation *tf, unsigned int vertexIndex) const {
	if (tf->hasModelOffset() && tf->hasModelMat()) {
		tf->tmpPos_ =
			((const Mat4f *) rawData_mat.r)[vertexIndex].position() +
			((const Vec4f *) rawData_offset.r)[vertexIndex].xyz_();
		return tf->tmpPos_;
	}
	if (tf->hasModelOffset()) {
		return ((const Vec4f *) rawData_offset.r)[vertexIndex].xyz_();
	}
	if (tf->hasModelMat()) {
		return ((const Mat4f *) rawData_mat.r)[vertexIndex].position();
	}
	return Vec3f::zero();
}

PositionReader ModelTransformation::position(uint32_t idx) const {
	return {this, idx};
}

void ModelTransformation::enable(RenderState *rs) {
	if (isAudioSource()) {
		boost::posix_time::ptime time(
				boost::posix_time::microsec_clock::local_time());
		GLdouble dt = ((GLdouble) (time - lastTime_).total_microseconds()) / 1000.0;
		lastTime_ = time;

		if (dt > 1e-6) {
			auto val = modelMat_->getVertex(0);
			velocity_->setVertex(0, (val.r.position() - lastPosition_) / dt);
			lastPosition_ = val.r.position();
			if (isAudioSource()) {
				audioSource_->set3f(AL_VELOCITY, velocity_->getVertex(0).r);
			}
			audioSource_->set3f(AL_POSITION, val.r.position());
		}
	}
	State::enable(rs);
}

static void transformMatrix(
		const std::string &target, Mat4f &mat, const Vec3f &value) {
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
	} else {
		REGEN_WARN("Unknown distribute target '" << target << "'.");
	}
}

static void transformMatrix2(
		const std::string &target, Mat4f &mat, Vec4f &offset, const Vec3f &value) {
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
	} else if (target == "offset") {
		offset.x += value.x;
		offset.y += value.y;
		offset.z += value.z;
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
	Vec2f areaSize = Vec2f(10.0f);
	float objMinScale = 0.6f;
	float objMaxScale = 1.0f;
	float objPosVariation = 0.0f;
	float objDensity = 1.0f;
	Vec2f objSize = Vec2f(1.0f);
	Vec2f ws_cellSize = Vec2f(1.0f);
	Vec2f ts_cellSize = Vec2f(0.1f);
	Vec2f cellHalfSize = Vec2f(0.5f);
	Vec3f cellWorldOffset = Vec3f(0.0f);
	std::vector<Mat4f> instanceData;
	ref_ptr<Texture2D> maskTexture;
	ref_ptr<Texture2D> heightMap;
	const GLubyte *maskData = nullptr;
	const GLubyte *heightData = nullptr;
	//
	PlaneCell *cells = nullptr;
	unsigned int cellCountX = 0;
	unsigned int cellCountY = 0;
	unsigned int numCells = 0;
};

static void makeInstance(InstancePlaneGenerator &generator, const PlaneCell &cell) {
	if (generator.maskData) {
		auto density = generator.maskTexture->sampleLinear<float>(
				cell.uv, generator.maskData);
		if (density < 0.1f) return;
	}

	auto &instanceMat = generator.instanceData.emplace_back(Mat4f::identity());

	// apply scaling, randomize between min and max scale
	auto sizeFactor = math::random<float>() * cell.density;
	auto scale = generator.objMinScale + sizeFactor * (generator.objMaxScale - generator.objMinScale);
	instanceMat.scale(scale);

	// apply random rotation around y axis
	auto angle = math::random<float>() * 2.0f * M_PI;
	Quaternion q(0.0, 0.0, 0.0, 1.0);
	q.setEuler(angle, 0.0f, 0.0f);
	instanceMat *= q.calculateMatrix();

	// translate to cell position
	Vec3f pos = Vec3f(cell.position.x, 0.0f, cell.position.y) + generator.cellWorldOffset;
	if (generator.objPosVariation > 0.0f) {
		// TODO: randomize position within cell, but need to re-compute UV then!
		//pos.x += generator.objPosVariation * (math::random<float>() - 0.5f) * 2.0f;
		//pos.z += generator.objPosVariation * (math::random<float>() - 0.5f) * 2.0f;
		//cell_uv = ...
	}
	instanceMat.x[12] += pos.x;
	instanceMat.x[13] += pos.y;
	instanceMat.x[14] += pos.z;
	// translate to height map position
	if (generator.heightData) {
		instanceMat.x[13] += generator.areaMaxHeight * generator.heightMap->sampleLinear<float>(
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
	if (cell.density < 0.1f) {
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
		const ref_ptr<ShaderInputMat4> &matrixInput,
		GLuint numInstances) {
	auto areaSize = input.getValue<Vec2f>("area-size", Vec2f(10.0f));
	auto areaHalfSize = areaSize * 0.5f;

	InstancePlaneGenerator generator;
	generator.areaSize = areaSize;
	generator.areaMaxHeight = input.getValue<GLfloat>("area-max-height", 0.0f);
	generator.objMinScale = input.getValue<GLfloat>("obj-min-scale", 0.6f);
	generator.objMaxScale = input.getValue<GLfloat>("obj-max-scale", 1.0f);
	generator.objPosVariation = input.getValue<GLfloat>("obj-pos-variation", 0.0f);
	generator.objDensity = input.getValue<GLfloat>("obj-density", 1.0f);
	generator.ws_cellSize = input.getValue<Vec2f>("cell-size", Vec2f(1.0f));
	generator.cellHalfSize = Vec2f(generator.ws_cellSize.x, generator.ws_cellSize.y) * 0.5f;
	generator.cellWorldOffset = input.getValue<Vec3f>("cell-offset", Vec3f(0.0f));
	if (input.hasAttribute("area-mask-texture")) {
		generator.maskTexture = scene->getResource<Texture2D>(input.getValue("area-mask-texture"));
		if (generator.maskTexture.get()) {
			generator.maskTexture->ensureTextureData();
			generator.maskData = generator.maskTexture->textureData();
		} else {
			REGEN_WARN("Unable to load mask texture.");
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
	auto meshes = scene->getResource<MeshVector>(input.getValue("obj-mesh"));
	if (meshes.get() && !meshes.get()->empty()) {
		auto &meshVec = *meshes.get();
		auto firstMesh = meshVec[0];
		Bounds<Vec3f> bounds(firstMesh->minPosition(), firstMesh->maxPosition());
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

	if (generator.maskData) {
		// compute cell density based on mask texture
		for (unsigned int i = 0; i < generator.numCells; i++) {
			auto &cell = generator.cells[i];
			cell.density = generator.maskTexture->sampleMax<float>(
					cell.uv,
					generator.ts_cellSize,
					generator.maskData);
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
		matrixInput->setInstanceData(numInstances, 1, nullptr);
		auto matrices = matrixInput->mapClientData<Mat4f>(ShaderData::WRITE);
		// TODO: apply previous transform instead of overwriting
		for (GLuint i = 0; i < numInstances; i += 1) {
			matrices.w[i] = generator.instanceData[i];
		}
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
		auto transformAnimation = ref_ptr<TransformAnimation>::alloc(tf->modelMat());

		if (child->hasAttribute("mesh-id")) {
			auto meshID = child->getValue("mesh-id");
			auto meshIndex = child->getValue<GLuint>("mesh-index", 0u);
			auto meshVec = scene->getResource<MeshVector>(meshID);
			if (meshVec.get() != nullptr && meshVec->size() > meshIndex) {
				auto mesh = (*meshVec.get())[meshIndex];
				transformAnimation->setMesh(mesh);
			}
		}

		for (auto &keyFrameNode: child->getChildren("key-frame")) {
			std::optional<Vec3f> framePos = std::nullopt;
			std::optional<Vec3f> frameDir = std::nullopt;
			if (keyFrameNode->hasAttribute("position")) {
				framePos = keyFrameNode->getValue<Vec3f>("position", Vec3f(0.0f));
			}
			if (keyFrameNode->hasAttribute("rotation")) {
				frameDir = keyFrameNode->getValue<Vec3f>("rotation", Vec3f(0.0f));
			}
			auto dt = keyFrameNode->getValue<GLdouble>("dt", 1.0);
			transformAnimation->push_back(framePos, frameDir, dt);
		}

		transformAnimation->setAnimationName(child->getName());
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
		GLuint numInstances) {
	for (auto &child: input.getChildren()) {
		std::list<GLuint> indices = child->getIndexSequence(numInstances);

		if (child->getCategory() == "set") {
			if (!tf->hasModelMat()) {
				REGEN_WARN("set requires a model matrix, but TF of "
								   << input.getDescription() << " has no model matrix.");
				continue;
			}
			auto mode = child->getValue("mode");
			if (mode == "plane") {
				numInstances = transformMatrixPlane(scene, *child.get(), tf->modelMat(), numInstances);
			} else {
				auto matrices = tf->modelMat()->mapClientData<Mat4f>(ShaderData::WRITE);
				scene::ValueGenerator<Vec3f> generator(child.get(), indices.size(),
													   child->getValue<Vec3f>("value", Vec3f(0.0f)));
				const auto target = child->getValue<std::string>("target", "translate");

				for (unsigned int &indice: indices) {
					transformMatrix(target, matrices.w[indice], generator.next());
				}
			}
		} else if (child->getCategory() == "animation") {
			transformAnimation(scene, child, state, parent, tf);
		} else {
			auto &modelMat = tf->modelMat();
			auto &modelOffset = tf->modelOffset();
			auto v_modelMat = modelMat->mapClientData<Mat4f>(ShaderData::WRITE);
			auto v_modelOffset = modelOffset->mapClientData<Vec4f>(ShaderData::WRITE);
			for (unsigned int &j: indices) {
				transformMatrix2(
						child->getCategory(),
						(modelMat->numInstances() > 1 ? v_modelMat.w[j] : v_modelMat.w[0]),
						(modelOffset->numInstances() > 1 ? v_modelOffset.w[j] : v_modelOffset.w[0]),
						child->getValue<Vec3f>("value", Vec3f(0.0f)));
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
	auto numInstances = input.getValue<GLuint>("num-instances", 1u);
	int tfMode = ModelTransformation::TF_MATRIX;
	if (input.hasAttribute("mode")) {
		auto tfMode_str = input.getValue<std::string>("mode", "matrix");
		if (tfMode_str == "offset") {
			tfMode = ModelTransformation::TF_OFFSET;
		} else if (tfMode_str == "both") {
			tfMode = ModelTransformation::TF_OFFSET | ModelTransformation::TF_MATRIX;
		}
	}
	transform = ref_ptr<ModelTransformation>::alloc(tfMode);
	// read the gpu-usage flag
	if (input.getValue<std::string>("gpu-usage", "READ") == "WRITE") {
		transform->modelMat()->set_gpuUsage(ShaderData::WRITE);
		transform->modelOffset()->set_gpuUsage(ShaderData::WRITE);
	} else {
		transform->modelMat()->set_gpuUsage(ShaderData::READ);
		transform->modelOffset()->set_gpuUsage(ShaderData::READ);
	}

	// Handle instanced model matrix
	if (isInstanced && numInstances > 1) {
		auto &modelMat = transform->modelMat();
		auto &modelOffset = transform->modelOffset();
		if (transform->hasModelMat()) {
			modelMat->setInstanceData(numInstances, 1, nullptr);
			auto matrices = modelMat->mapClientData<Mat4f>(ShaderData::WRITE);
			for (GLuint i = 0; i < numInstances; i += 1) matrices.w[i] = Mat4f::identity();
		} else if (transform->hasModelOffset()) {
			modelOffset->setInstanceData(numInstances, 1, nullptr);
			auto offsets = modelOffset->mapClientData<Vec4f>(ShaderData::WRITE);
			for (GLuint i = 0; i < numInstances; i += 1) offsets.w[i] = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
		}
		// update numInstances
		transformMatrix(scene, input, state, ctx.parent(), transform, numInstances);
	} else {
		transformMatrix(scene, input, state, ctx.parent(), transform, 1u);
	}

	transform->bufferContainer()->updateBuffer();
	state->joinStates(transform);
	scene->putResource<ModelTransformation>(input.getName(), transform);
	return transform;
}

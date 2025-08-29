#include <map>
#include "mesh-simplifier.h"

#define SIMPLIFIER_USE_PATH_COMPRESSION
#define SIMPLIFIER_USE_VALENCE_COST
//#define SIMPLIFIER_USE_AREA_COST

using namespace regen;

MeshSimplifier::MeshSimplifier(const ref_ptr<Mesh> &mesh) : mesh_(mesh) {
	switch (mesh_->primitive()) {
		case GL_TRIANGLES:
			break;
		case GL_TRIANGLE_FAN:
		case GL_TRIANGLE_STRIP:
		case GL_QUADS:
		case GL_QUAD_STRIP:
		case GL_LINES:
		case GL_LINE_LOOP:
		case GL_LINE_STRIP:
		default:
			// TODO: support other primitive types than triangles
			REGEN_ERROR("Mesh simplifier only supports triangle meshes.");
			hasValidAttributes_ = false;
			break;
	}

	// lookup mesh attributes
	for (auto &in: mesh_->inputs()) {
		if (in.in_->isVertexAttribute()) {
			if (!addInputAttribute(in)) {
				hasValidAttributes_ = false;
			}
		}
	}
	inputIndices_ = mesh_->indices();
	quadrics_.resize(inputPos_->numVertices());
	neighbors_.resize(inputPos_->numVertices());
	// Validate attributes
	if (!inputPos_.get()) {
		REGEN_ERROR("Mesh has no position attribute, no LoD will be generated.");
		hasValidAttributes_ = false;
	}
	int numBoneAtts = (inputBoneWeights_.get() != nullptr ? 1 : 0) +
					  (inputBoneIndices_.get() != nullptr ? 1 : 0);
	if (numBoneAtts > 0) {
		if (numBoneAtts == 1) {
			if (inputBoneWeights_.get() != nullptr) {
				REGEN_ERROR("Mesh has bone weights, but no bone indices.");
				hasValidAttributes_ = false;
			}
		} else if (inputBoneWeights_->numArrayElements() != inputBoneIndices_->numArrayElements()) {
			REGEN_ERROR("Mesh has bones, but the number of weights and indices is different.");
			hasValidAttributes_ = false;
		}
	}
	REGEN_INFO("Input mesh has " <<
							   "#faces=" << inputIndices_->numVertices() / 3 << " " <<
							   "#verts=" << inputPos_->numVertices() << " " <<
							   "#attributes=" << (inputAttributes_.size() + 1) << " ");
	// avoid resize
	const int numLodLevels = 3;
	lodData_.reserve(numLodLevels - 1);
	lodLevels_.reserve(numLodLevels);
}

bool ensureBaseType(const ref_ptr<ShaderInput> &input, GLenum baseType) {
	if (input->baseType() != baseType) {
		REGEN_ERROR("Attribute " << input->name() << " has base type " <<
							   std::hex << input->baseType() << ", but required is " <<
							   std::hex << baseType);
		return false;
	}
	return true;
}

bool ensureValueSize(const ref_ptr<ShaderInput> &input, int size) {
	if (input->valsPerElement() != size) {
		REGEN_ERROR("Attribute " << input->name() << " has value size " <<
							   input->valsPerElement() << ", but required is " << size);
		return false;
	}
	return true;
}

bool ensureNoArray(const ref_ptr<ShaderInput> &input) {
	if (input->numArrayElements() > 1) {
		REGEN_ERROR("Attribute " << input->name() << " is an array, but required is no array.");
		return false;
	}
	return true;
}

bool MeshSimplifier::addInputAttribute(const NamedShaderInput &namedAttribute) {
	if (namedAttribute.name_.find(ATTRIBUTE_NAME_POS) == 0) {
		if (!ensureBaseType(namedAttribute.in_, GL_FLOAT)) return false;
		if (!ensureValueSize(namedAttribute.in_, 3)) return false;
		if (!ensureNoArray(namedAttribute.in_)) return false;
		inputPos_ = ref_ptr<ShaderInput3f>::dynamicCast(namedAttribute.in_);
		return inputPos_.get() != nullptr;
	}
	else if (namedAttribute.name_.find(ATTRIBUTE_NAME_NOR) == 0) {
		if (!ensureBaseType(namedAttribute.in_, GL_FLOAT)) return false;
		if (!ensureValueSize(namedAttribute.in_, 3)) return false;
		if (!ensureNoArray(namedAttribute.in_)) return false;
		inputAttributes_.emplace_back(AttributeSemantic::NORMAL, namedAttribute.in_);
		inputNor_ = ref_ptr<ShaderInput3f>::dynamicCast(namedAttribute.in_);
		norIndex_ = inputAttributes_.size() - 1;
	}
	else if (namedAttribute.name_.find("texco") == 0 ||
			   namedAttribute.name_.find("uv") == 0) {
		if (!ensureBaseType(namedAttribute.in_, GL_FLOAT)) return false;
		if (!ensureNoArray(namedAttribute.in_)) return false;
		inputAttributes_.emplace_back(AttributeSemantic::TEXCOORD, namedAttribute.in_);
	}
	else if (namedAttribute.name_.find("col") == 0) {
		if (!ensureBaseType(namedAttribute.in_, GL_FLOAT)) return false;
		if (!ensureNoArray(namedAttribute.in_)) return false;
		inputAttributes_.emplace_back(AttributeSemantic::COLOR, namedAttribute.in_);
	}
	else if (namedAttribute.name_.find(ATTRIBUTE_NAME_TAN) == 0) {
		if (!ensureBaseType(namedAttribute.in_, GL_FLOAT)) return false;
		if (!ensureValueSize(namedAttribute.in_, 4)) return false;
		if (!ensureNoArray(namedAttribute.in_)) return false;
		inputAttributes_.emplace_back(AttributeSemantic::TANGENT, namedAttribute.in_);
	}
	else if (namedAttribute.name_.find("bitan") == 0) {
		if (!ensureBaseType(namedAttribute.in_, GL_FLOAT)) return false;
		if (!ensureValueSize(namedAttribute.in_, 4)) return false;
		if (!ensureNoArray(namedAttribute.in_)) return false;
		inputAttributes_.emplace_back(AttributeSemantic::BITANGENT, namedAttribute.in_);
	}
	else if (namedAttribute.name_.find("boneWeights") == 0) {
		if (!ensureBaseType(namedAttribute.in_, GL_FLOAT)) return false;
		inputAttributes_.emplace_back(AttributeSemantic::BONE_WEIGHTS, namedAttribute.in_);
		inputBoneWeights_ = ref_ptr<ShaderInput1f>::dynamicCast(namedAttribute.in_);
	}
	else if (namedAttribute.name_.find("boneIndices") == 0) {
		if (!ensureBaseType(namedAttribute.in_, GL_UNSIGNED_INT)) return false;
		inputAttributes_.emplace_back(AttributeSemantic::BONE_INDICES, namedAttribute.in_);
		inputBoneIndices_ = ref_ptr<ShaderInput1ui>::dynamicCast(namedAttribute.in_);
	}
	else {
		REGEN_DEBUG("Unknown attribute semantics for attribute '" << namedAttribute.name_ << "'." <<
			" Treating it as generic float attribute.");
		if (!ensureBaseType(namedAttribute.in_, GL_FLOAT)) return false;
		inputAttributes_.emplace_back(AttributeSemantic::UNKNOWN, namedAttribute.in_);
	}
	return true;
}

bool MeshSimplifier::isBoundaryVertex(uint32_t idx) const {
	// boundary vertices are part of a boundary edge, i.e.
	// one that appears only in one face.
	if (isBoundary_.size() > idx) {
		return isBoundary_[idx];
	}
	return false;
}

static inline std::pair<uint32_t, uint32_t> makeEdge(uint32_t v0, uint32_t v1) {
	return {std::min(v0, v1), std::max(v0, v1)};
}

void MeshSimplifier::computeQuadrics(const std::vector<Triangle> &faces, const Vec3f *posData, uint32_t numVertices) {
	// this is done for each LOD level, so first clear the quadrics and neighbors
	quadrics_.clear();
	quadrics_.resize(numVertices);
	neighbors_.clear();
	neighbors_.resize(numVertices);
	// count edges for boundary detection
	std::map<std::pair<uint32_t, uint32_t>, int> inputEdges;
	isBoundary_.clear();
	isBoundary_.resize(numVertices, false);

	for (auto &face: faces) {
		neighbors_[face.v0].insert(face.v1);
		neighbors_[face.v0].insert(face.v2);
		neighbors_[face.v1].insert(face.v0);
		neighbors_[face.v1].insert(face.v2);
		neighbors_[face.v2].insert(face.v0);
		neighbors_[face.v2].insert(face.v1);
		// store the edges
		inputEdges[makeEdge(face.v0, face.v1)]++;
		inputEdges[makeEdge(face.v1, face.v2)]++;
		inputEdges[makeEdge(face.v2, face.v0)]++;

		// update vertex quadrics
		auto &p0 = posData[face.v0];
		auto &p1 = posData[face.v1];
		auto &p2 = posData[face.v2];
		auto faceNormal = (p1 - p0).cross(p2 - p0);
		faceNormal.normalize();
		Quadric faceQuadric(
				faceNormal.x,
				faceNormal.y,
				faceNormal.z,
				-faceNormal.dot(p0));
		quadrics_[face.v0] += faceQuadric;
		quadrics_[face.v1] += faceQuadric;
		quadrics_[face.v2] += faceQuadric;
	}

	// mark boundary vertices, we will avoid collapsing them
	for (auto &edge: inputEdges) {
		if (edge.second == 1) {
			isBoundary_[edge.first.first] = true;
			isBoundary_[edge.first.second] = true;
		}
	}
}

bool MeshSimplifier::solve(const Quadric &Q, Vec3f &outPos) {
	// Build the system from the quadric: A * v = -b
	Mat3f inv;
	if (!Q.toMatrix().inverse(inv)) {
		// Degenerate quadric
		return false;
	}
	float b[3] = {-Q.a[3], -Q.a[6], -Q.a[8]};
	outPos.x = inv.x[0] * b[0] + inv.x[1] * b[1] + inv.x[2] * b[2];
	outPos.y = inv.x[3] * b[0] + inv.x[4] * b[1] + inv.x[5] * b[2];
	outPos.z = inv.x[6] * b[0] + inv.x[7] * b[1] + inv.x[8] * b[2];
	return true;
}

void MeshSimplifier::pushEdge(uint32_t idx0, uint32_t idx1, const Quadric &Q, const LODLevel &lod) {
	auto posData = lod.pos.data();

	// Compute cost based on quadric error
	Vec3f collapsePos;
	if (!solve(Q, collapsePos)) {
		// pick the midpoint of the edge in case of degenerate quadric
		collapsePos = (posData[idx0] + posData[idx1]) * 0.5f;
	}
	auto cost = Q.evaluate(collapsePos);

	// TODO: Add additional constraints to cost function.
	//		 - Experiment with penalty for boundary condition.
	//       Currently boundary edges are skipped entirely. But
	//       some meshes use them for small details in which case we actually want to collapse them.
	//		 - discourage collapsing across visible UV seams
	//			- float uvDistance = length(v1.uv - v2.uv);
	//			- if (uvDistance > threshold) return;
#ifdef SIMPLIFIER_USE_AREA_COST
	auto area = averageAreaOfAffectedTriangles(idx0, idx1);
	auto areaCost = 1.0f / (area + 1e-6f);
	cost += areaCost * areaPenalty_;
#endif
#ifdef SIMPLIFIER_USE_VALENCE_COST
	// Add a Valence Preservation Heuristic
	auto diff = std::abs(
			static_cast<int>(neighbors_[idx0].size()) -
			static_cast<int>(neighbors_[idx1].size()));
	cost += (1.0f / (1.0f + static_cast<float>(diff))) * valencePenalty_;
#endif
	// Check Attribute Continuity Constraints
	if (inputNor_.get()) {
		// add a penalty for normal difference based on angle diff
		auto faceNor = (Vec3f *) lod.attributes[norIndex_]->clientData();
		auto angle = acosf(std::clamp(
				faceNor[idx0].dot(faceNor[idx1]), -1.0f, 1.0f));
		if (normalMaxAngle_ > 0.0 && angle > normalMaxAngle_) {
			// skip collapse if angle is too large
			return;
		}
		auto norCost = (angle / M_PIf);
		// add squared penalty to cost
		cost += norCost * norCost * normalPenalty_;
	}

	edgeCollapses_.push({idx0, idx1, collapsePos, cost});
}

void MeshSimplifier::buildEdgeQueue(const std::vector<Triangle> &faces, const LODLevel &data) {
	// Build the edge queue from the faces.
	// It orders the edges by their cost, which is the sum of the quadrics
	// of the two vertices of the edge.
	std::set<std::pair<uint32_t, uint32_t>> usedEdges;
	std::array<std::pair<uint32_t, uint32_t>, 3> edges;

	// remove any remaining edges from the previous LOD level
	edgeCollapses_ = {};

	for (auto &face: faces) {
		edges[0] = makeEdge(face.v0, face.v1);
		edges[1] = makeEdge(face.v1, face.v2);
		edges[2] = makeEdge(face.v2, face.v0);

		for (auto &edge: edges) {
			if (usedEdges.find(edge) != usedEdges.end()) continue;
			usedEdges.insert(edge);
			pushEdge(edge.first, edge.second,
					 quadrics_[edge.first] + quadrics_[edge.second],
					 data);
		}
	}
}

static uint32_t resolve(uint32_t v, std::vector<uint32_t> &mapping) {
#ifdef SIMPLIFIER_USE_PATH_COMPRESSION
	if (mapping[v] != v) {
		mapping[v] = resolve(mapping[v], mapping);
	}
	return mapping[v];
#else
	while (mapping[v] != v) v = mapping[v];
	return v;
#endif
}

static void interpolateNormal(LODAttribute &attr, uint32_t i1, uint32_t i2, float w1, float w2) {
	auto &data = ((LODAttributeT<Vec3f> *) (&attr))->data;
	auto interp = (data[i1] * w1 + data[i2] * w2);
	interp.normalize();
	data.push_back(interp);
}

static void interpolateTangent(LODAttribute &attr, uint32_t i1, uint32_t i2, float w1, float w2) {
	auto &data = ((LODAttributeT<Vec4f> *) (&attr))->data;
	auto &x1 = data[i1];
	auto &x2 = data[i2];
	auto interp = (x1.xyz_() * w1 + x2.xyz_() * w2);
	interp.normalize();
	data.emplace_back(
			interp.x,
			interp.y,
			interp.z,
			(x1.w + x2.w) * 0.5f);
}

template<class T>
static void interpolateLinearT(LODAttribute &attr, uint32_t i1, uint32_t i2, float w1, float w2) {
	auto &data = ((LODAttributeT<T> *) (&attr))->data;
	auto numArrayElements = static_cast<uint32_t>(attr.attribute->numArrayElements());
	if (numArrayElements > 1) {
		auto offset1 = i1 * numArrayElements;
		auto offset2 = i2 * numArrayElements;
		for (uint32_t i = 0; i < numArrayElements; ++i) {
			auto &x1 = data[offset1 + i];
			auto &x2 = data[offset2 + i];
			data.emplace_back(x1 * w1 + x2 * w2);
		}
	}
	else {
		auto &x1 = data[i1];
		auto &x2 = data[i2];
		data.emplace_back(x1 * w1 + x2 * w2);
	}
}

static void interpolateTexco(LODAttribute &attr, uint32_t i1, uint32_t i2, float w1, float w2) {
	// Texco can be 2d/Vec2f or 3d/Vec3f.
	if (attr.attribute->valsPerElement() == 2) {
		interpolateLinearT<Vec2f>(attr, i1, i2, w1, w2);
	} else if (attr.attribute->valsPerElement() == 3) {
		interpolateLinearT<Vec3f>(attr, i1, i2, w1, w2);
	} else {
		REGEN_WARN("Unknown texco type.");
	}
}

struct BoneInfluence {
	uint32_t index;
	float weight;
};

// Helper: normalize weights so they sum to 1
static inline void normalizeWeights(std::vector<BoneInfluence> &influences) {
	float total = 0.0f;
	for (const auto &inf: influences) total += inf.weight;
	if (total > 0.0f) {
		for (auto &inf: influences) inf.weight /= total;
	}
}

static void interpolateBones(
		LODAttribute &indexAttr,
		LODAttribute *weightAttr,
		uint32_t i1, uint32_t i2, float w1, float w2) {
	auto numWeights = static_cast<uint32_t>(indexAttr.attribute->numArrayElements());
	auto offset1 = i1 * numWeights;
	auto offset2 = i2 * numWeights;
	auto &indexData = ((LODAttributeT<uint32_t> *) (&indexAttr))->data;
	// temporary map to accumulate weights
	static std::unordered_map<uint32_t, float> boneWeightMap;
	boneWeightMap.clear();
	// Accumulate bone weights
	auto *indices1 = indexData.data() + offset1;
	auto *indices2 = indexData.data() + offset2;
	float *weights1 = nullptr, *weights2 = nullptr;
	if (weightAttr != nullptr) {
		auto &weightData = ((LODAttributeT<float> *) weightAttr)->data;
		weights1 = weightData.data() + offset1;
		weights2 = weightData.data() + offset2;
		for (uint32_t i = 0; i < numWeights; ++i) { boneWeightMap[indices1[i]] += weights1[i] * w1; }
		for (uint32_t i = 0; i < numWeights; ++i) { boneWeightMap[indices2[i]] += weights2[i] * w2; }
	} else {
		for (uint32_t i = 0; i < numWeights; ++i) { boneWeightMap[indices1[i]] += w1; }
		for (uint32_t i = 0; i < numWeights; ++i) { boneWeightMap[indices2[i]] += w2; }
	}
	// Flatten to vector
	static std::vector<BoneInfluence> merged;
	merged.clear();
	merged.reserve(boneWeightMap.size());
	for (const auto &[idx, w]: boneWeightMap) {
		merged.push_back({idx, w});
	}
	// Sort descending by weight
	std::sort(merged.begin(), merged.end(), [](const BoneInfluence &a, const BoneInfluence &b) {
		return a.weight > b.weight;
	});
	// Keep top N and normalize
	merged.resize(std::min<uint32_t>(numWeights, merged.size()));
	normalizeWeights(merged);
	// Write output
	if (weightAttr != nullptr) {
		auto &weightData = ((LODAttributeT<float> *) weightAttr)->data;
		for (uint32_t i = 0; i < numWeights; ++i) {
			if (i >= merged.size()) {
				weightData.emplace_back(0.0f);
				indexData.emplace_back(0);
			} else {
				weightData.emplace_back(merged[i].weight);
				indexData.emplace_back(merged[i].index);
			}
		}
	} else {
		for (uint32_t i = 0; i < numWeights; ++i) {
			if (i >= merged.size()) {
				indexData.emplace_back(0);
			} else {
				indexData.emplace_back(merged[i].index);
			}
		}
	}
}

static void interpolateBones(
		LODLevel &level,
		LODAttribute &indexAttr,
		uint32_t i1, uint32_t i2,
		float w1, float w2) {
	// find the bone indices attribute
	LODAttribute *weightAttr = nullptr;
	for (auto &attr: level.attributes) {
		if (attr->semantic == AttributeSemantic::BONE_WEIGHTS) {
			weightAttr = attr;
			break;
		}
	}
	interpolateBones(indexAttr, weightAttr, i1, i2, w1, w2);
}

static void interpolateLinear(LODAttribute &attr, uint32_t i1, uint32_t i2, float w1, float w2) {
	if (attr.attribute->valsPerElement() == 1) {
		interpolateLinearT<float>(attr, i1, i2, w1, w2);
	} else if (attr.attribute->valsPerElement() == 2) {
		interpolateLinearT<Vec2f>(attr, i1, i2, w1, w2);
	} else if (attr.attribute->valsPerElement() == 3) {
		interpolateLinearT<Vec3f>(attr, i1, i2, w1, w2);
	} else if (attr.attribute->valsPerElement() == 4) {
		interpolateLinearT<Vec4f>(attr, i1, i2, w1, w2);
	} else if (attr.attribute->valsPerElement() == 9) {
		interpolateLinearT<Mat3f>(attr, i1, i2, w1, w2);
	} else if (attr.attribute->valsPerElement() == 16) {
		interpolateLinearT<Mat4f>(attr, i1, i2, w1, w2);
	} else {
		REGEN_WARN("Unknown attribute type.");
	}
}

static void interpolate(
		LODLevel &level,
		LODAttribute &attr,
		uint32_t i1, uint32_t i2,
		float w1, float w2) {
	switch (attr.semantic) {
		case AttributeSemantic::NORMAL:
			interpolateNormal(attr, i1, i2, w1, w2);
			break;
		case AttributeSemantic::BITANGENT:
		case AttributeSemantic::TANGENT:
			interpolateTangent(attr, i1, i2, w1, w2);
			break;
		case AttributeSemantic::TEXCOORD:
			interpolateTexco(attr, i1, i2, w1, w2);
			break;
		case AttributeSemantic::BONE_INDICES:
			interpolateBones(level, attr, i1, i2, w1, w2);
			break;
		case AttributeSemantic::BONE_WEIGHTS:
			// NOTE: indices are handled together with weights
			break;
		default:
			// assert(attr.attribute->baseType() == GL_FLOAT);
			interpolateLinear(attr, i1, i2, w1, w2);
			break;
	}
}

uint32_t MeshSimplifier::collapseEdge(uint32_t i1, uint32_t i2, const Vec3f &opt, LODLevel &level) {
	// Get positions
	auto &p1 = level.pos[i1];
	auto &p2 = level.pos[i2];
	// Compute distances to optimal position
	auto d1 = (opt - p1).length();
	auto d2 = (opt - p2).length();
	// Threshold for reusing existing vertex
	const float reuseThreshold = 1e-6f;
	uint32_t newIdx;

	if (d1 < reuseThreshold || isBoundaryVertex(i1)) {
		// Collapse to v1
		newIdx = i1;
	} else if (d2 < reuseThreshold || isBoundaryVertex(i2)) {
		// Collapse to v2
		newIdx = i2;
	} else {
		// Compute weights for interpolation
		auto invSum = 1.0f / (d1 + d2 + 1e-6f);
		auto w1 = d2 * invSum;
		auto w2 = d1 * invSum;

		// Create new vertex with interpolated attributes
		newIdx = static_cast<uint32_t>(level.pos.size());
		level.pos.push_back(opt);
		for (auto &attr: level.attributes) {
			interpolate(level, *attr, i1, i2, w1, w2);
		}
	}

	return newIdx;
}

void MeshSimplifier::updateEdgeCosts(uint32_t vNew,
									 const LODLevel &levelData,
									 std::vector<uint32_t> &mapping) {
	const Quadric &Qv = quadrics_[vNew];
	for (uint32_t neighbor: neighbors_[vNew]) {
		// Skip if vertex is no longer active
		if (mapping[neighbor] != neighbor) continue;
		// Else create new edge, as we do not want to modify items in the priority queue
		pushEdge(
				std::min(vNew, neighbor),
				std::max(vNew, neighbor),
				Qv + quadrics_[neighbor],
				levelData);
	}
}

uint32_t MeshSimplifier::generateLodLevel(size_t targetFaceCount, LODLevel &lodData) {
	const auto &inputTriangles = lodLevels_.back();  // Last LOD level
	auto triangles = inputTriangles; // Working copy
	// LOD data has initially the same size as the last LOD level output.
	// and also holds a copy of the vertex data.
	// In the loop below, we might create additional vertices which
	// are pushed to the back of the lodData.
	// The output of generateLodLevel then may has some unused vertices and faces,
	// which we will remove in compactLodLevel.
	// Map from old vertex index to its current representative
	std::vector<uint32_t> vertexMapping(lodData.pos.size());
	std::iota(vertexMapping.begin(), vertexMapping.end(), 0); // initially identity mapping
	uint32_t collapseCount = 0;
	uint32_t numActiveFaces = triangles.size();
	uint32_t r1, r2, newIdx;

	while (numActiveFaces > targetFaceCount && !edgeCollapses_.empty()) {
		{    // collapse next edge
			auto &collapse = edgeCollapses_.top();
			r1 = resolve(collapse.v1, vertexMapping);
			r2 = resolve(collapse.v2, vertexMapping);
			bool preventCollapse = (r1 == r2);
			if (useStrictBoundary_) {
				if (isBoundaryVertex(r1) || isBoundaryVertex(r2)) {
					preventCollapse = true;
				}
			} else {
				if (isBoundaryVertex(r1) && isBoundaryVertex(r2)) {
					preventCollapse = true;
				}
			}

			if (preventCollapse ||
				// Already collapsed, and another edge was added to the queue
				r1 != collapse.v1 || r2 != collapse.v2) {
				edgeCollapses_.pop();
				continue;
			}
			collapseCount += 1;

			// Collapse v2 into v1 or use a new vertex
			newIdx = collapseEdge(r1, r2, collapse.optimalPos, lodData);
			edgeCollapses_.pop();
		}
		// clean up the neighborhood
		if (r1 != newIdx) {
			for (auto n: neighbors_[r1]) {
				neighbors_[resolve(n, vertexMapping)].erase(r1);
			}
			neighbors_[r1].clear();
		}
		if (r2 != newIdx) {
			for (auto n: neighbors_[r2]) {
				neighbors_[resolve(n, vertexMapping)].erase(r2);
			}
			neighbors_[r2].clear();
		}
		// Update mapping: collapsed vertices now point to new vertex
		vertexMapping[r1] = newIdx;
		vertexMapping[r2] = newIdx;
		if (newIdx >= vertexMapping.size()) {
			// New vertex created, update mapping and add to neighbors (empty set of neighbors)
			vertexMapping.push_back(newIdx);
			neighbors_.emplace_back();
			quadrics_.emplace_back();
		}
		// Compute quadric of new vertex
		quadrics_[newIdx] = quadrics_[r1] + quadrics_[r2];

		// Update triangles
		for (auto &tri: triangles) {
			if (!tri.active) continue;
			tri.v0 = resolve(tri.v0, vertexMapping);
			tri.v1 = resolve(tri.v1, vertexMapping);
			tri.v2 = resolve(tri.v2, vertexMapping);
			if (tri.isDegenerate()) {
				tri.active = false;
				numActiveFaces -= 1;
				continue;
			}
			// Update neighbors, we only insert here and skip the inactive neighbors later in edge collapses
			if (tri.v0 == newIdx) {
				neighbors_[tri.v0].insert(tri.v1);
				neighbors_[tri.v0].insert(tri.v2);
				neighbors_[tri.v1].insert(tri.v0);
				neighbors_[tri.v2].insert(tri.v0);
			} else if (tri.v1 == newIdx) {
				neighbors_[tri.v1].insert(tri.v0);
				neighbors_[tri.v1].insert(tri.v2);
				neighbors_[tri.v0].insert(tri.v1);
				neighbors_[tri.v2].insert(tri.v1);
			} else if (tri.v2 == newIdx) {
				neighbors_[tri.v2].insert(tri.v0);
				neighbors_[tri.v2].insert(tri.v1);
				neighbors_[tri.v0].insert(tri.v2);
				neighbors_[tri.v1].insert(tri.v2);
			}
		}

		// rebuild affected edge collapses around `newIndex`
		updateEdgeCosts(newIdx, lodData, vertexMapping);
	}

	// Store the result
	auto &finalLOD = lodLevels_.emplace_back();
	for (const auto &tri: triangles) {
		if (tri.active) finalLOD.push_back(tri);
	}

	return collapseCount;
}

static void compactLodLevel(const LODLevel &data, LODLevel &compactData, std::vector<Triangle> &faces) {
	// after collapse some vertices may be unused.
	// we remove them here, and remap the indices accordingly.
	std::unordered_set<uint32_t> usedIndices;
	for (const Triangle &tri: faces) {
		if (!tri.active) continue;
		usedIndices.insert(tri.v0);
		usedIndices.insert(tri.v1);
		usedIndices.insert(tri.v2);
	}

	std::unordered_map<uint32_t, uint32_t> indexRemap;
	// allocate compacted position buffer
	compactData.pos.resize(usedIndices.size());
	for (auto &a: data.attributes) {
		auto sibling = a->makeSibling(usedIndices.size());
		compactData.attributes.push_back(sibling);
	}
	// copy the data
	uint32_t newIdx = 0u;
	for (uint32_t oldIdx: usedIndices) {
		compactData.pos[newIdx] = data.pos[oldIdx];
		for (uint32_t attributeIndex = 0; attributeIndex < data.attributes.size(); ++attributeIndex) {
			auto &attr_in = data.attributes[attributeIndex];
			auto &attr_out = compactData.attributes[attributeIndex];
			attr_out->setVertex(newIdx, attr_in, oldIdx);
		}
		indexRemap[oldIdx] = newIdx++;
	}

	// Rewrite Indices for new vertex buffer data
	for (Triangle &tri: faces) {
		if (!tri.active) continue;
		tri.v0 = indexRemap[tri.v0];
		tri.v1 = indexRemap[tri.v1];
		tri.v2 = indexRemap[tri.v2];
	}
}

static void initializeLevelData(LODLevel &data,
								const ref_ptr<ShaderInput> &attribute,
								AttributeSemantic semantic,
								uint32_t numElements,
								byte *dataPtr) {
	LODAttribute *a = nullptr;
	if (attribute->baseType() == GL_FLOAT) {
		if (attribute->valsPerElement() == 1) {
			a = new LODAttributeT<float>(attribute, semantic, numElements, (float*)dataPtr);
		} else if (attribute->valsPerElement() == 2) {
			a = new LODAttributeT<Vec2f>(attribute, semantic, numElements, (Vec2f*)dataPtr);
		} else if (attribute->valsPerElement() == 3) {
			a = new LODAttributeT<Vec3f>(attribute, semantic, numElements, (Vec3f*)dataPtr);
		} else if (attribute->valsPerElement() == 4) {
			a = new LODAttributeT<Vec4f>(attribute, semantic, numElements, (Vec4f*)dataPtr);
		} else if (attribute->valsPerElement() == 9) {
			a = new LODAttributeT<Mat3f>(attribute, semantic, numElements, (Mat3f*)dataPtr);
		} else if (attribute->valsPerElement() == 16) {
			a = new LODAttributeT<Mat4f>(attribute, semantic, numElements, (Mat4f*)dataPtr);
		}
	} else if (attribute->baseType() == GL_UNSIGNED_INT) {
		if (attribute->valsPerElement() == 1) {
			a = new LODAttributeT<uint32_t>(attribute, semantic, numElements, (uint32_t*)dataPtr);
		} else if (attribute->valsPerElement() == 2) {
			a = new LODAttributeT<Vec2ui>(attribute, semantic, numElements, (Vec2ui*)dataPtr);
		} else if (attribute->valsPerElement() == 3) {
			a = new LODAttributeT<Vec3ui>(attribute, semantic, numElements, (Vec3ui*)dataPtr);
		} else if (attribute->valsPerElement() == 4) {
			a = new LODAttributeT<Vec4ui>(attribute, semantic, numElements, (Vec4ui*)dataPtr);
		}
	} else if (attribute->baseType() == GL_INT) {
		if (attribute->valsPerElement() == 1) {
			a = new LODAttributeT<int32_t>(attribute, semantic, numElements, (int32_t*)dataPtr);
		} else if (attribute->valsPerElement() == 2) {
			a = new LODAttributeT<Vec2i>(attribute, semantic, numElements, (Vec2i*)dataPtr);
		} else if (attribute->valsPerElement() == 3) {
			a = new LODAttributeT<Vec3i>(attribute, semantic, numElements, (Vec3i*)dataPtr);
		} else if (attribute->valsPerElement() == 4) {
			a = new LODAttributeT<Vec4i>(attribute, semantic, numElements, (Vec4i*)dataPtr);
		}
	}
	if (a) {
		data.attributes.push_back(a);
	} else {
		REGEN_WARN("Unknown attribute type.");
	}
}

bool MeshSimplifier::useOriginalData() {
	return (thresholds_[0] >= 0.999f);
}

uint32_t MeshSimplifier::getNumFaces(float threshold) {
	return static_cast<unsigned int>(
		static_cast<float>(lodLevels_.front().size()) * threshold);
}

void MeshSimplifier::simplifyMesh() {
	if (!hasValidAttributes_) {
		REGEN_ERROR("Mesh simplifier has invalid attributes, simplification will not be performed.");
		return;
	}

	{    // construct first LOD level.
		auto numOriginalFaces = mesh_->numIndices() / 3;
		auto &lodLevel0 = lodLevels_.emplace_back();
		lodLevel0.reserve(numOriginalFaces);
		if (inputIndices_->baseType() == GL_UNSIGNED_BYTE) {
			auto m_inIdx = (uint8_t *) inputIndices_->clientData();
			for (int i = 0; i < numOriginalFaces; ++i) {
				auto j = i * 3;
				lodLevel0.emplace_back(
					(uint32_t)m_inIdx[j],
					(uint32_t)m_inIdx[j + 1],
					(uint32_t)m_inIdx[j + 2]);
			}
		} else if (inputIndices_->baseType() == GL_UNSIGNED_SHORT) {
			auto m_inIdx = (uint16_t *) inputIndices_->clientData();
			for (int i = 0; i < numOriginalFaces; ++i) {
				auto j = i * 3;
				lodLevel0.emplace_back(
					(uint32_t)m_inIdx[j],
					(uint32_t)m_inIdx[j + 1],
					(uint32_t)m_inIdx[j + 2]);
			}
		} else {
			auto m_inIdx = (uint32_t *) inputIndices_->clientData();
			for (int i = 0; i < numOriginalFaces; ++i) {
				auto j = i * 3;
				lodLevel0.emplace_back(
					(uint32_t)m_inIdx[j],
					(uint32_t)m_inIdx[j + 1],
					(uint32_t)m_inIdx[j + 2]);
			}
		}
	}

	// compute number of faces for each LOD level
	std::vector<unsigned int> lodFaces = {0, 0, 0, 0, 0};
	lodFaces[0] = lodLevels_.front().size();
	if (useOriginalData()) {
		lodFaces[1] = getNumFaces(thresholds_[1]);
		lodFaces[2] = getNumFaces(thresholds_[2]);
		lodFaces[3] = getNumFaces(thresholds_[3]);
		lodFaces[4] = 0.0f;
	} else {
		lodFaces[1] = getNumFaces(thresholds_[0]);
		lodFaces[2] = getNumFaces(thresholds_[1]);
		lodFaces[3] = getNumFaces(thresholds_[2]);
		lodFaces[4] = getNumFaces(thresholds_[3]);
	}
	uint32_t numLodLevels = 1;
	for (int i=0; i < 4; ++i) {
		if (i!=0 && thresholds_[i] >= thresholds_[i-1]) {
			break;
		}
		if (thresholds_[i] > 0.0001f && thresholds_[i] < 0.9999f) {
			numLodLevels++;
		}
	}

	// First level offset is num vertices of the original mesh
	vertexOffset_ = useOriginalData() ? inputPos_->numVertices() : 0u;

	// Iteratively collapse the cheapest edge, updating affected geometry and edge costs.
	//	- Stop once your desired number of triangles is reached (or vertices),
	//    and record the current mesh as LOD level.
	// TODO: Possible optimization: Use reserve to avoid reallocating the vector buffers in LODLevel.
	//       Required space depends on how many collapses we do, so we can only guess beforehand.
	uint32_t collapseCount = 0;
	for (size_t lodLevel = 1; lodLevel < numLodLevels; ++lodLevel) {
		// temporary level data
		LODLevel levelData;
		{    // generate LOD level
			levelData.offset = vertexOffset_;
			if (lodLevel == 1) {
				levelData.pos.resize(inputPos_->numVertices());
				std::memcpy(
						(byte *) levelData.pos.data(),
						inputPos_->clientData(),
						inputPos_->inputSize());
				for (auto &it: inputAttributes_) {
					auto &attr = it.second;
					initializeLevelData(
							levelData,
							attr,
							it.first,
							attr->numVertices() * attr->numArrayElements(),
							attr->clientData());
				}
				computeQuadrics(lodLevels_.back(), (Vec3f *) inputPos_->clientData(), levelData.pos.size());
			} else {
				// copy the previous LOD level
				auto &lastLevelData = lodData_[lodLevel - 2];
				levelData.pos.resize(lastLevelData.pos.size());
				std::memcpy(
						(byte *) levelData.pos.data(),
						(byte *) lastLevelData.pos.data(),
						levelData.pos.size() * inputPos_->elementSize());
				for (unsigned int attributeIndex = 0u; attributeIndex < inputAttributes_.size(); ++attributeIndex) {
					auto lodAttr = lastLevelData.attributes[attributeIndex];
					initializeLevelData(
						levelData,
						lodAttr->attribute,
						lodAttr->semantic,
						lodAttr->numElements(),
						(byte *) lodAttr->clientData());
				}
				computeQuadrics(lodLevels_.back(), lastLevelData.pos.data(), levelData.pos.size());
			}
			buildEdgeQueue(lodLevels_.back(), levelData);
			collapseCount = generateLodLevel(lodFaces[lodLevel], levelData);
		}
		if (collapseCount == 0) {
			REGEN_INFO("Mesh simplifier: No more collapses possible, stopping.");
			// remove all remaining LOD levels
			lodLevels_.resize(lodLevels_.size() - 1);
			break;
		}
		{    // compact the LOD level
			auto &compactData = lodData_.emplace_back();
			compactData.offset = vertexOffset_;
			compactLodLevel(levelData, compactData, lodLevels_[lodLevel]);
			REGEN_INFO("LOD level " << lodLevels_.size() - 1 << ":"
									<< " #faces=" << lodLevels_[lodLevel].size()
									<< " #verts=" << compactData.pos.size()
									<< " #collapses=" << collapseCount
									<< " offset=" << compactData.offset
									<< " target=" << lodFaces[lodLevel]);
			// increment vertex offset for next LOD level
			vertexOffset_ += compactData.pos.size();
		}
	}

	applyAttributes();
}

uint32_t MeshSimplifier::createOutputAttributes() {
	bool useOriginal = useOriginalData();
	uint32_t numVertices = inputPos_->numVertices();
	uint32_t numIndices = 0;
	for (auto &faces: lodLevels_) {
		numIndices += faces.size() * 3;
	}
	for (auto &lod: lodData_) {
		numVertices += lod.pos.size();
	}
	if (!useOriginal) {
		numVertices -= inputPos_->numVertices();
		numIndices -= lodLevels_.front().size() * 3;
	}
	REGEN_INFO("Mesh simplified:"
					   << " #lods=" << (lodLevels_.size() - (useOriginal ? 0 : 1))
					   << " #faces=" << numIndices / 3
					   << " #verts=" << numVertices
					   << " #indices=" << numIndices);

	// create a new index buffer
	outputIndices_ = createIndexInput(numIndices, numVertices);
	auto v_indices = (byte*) outputIndices_->clientData();
	auto indexType = outputIndices_->baseType();
	uint32_t v_idx = 0;
	for (uint32_t lod_i = (useOriginal ? 0 : 1); lod_i < lodLevels_.size(); ++lod_i) {
		auto &faces = lodLevels_[lod_i];
		auto vertexOffset = (lod_i == 0 ? 0u : lodData_[lod_i - 1].offset);
		for (auto &face: faces) {
			// faces count from 0 to numVertices, so we need to add the vertexOffset
			setIndexValue(v_indices, indexType, v_idx++, vertexOffset + face.v0);
			setIndexValue(v_indices, indexType, v_idx++, vertexOffset + face.v1);
			setIndexValue(v_indices, indexType, v_idx++, vertexOffset + face.v2);
		}
	}

	// allocate new attributes
	outputPos_ = ref_ptr<ShaderInput3f>::alloc("pos");
	outputPos_->setVertexData(numVertices);
	for (auto &typedAttribute: inputAttributes_) {
		auto att = ShaderInput::create(typedAttribute.second);
		att->setVertexData(numVertices);
		outputAttributes_.emplace_back(typedAttribute.first, att);
	}
	// and copy data into the new attributes using std::memcpy into clientData() of the new attributes
	// first copy the original mesh data to the beginning of the new attributes
	if (useOriginal) {
		std::memcpy(outputPos_->clientData(), inputPos_->clientData(), inputPos_->inputSize());
		for (uint32_t att_i = 0; att_i < inputAttributes_.size(); ++att_i) {
			auto &in = inputAttributes_[att_i].second;
			auto &out = outputAttributes_[att_i].second;
			std::memcpy(out->clientData(), in->clientData(), in->inputSize());
		}
	}
	// then copy the LOD data into the new attributes
	for (auto &lod: lodData_) {
		std::memcpy(
				outputPos_->clientData() + lod.offset * outputPos_->elementSize(),
				(byte *) lod.pos.data(),
				lod.pos.size() * outputPos_->elementSize());
		for (unsigned int attributeIndex = 0; attributeIndex < lod.attributes.size(); ++attributeIndex) {
			auto &in = lod.attributes[attributeIndex];
			auto &out = outputAttributes_[attributeIndex].second;
			std::memcpy(
					out->clientData() + lod.offset * out->elementSize(),
					in->clientData(),
					in->inputSize());
		}
	}

	return numVertices;
}

void MeshSimplifier::applyAttributes() {
	// create the output attributes from the generated LOD levels
	auto numVertices = createOutputAttributes();

	// remove the input attributes from the mesh
	mesh_->removeInput(inputPos_);
	for (auto &pair: inputAttributes_) {
		mesh_->removeInput(pair.second);
	}

	// add the output attributes to the mesh
	mesh_->set_numVertices(numVertices);
	mesh_->begin(Mesh::INTERLEAVED);
	auto indexRef = mesh_->setIndices(outputIndices_, numVertices);
	mesh_->setInput(outputPos_);
	for (auto &pair: outputAttributes_) {
		mesh_->setInput(pair.second);
	}
	mesh_->end();

	// create the mesh LODs
	std::vector<Mesh::MeshLOD> meshLODs(useOriginalData() ? lodLevels_.size() : lodLevels_.size() - 1);
	uint32_t indexOffset = 0, vertexOffset = 0, lodIndex = 0;
	for (size_t i = (useOriginalData() ? 0 : 1); i < lodLevels_.size(); ++i) {
		auto &faces = lodLevels_[i];
		meshLODs[lodIndex].d->numIndices = faces.size() * 3;
		meshLODs[lodIndex].d->indexOffset = indexRef->address() + indexOffset * outputIndices_->dataTypeBytes();
		meshLODs[lodIndex].d->vertexOffset = vertexOffset;
		if (i == 0) {
			meshLODs[lodIndex].d->numVertices = inputPos_->numVertices();
		} else {
			meshLODs[lodIndex].d->numVertices = lodData_[i - 1].pos.size();
		}
		indexOffset += meshLODs[lodIndex].d->numIndices;
		vertexOffset += meshLODs[lodIndex].d->numVertices;
		lodIndex += 1;
	}
	mesh_->setMeshLODs(meshLODs);
	mesh_->activateLOD(0);
}

#include "mesh-simplifier.h"

using namespace regen;

MeshSimplifier::MeshSimplifier(const ref_ptr<Mesh> &mesh) : mesh_(mesh) {
	// lookup mesh attributes
	for (auto &in : mesh_->inputContainer()->inputs()) {
		if (in.in_->isVertexAttribute()) {
			createAttribute(in);
		}
	}
	inputIndices_ = ref_ptr<ShaderInput1ui>::dynamicCast(mesh_->inputContainer()->indices());
	// validate attributes
	if (!inputAttributes_.contains(AttributeType::POSITION)) {
		REGEN_WARN("Mesh has no position attribute, no LoD will be generated.");
		hasValidAttributes_ = false;
	}
	// TODO: probably disable simplification in case some unsupported vertex attributes are used?

	quadrics_.resize(mesh_->inputContainer()->numVertices());
}

AttributeType getAttributeType(const NamedShaderInput &namedAttribute) {
	// for now classify by name prefix, e.g. "pos" for position
	if (namedAttribute.name_.find("pos") == 0) {
		return AttributeType::POSITION;
	} else if (namedAttribute.name_.find("nor") == 0) {
		return AttributeType::NORMAL;
	} else if (namedAttribute.name_.find("texco") == 0) {
		return AttributeType::TEXCOORD;
	} else if (namedAttribute.name_.find("uv") == 0) {
		return AttributeType::TEXCOORD;
	} else if (namedAttribute.name_.find("col") == 0) {
		return AttributeType::COLOR;
	} else if (namedAttribute.name_.find("tan") == 0) {
		return AttributeType::TANGENT;
	} else {
		return AttributeType::UNKNOWN;
	}
}

void MeshSimplifier::createAttribute(const NamedShaderInput &namedAttribute) {
	// for now classify by name prefix, e.g. "pos" for position
	auto attributeType = getAttributeType(namedAttribute);
	inputAttributes_[attributeType].push_back(namedAttribute);
	if (attributeType == AttributeType::UNKNOWN) {
		REGEN_WARN("Unknown attribute type for '" << namedAttribute.name_ << "'.");
	}
}

void MeshSimplifier::computeQuadrics() {
	auto numOriginalFaces = mesh_->inputContainer()->numIndices() / 3;
	auto &lodLevel0 = lodLevels_.emplace_back();
	lodLevel0.reserve(numOriginalFaces);

	auto m_inputIndices =
		inputIndices_->mapClientData<uint32_t>(ShaderData::READ);
	auto m_pos =
		inputAttributes_[AttributeType::POSITION].front().in_->mapClientData<Vec3f>(ShaderData::READ);
	for (size_t i = 0; i < numOriginalFaces; ++i) {
		// create triangle faces of the original mesh
		auto j = i * 3;
		lodLevel0.emplace_back(
			m_inputIndices.r[j],
			m_inputIndices.r[j+1],
			m_inputIndices.r[j+2]);

		// update vertex quadrics
		auto &p0 = m_pos.r[lodLevel0[i].v0];
		auto &p1 = m_pos.r[lodLevel0[i].v1];
		auto &p2 = m_pos.r[lodLevel0[i].v2];
		auto faceNormal = (p1 - p0).cross(p2 - p0);
		faceNormal.normalize();
		Quadric faceQuadric(
				faceNormal.x,
				faceNormal.y,
				faceNormal.z,
				-faceNormal.dot(p0));
		quadrics_[lodLevel0[i].v0] += faceQuadric;
		quadrics_[lodLevel0[i].v1] += faceQuadric;
		quadrics_[lodLevel0[i].v2] += faceQuadric;
	}
}

void MeshSimplifier::run() {
	if (!hasValidAttributes_) {
		return;
	}
	// TODO: support other primitive types than triangles
	// FIXME: check for primitive type

	//int LOD0 = originalTriangleCount;
	//int LOD1 = LOD0 * 0.5;
	//int LOD2 = LOD0 * 0.25;

	// Generate the first LOD level in lodLevels_[0].
	// Also compute the quadrics for the original mesh on the way.
	computeQuadrics();

	// TODO Build a list of valid edges: unique (v1, v2) pairs (undirected).
	//	- edge has Optimal collapse position (via quadric minimization), and cost = quadric error at that position.

	// TODO Iteratively collapse the cheapest edge, updating affected geometry and edge costs.
	//	- Stop once your desired number of triangles is reached (or vertices), and record the current mesh as LOD level.
	//
	/** while (triangleCount > targetCount) {
		Edge e = edgeQueue.pop(); // smallest cost edge

		if (!stillValid(e)) continue; // skip if topologically invalid now

		collapseEdge(e);

		// Update affected vertices & faces
		updateQuadrics(e);
		updateEdgeCosts(around e);
	} */
	applyAttributes();
}

void MeshSimplifier::applyAttributes() {
	// remove old attributes
	for (auto &pair : inputAttributes_) {
		for (auto &namedInput : pair.second) {
			mesh_->disjoinShaderInput(namedInput.in_);
		}
	}
	if (inputIndices_.get()) {
		mesh_->disjoinShaderInput(inputIndices_);
	}
	// set new attributes
	// TODO: also allocate here from the LOD faces
	for (auto &pair : outputAttributes_) {
		for (auto &namedInput : pair.second) {
			mesh_->joinShaderInput(namedInput.in_);
		}
	}
	if (outputIndices_.get()) {
		mesh_->joinShaderInput(outputIndices_);
	}
}

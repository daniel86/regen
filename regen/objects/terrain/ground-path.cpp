
#include "../lod/tessellation.h"
#include "ground-path.h"

#include <ranges>

#include "regen/compute/bezier.h"

using namespace regen;

GroundPath::GroundPath(const Config &cfg)
		: Mesh(GL_TRIANGLES, cfg.updateHint) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	setBufferMapMode(cfg.mapMode);
	setClientAccessMode(cfg.accessMode);
	updateAttributes(cfg);
}

GroundPath::Config::Config()
		: levelOfDetails({0}),
		  posScale(Vec3f::one()),
		  rotation(Vec3f::zero()),
		  texcoScale(Vec2f::one()),
		  isTexCoordRequired(false),
		  isNormalRequired(false),
		  isTangentRequired(false),
		  discRadius(1.0f) {
}

void GroundPath::generateLODLevel(
		const Config &cfg,
		uint32_t numCurveSamples,
		uint32_t vertexOffset,
		uint32_t indexOffset) {
#define _VERTEX_DATA(T, ATTR) (T*) ATTR->clientBuffer()->clientData(0)
	Vec3f* v_pos = _VERTEX_DATA(Vec3f, pos_);
	Vec3f* v_nor = (cfg.isNormalRequired ? _VERTEX_DATA(Vec3f, nor_) : nullptr);
	Vec4f* v_tan = (cfg.isTangentRequired ? _VERTEX_DATA(Vec4f, tan_) : nullptr);
	Vec2f* v_texco = (cfg.isTexCoordRequired ? _VERTEX_DATA(Vec2f, texco_) : nullptr);
	byte* indices = _VERTEX_DATA(byte, indices_);
	auto indexType = indices_->baseType();
#undef _VERTEX_DATA

	uint32_t vertexIndex = vertexOffset;
	for (auto &spline: cfg.splines) {
		float lengthAccum = 0.0f;
		Vec2f prev;

		vertexOffset = vertexIndex;

		for (uint32_t sampleIdx = 0; sampleIdx <= numCurveSamples; ++sampleIdx) {
			float t = (float)sampleIdx / (float)numCurveSamples;
			// point on the spline
			Vec2f p = spline.sample(t);
			// create two points at left and right of the spline
			// (perpendicular to the tangent)
			Vec2f tan = spline.tangent(t);
			Vec2f right(-tan.y, tan.x); // perpendicular
			right.normalize();
			// push last/first sample a bit further along the tangent to avoid
			// artifacts when two path segments meet at sharp angles
			if (sampleIdx == numCurveSamples) {
				p += tan * cfg.pathWidth * 0.25f;
			}
			if (sampleIdx == 0) {
				p -= tan * cfg.pathWidth * 0.25f;
			}

			Vec2f leftPos  = p - right * cfg.pathWidth * 0.5f;
			Vec2f rightPos = p + right * cfg.pathWidth * 0.5f;

			// left+right vertex
			v_pos[vertexIndex+0] = Vec3f(leftPos.x, cfg.baseHeight, leftPos.y) * cfg.posScale;
			v_pos[vertexIndex+1] = Vec3f(rightPos.x, cfg.baseHeight, rightPos.y) * cfg.posScale;

			minPosition_.setMin(v_pos[vertexIndex+0]);
			minPosition_.setMin(v_pos[vertexIndex+1]);
			maxPosition_.setMax(v_pos[vertexIndex+0]);
			maxPosition_.setMax(v_pos[vertexIndex+1]);
			if (cfg.isNormalRequired) {
				v_nor[vertexIndex+0] = Vec3f(0.0f, 1.0f, 0.0f);
				v_nor[vertexIndex+1] = Vec3f(0.0f, 1.0f, 0.0f);
			}
			if (cfg.isTexCoordRequired) {
				if (sampleIdx > 0) {
					// accumulate length
					lengthAccum += (p - prev).length();
				}
				v_texco[vertexIndex+0] = Vec2f(0.0f, lengthAccum) * cfg.texcoScale;
				v_texco[vertexIndex+1] = Vec2f(1.0f, lengthAccum) * cfg.texcoScale;
			}
			if (cfg.isTangentRequired) {
				Vec4f tan4(tan.x, 0.0f, tan.y, 1.0f);
				v_tan[vertexIndex+0] = tan4;
				v_tan[vertexIndex+1] = tan4;
			}
			vertexIndex += 2;

			// Create two triangles per sample, connecting to previous sample
			if (sampleIdx > 0) {
				uint32_t baseIdx = vertexOffset + (sampleIdx - 1) * 2;
				// triangle 1
				setIndexValue(indices, indexType, indexOffset++, baseIdx + 0);
				setIndexValue(indices, indexType, indexOffset++, baseIdx + 1);
				setIndexValue(indices, indexType, indexOffset++, baseIdx + 2);
				// triangle 2
				setIndexValue(indices, indexType, indexOffset++, baseIdx + 1);
				setIndexValue(indices, indexType, indexOffset++, baseIdx + 3);
				setIndexValue(indices, indexType, indexOffset++, baseIdx + 2);
			}
			prev = p;
		}
	}
}

void GroundPath::updateAttributes(const Config &cfg) {
	std::vector<uint32_t> numSamplesPerLOD;
	uint32_t vertexOffset = 0;
	uint32_t indexOffset = 0;
	uint32_t numCurves = cfg.splines.size();

	for (auto &lod: cfg.levelOfDetails) {
		uint32_t numCurveSamples = 4u * pow(2u, lod);
		uint32_t numVerticesPerCurve = (numCurveSamples + 1) * 2;
		uint32_t numLODVertices = numCurves * numVerticesPerCurve;
		uint32_t numLODTris = numCurves * numCurveSamples * 2;
		numSamplesPerLOD.push_back(numCurveSamples);
		auto &x = meshLODs_.emplace_back();
		// two vertices per sample, one left side, one right side
		x.d->numVertices = numLODVertices;
		// these two vertices form two triangles with vertices from the previous sample
		x.d->numIndices = numLODTris * 3;
		x.d->vertexOffset = vertexOffset;
		x.d->indexOffset = indexOffset;
		vertexOffset += x.d->numVertices;
		indexOffset += x.d->numIndices;
	}
	uint32_t numVertices = vertexOffset;
	uint32_t numIndices = indexOffset;

	pos_->setVertexData(numVertices);
	if (cfg.isNormalRequired) {
		nor_->setVertexData(numVertices);
	}
	if (cfg.isTangentRequired) {
		tan_->setVertexData(numVertices);
	}
	if (cfg.isTexCoordRequired) {
		texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
		texco_->setVertexData(numVertices);
	}
	indices_ = createIndexInput(numIndices, numVertices);

	minPosition_ = Vec3f::posMax();
	maxPosition_ = Vec3f::negMax();
	for (auto i = 0u; i < numSamplesPerLOD.size(); ++i) {
		generateLODLevel(cfg, numSamplesPerLOD[i],
						 meshLODs_[i].d->vertexOffset,
						 meshLODs_[i].d->indexOffset);
	}

	// Set up the vertex attributes
	auto indexRef = setIndices(indices_, numVertices);
	setInput(pos_);
	if (cfg.isNormalRequired) {
		setInput(nor_);
	}
	if (cfg.isTexCoordRequired) {
		setInput(texco_);
	}
	if (cfg.isTangentRequired) {
		setInput(tan_);
	}
	updateVertexData();

	for (auto &x: meshLODs_) {
		// add the index buffer offset (in number of bytes)
		x.d->indexOffset = indexRef->address() + x.d->indexOffset * indices_->dataTypeBytes();
	}
	activateLOD(0);
}

static inline void addPathSpline(
		GroundPath::Config &cfg,
		const WayPoint *source,
		const WayPoint *target,
		const Vec2f &dir_a,
		const Vec2f &dir_d) {
	Vec2f p0 = source->position2D();
	Vec2f p3 = target->position2D();

	float handleScale = (p3 - p0).length() * 0.5f;
	Vec2f p1 = p0 - dir_a * handleScale * 0.5f;
	Vec2f p2 = p3 + dir_d * handleScale * 0.5f;

	math::Bezier<Vec2f> &spline = cfg.splines.emplace_back();
	spline.p0 = p0;
	spline.p1 = p1;
	spline.p2 = p2;
	spline.p3 = p3;
}

static inline void addPathSpline(
		GroundPath::Config &cfg,
		const WayPoint *source,
		const WayPoint *target,
		const WayPoint *approachingFrom,
		const WayPoint *departingTo) {
	Vec2f p0 = source->position2D();
	Vec2f p3 = target->position2D();

	Vec2f dir_a = p0 - approachingFrom->position2D();
	float len_a = dir_a.length();
	if (len_a > 1e-4f) {
		dir_a /= len_a;
	} else {
		dir_a = p3 - p0;
		dir_a.normalize();
	}

	Vec2f dir_d = departingTo->position2D() - p3;
	float len_d = dir_d.length();
	if (len_d > 1e-4f) {
		dir_d /= len_d;
	} else {
		dir_d = p3 - p0;
		dir_d.normalize();
	}

	float handleScale = (p3 - p0).length() * 0.5f;
	Vec2f p1 = p0 + dir_a * handleScale * 0.5f;
	Vec2f p2 = p3 - dir_d * handleScale * 0.5f;

	math::Bezier<Vec2f> &spline = cfg.splines.emplace_back();
	spline.p0 = p0;
	spline.p1 = p1;
	spline.p2 = p2;
	spline.p3 = p3;
}

static inline Vec2f computeArrivalDirection(
		const WayPoint *from,
		const WayPoint *to,
		const WayPoint *next) {
	Vec2f dir = (to->position2D() - next->position2D()) * 0.5f +
				(from->position2D() - to->position2D()) * 0.5f;
	float len = dir.length();
	if (len > 1e-4f) {
		dir /= len;
	} else {
		dir = to->position2D() - from->position2D();
		dir.normalize();
	}
	return dir;
}

ref_ptr<GroundPath> GroundPath::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto scene = ctx.scene();
	ref_ptr<WorldModel> world = scene->worldModel();

	GroundPath::Config cfg;
	if (input.hasAttribute("lod-levels")) {
		auto lodVec = input.getValue<Vec4ui>("lod-levels", Vec4ui::create(0));
		cfg.levelOfDetails.resize(4);
		cfg.levelOfDetails[0] = lodVec.x;
		cfg.levelOfDetails[1] = lodVec.y;
		cfg.levelOfDetails[2] = lodVec.z;
		cfg.levelOfDetails[3] = lodVec.w;
	} else {
		cfg.levelOfDetails.push_back(input.getValue<uint32_t>("lod-level", 0));
	}
	cfg.isTexCoordRequired = input.getValue<bool>("use-texco", false);
	cfg.isNormalRequired = input.getValue<bool>("use-normal", false);
	cfg.isTangentRequired = input.getValue<bool>("use-tangent", false);
	cfg.posScale = input.getValue<Vec3f>("scaling", Vec3f::one());
	cfg.rotation = input.getValue<Vec3f>("rotation", Vec3f::zero());
	cfg.texcoScale = input.getValue<Vec2f>("texco-scaling", Vec2f::one());
	cfg.baseHeight = input.getValue<float>("base-height", 0.0f);
	cfg.pathWidth = input.getValue<float>("path-width", 4.0f);
	cfg.updateHint.frequency = input.getValue<BufferUpdateFrequency>(
		"update-frequency", BUFFER_UPDATE_NEVER);
	cfg.updateHint.scope = input.getValue<BufferUpdateScope>(
		"update-scope", BUFFER_UPDATE_FULLY);
	cfg.accessMode = input.getValue<ClientAccessMode>(
		"access-mode", BUFFER_CPU_WRITE);
	cfg.mapMode = input.getValue<BufferMapMode>(
		"map-mode", BUFFER_MAP_DISABLED);

	for (auto &c : world->wayPointConnections) {
		const WayPoint *wp0 = c.first.get();
		const WayPoint *wp1 = c.second.get();
		// We make an assumption here that character approach this wp0 to reach wp1 by approaching
		// from wp closest to wp1, and vice versa.
		const WayPoint *wp_a = world->getClosestTo(*wp0, {wp1});
		const WayPoint *wp_d = world->getClosestTo(*wp1, {wp0});
		if (!wp_a) wp_a = wp0;
		if (!wp_d) wp_d = wp1;
		addPathSpline(cfg, wp0, wp1, wp_a, wp_d);
	}

	for (auto &place : world->places) {
		for (const auto &val: place->pathWays() | std::views::values) {
			for (auto &typedPath : val) {
				const WayPoint *wp0 = typedPath.front().get();
				Vec2f startDir, stopDir;

				startDir = computeArrivalDirection(
					typedPath.back().get(),
					wp0,
					typedPath.size() > 1 ? typedPath[1].get() : typedPath[0].get());

				for (uint32_t i = 1; i < typedPath.size(); ++i) {
					const WayPoint *wp1 = typedPath[i].get();
					const WayPoint *wp_d = (i + 1 < typedPath.size() ? typedPath[i + 1].get() : typedPath[0].get());
					stopDir = computeArrivalDirection(wp0, wp1, wp_d);
					addPathSpline(cfg, wp0, wp1, startDir, stopDir);
					wp0 = wp1;
					startDir = stopDir;
				}
				// add closing segment
				if (typedPath.size() >= 2) {
					const WayPoint *wp1 = typedPath[0].get();
					stopDir = computeArrivalDirection(wp0, wp1, typedPath[1].get());
					addPathSpline(cfg, wp0, wp1, startDir, stopDir);
				}
			}
		}
	}

	return ref_ptr<GroundPath>::alloc(cfg);
}

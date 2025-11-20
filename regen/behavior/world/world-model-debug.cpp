#include "world-model-debug.h"

#include "character-object.h"

using namespace regen;

WorldModelDebug::WorldModelDebug(const ref_ptr<WorldModel> &world)
		: StateNode(),
		  HasShader("regen.models.lines"),
		  world_(world),
		  lineLocation_(-1),
		  vbo_(0) {
	lineColor_ = ref_ptr<ShaderInput3f>::alloc("lineColor");
	lineColor_->setUniformData(Vec3f::one());
	state()->setInput(lineColor_);
	state()->joinStates(shaderState_);
	lineVertices_ = ref_ptr<ShaderInput3f>::alloc("lineVertices");
	lineVertices_->setVertexData(2);
	// Create and set up the VAO and VBO
	vao_ = ref_ptr<VAO>::alloc();
	bufferSize_ = sizeof(GLfloat) * 3 * lineVertices_->numVertices();
	glGenBuffers(1, &vbo_);
}

void WorldModelDebug::drawLine(const Vec3f &from, const Vec3f &to, const Vec3f &color) {
	auto rs = RenderState::get();
	lineColor_->setUniformData(color);
	if (lineLocation_ < 0) {
		lineLocation_ = shaderState_->shader()->uniformLocation(lineColor_->name());
	}
	lineColor_->enableUniform(lineLocation_);
	// update cpu-side vertex data
	lineVertices_->setVertex(0, from);
	lineVertices_->setVertex(1, to);
	// update gpu-side vertex data
	{
		auto mappedClientData = lineVertices_->mapClientDataRaw(BUFFER_GPU_READ);
		glBufferData(GL_ARRAY_BUFFER, bufferSize_, mappedClientData.r, GL_DYNAMIC_DRAW);
	}
	// draw the line
	rs->vao().apply(vao_->id());
	lineVertices_->enableAttribute(0);
	glDrawArrays(GL_LINES, 0, 2);
}

void WorldModelDebug::drawCircle(const Vec3f &center, float radius, const Vec3f &color) {
	// draw a circle
	const int segments = 32;
	const float angleStep = 2.0f * M_PI / segments;
	for (int i = 0; i < segments; i++) {
		float angle = i * angleStep;
		float nextAngle = (i + 1) * angleStep;
		Vec3f from = center + Vec3f(std::cos(angle) * radius, 0, std::sin(angle) * radius);
		Vec3f to = center + Vec3f(std::cos(nextAngle) * radius, 0, std::sin(nextAngle) * radius);
		drawLine(from, to, color);
	}
}

void WorldModelDebug::drawCrossXZ(const Vec3f &center, float radius, const Vec3f &color) {
	drawLine(center + Vec3f(-radius, 0, -radius), center + Vec3f(radius, 0, radius), color);
	drawLine(center + Vec3f(-radius, 0, radius), center + Vec3f(radius, 0, -radius), color);
}

void WorldModelDebug::drawArrow(const Vec3f &origin, const Vec3f &direction, float length, const Vec3f &color) {
	Vec3f dirNorm = direction;
	dirNorm.normalize();
	Vec3f end = origin + dirNorm * length;
	drawLine(origin, end, color);
	// draw arrow head
	Vec3f right = Vec3f(-dirNorm.z, 0.0f, dirNorm.x); // perpendicular in XZ plane
	float headSize = length * 0.2f;
	drawLine(end, end - dirNorm * headSize + right * headSize * 0.5f, color);
	drawLine(end, end - dirNorm * headSize - right * headSize * 0.5f, color);
}

inline Vec3f toVec3(const Vec2f &v, float y) {
	return {v.x, y, v.y};
}

static Vec3f getAffordanceColor(ActionType type) {
	switch (type) {
		case ActionType::CONVERSING:
			return Vec3f(1.0f, 1.0f, 0.0f); // yellow
		case ActionType::SITTING:
			return Vec3f(1.0f, 0.0f, 0.0f);
		case ActionType::SLEEPING:
			return Vec3f(0.0f, 0.0f, 1.0f);
		case ActionType::PRAYING:
			return Vec3f(1.0f, 0.0f, 1.0f); // magenta
		case ActionType::OBSERVING:
			return Vec3f(0.0f, 1.0f, 1.0f); // cyan
		default:
			return Vec3f(1.0f, 1.0f, 1.0f); // white
	}
}

void WorldModelDebug::drawCurve(
		const math::Bezier<Vec2f> &curve,
		float height, int segments, const Vec3f &color) {
	Vec3f prevPoint = toVec3(curve.sample(0.0f), height);
	for (int i = 1; i <= segments; i++) {
		float t = static_cast<float>(i) / static_cast<float>(segments);
		Vec3f currPoint = toVec3(curve.sample(t), height);
		drawLine(prevPoint, currPoint, color);
		prevPoint = currPoint;
	}
}

void WorldModelDebug::traverse(regen::RenderState *rs) {
	state()->enable(rs);
	rs->arrayBuffer().apply(vbo_);
	for (auto &shape: world_->worldObjects()) {
		auto pos = shape->position3D();
		drawCircle(pos, shape->radius(), Vec3f(0.0f, 1.0f, 0.0f));
		// Draw affordances
		for (auto &aff : shape->affordances()) {
			Vec3f color = getAffordanceColor(aff->type);
			if (aff->minDistance > 0.0f) {
				drawCircle(pos, aff->minDistance, color);
			}
			if (aff->radius != shape->radius()) {
				drawCircle(pos, aff->radius, color * 0.5f);
			}
			// Draw slots
			for (int i = 0; i < aff->slotCount; i++) {
				auto &slotPos = aff->slotPosition(i);
				Vec3f slotColor = (aff->users[i] ? Vec3f(1.0f, 0.0f, 0.0f) : Vec3f(0.0f, 0.0f, 1.0f));
				drawCrossXZ(slotPos, 0.1f, slotColor);
			}
		}
		// draw group center positions
		if (shape->objectType() == ObjectType::LOCATION) {
			auto &loc = static_cast<Location&>(*shape.get());
			for (int32_t groupIdx = 0; groupIdx < loc.numGroups(); groupIdx++) {
				auto *group = loc.group(groupIdx);
				if (group->numMembers() == 0) {
					REGEN_WARN("Location '" << loc.name() << "' has group with zero members.");
					continue;
				}
				drawCrossXZ(group->position3D(), 0.2f, Vec3f(1.0f, 0.0f, 1.0f));
				// draw a line to each member
				for (uint32_t memberIdx = 0; memberIdx < group->numMembers(); memberIdx++) {
					auto *member = group->member(memberIdx);
					drawLine(group->position3D(), member->position3D(), Vec3f(1.0f, 0.0f, 1.0f));
				}
			}
		}
		if (shape->objectType() == ObjectType::CHARACTER || shape->objectType() == ObjectType::ANIMAL) {
			auto *character = static_cast<CharacterObject *>(shape.get());
			if (character->hasKnowledgeBase()) {
				auto *kb = character->knowledgeBase();
				// draw arrow to navigation target
				auto &navTarget = kb->hasNavigationTarget() ?
					kb->navigationTarget() : kb->interactionTarget();
				if (navTarget.object.get()) {
					Vec3f targetPos = navTarget.object->position3D();
					drawArrow(pos, targetPos - pos, 2.0f, Vec3f(1.0f, 1.0f, 0.0f));
				}

				auto &interactionTarget = kb->interactionTarget();
				if (interactionTarget.affordance.get()) {
					auto &affordancePos = interactionTarget.affordance->slotPosition(interactionTarget.affordanceSlot);
					drawLine(affordancePos, pos, Vec3f(1.0f, 0.0f, 1.0f));
				}
			}
			if (character->hasNPCController()) {
				auto *controller = character->npcController();
				if (controller->isNavigationApproaching()) {
					const math::Bezier<Vec2f> &curvePath = controller->currentCurvePath();
					drawCurve(curvePath, pos.y, 10, Vec3f(0.0f, 1.0f, 1.0f));
				}
			}
		}
	}
	// Draw global navigation paths
	for (auto &path : world_->wayPointConnections) {
		auto from = path.first->position3D();
		auto to = path.second->position3D();
		drawLine(from, to, Vec3f(1.0f, 0.0f, 0.0f));
	}
	// Draw place-specific things
	for (auto &place : world_->places) {
		// Draw patrol and stroll paths
		for (auto &patrolPath : place->getPathWays(PathwayType::PATROL)) {
			for (size_t i = 0; i < patrolPath.size(); i++) {
				auto from = patrolPath[i]->position3D();
				auto to = patrolPath[(i + 1) % patrolPath.size()]->position3D();
				drawLine(from, to, Vec3f(0.0f, 0.0, 1.0f));
			}
		}
		for (auto &strollPath : place->getPathWays(PathwayType::STROLL)) {
			for (size_t i = 0; i < strollPath.size(); i++) {
				auto from = strollPath[i]->position3D();
				auto to = strollPath[(i + 1) % strollPath.size()]->position3D();
				drawLine(from, to, Vec3f(1.0f, 0.5f, 0.0f));
			}
		}
	}
	state()->disable(rs);
}

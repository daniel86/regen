#include "spatial-index-debug.h"
#include "orthogonal-projection.h"
#include <regen/gl-types/gl-util.h>
#include "regen/camera/reflection-camera.h"

using namespace regen;

SpatialIndexDebug::SpatialIndexDebug(const ref_ptr<SpatialIndex> &index)
		: StateNode(),
		  HasShader("regen.models.lines"),
		  index_(index),
		  lineLocation_(-1),
		  vbo_(0) {
	lineColor_ = ref_ptr<ShaderInput3f>::alloc("lineColor");
	lineColor_->setUniformData(Vec3f(1.0f));
	state()->setInput(lineColor_);
	state()->joinStates(shaderState_);
	lineVertices_ = ref_ptr<ShaderInput3f>::alloc("lineVertices");
	lineVertices_->setVertexData(2);
	// Create and set up the VAO and VBO
	vao_ = ref_ptr<VAO>::alloc();
	bufferSize_ = sizeof(GLfloat) * 3 * lineVertices_->numVertices();
	glGenBuffers(1, &vbo_);
}

void SpatialIndexDebug::drawLine(const Vec3f &from, const Vec3f &to, const Vec3f &color) {
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

void SpatialIndexDebug::drawCircle(const Vec3f &center, float radius, const Vec3f &color) {
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

void SpatialIndexDebug::drawBox(const BoundingBox &box) {
	auto *boxVertices = box.boxVertices();
	Vec3f lineColor = Vec3f(1.0f, 1.0f, 0.0f);
	// draw the 12 lines of the box
	drawLine(boxVertices[0], boxVertices[1], lineColor);
	drawLine(boxVertices[1], boxVertices[3], lineColor);
	drawLine(boxVertices[3], boxVertices[2], lineColor);
	drawLine(boxVertices[4], boxVertices[5], lineColor);
	drawLine(boxVertices[5], boxVertices[7], lineColor);
	drawLine(boxVertices[7], boxVertices[6], lineColor);
	drawLine(boxVertices[1], boxVertices[5], lineColor);
	drawLine(boxVertices[3], boxVertices[7], lineColor);
	drawLine(boxVertices[2], boxVertices[0], lineColor);
	drawLine(boxVertices[2], boxVertices[6], lineColor);
	drawLine(boxVertices[6], boxVertices[4], lineColor);
	drawLine(boxVertices[0], boxVertices[4], lineColor);
	drawLine(boxVertices[0], boxVertices[5], lineColor);
}

void SpatialIndexDebug::drawSphere(const BoundingSphere &sphere) {
	auto &center = sphere.getShapeOrigin();
	auto radius = sphere.radius();
	Vec3f color = Vec3f(0.0f, 1.0f, 1.0f);

	// draw a simple sphere
	const int segments = 32;
	const float angleStep = 2.0f * M_PI / segments;

	// Draw circle in the xy-plane
	for (int i = 0; i < segments; i++) {
		float angle = i * angleStep;
		float nextAngle = (i + 1) * angleStep;
		Vec3f from = center + Vec3f(std::cos(angle) * radius, std::sin(angle) * radius, 0);
		Vec3f to = center + Vec3f(std::cos(nextAngle) * radius, std::sin(nextAngle) * radius, 0);
		drawLine(from, to, color);
	}

	// Draw circle in the xz-plane
	for (int i = 0; i < segments; i++) {
		float angle = i * angleStep;
		float nextAngle = (i + 1) * angleStep;
		Vec3f from = center + Vec3f(std::cos(angle) * radius, 0, std::sin(angle) * radius);
		Vec3f to = center + Vec3f(std::cos(nextAngle) * radius, 0, std::sin(nextAngle) * radius);
		drawLine(from, to, color);
	}

	// Draw circle in the yz-plane
	for (int i = 0; i < segments; i++) {
		float angle = i * angleStep;
		float nextAngle = (i + 1) * angleStep;
		Vec3f from = center + Vec3f(0, std::cos(angle) * radius, std::sin(angle) * radius);
		Vec3f to = center + Vec3f(0, std::cos(nextAngle) * radius, std::sin(nextAngle) * radius);
		drawLine(from, to, color);
	}
}

void SpatialIndexDebug::drawFrustum(const Frustum &frustum, const Vec3f &color) {
	auto &vertices = frustum.points;
	// near plane
	drawLine(vertices[0], vertices[1], color);
	drawLine(vertices[1], vertices[2], color);
	drawLine(vertices[2], vertices[3], color);
	drawLine(vertices[3], vertices[0], color);
	drawLine(vertices[1], vertices[3], color);
	drawLine(vertices[0], vertices[2], color);
	// far plane
	drawLine(vertices[4], vertices[5], color);
	drawLine(vertices[5], vertices[6], color);
	drawLine(vertices[6], vertices[7], color);
	drawLine(vertices[7], vertices[4], color);
	drawLine(vertices[5], vertices[7], color);
	drawLine(vertices[4], vertices[6], color);
	// connect near and far plane
	drawLine(vertices[0], vertices[4], color);
	drawLine(vertices[1], vertices[5], color);
	drawLine(vertices[2], vertices[6], color);
	drawLine(vertices[3], vertices[7], color);

}

inline Vec3f toVec3(const Vec2f &v, float y) {
	return {v.x, y, v.y};
}

void SpatialIndexDebug::debugFrustum(const Frustum &frustum, const Vec3f &color) {
	//drawFrustum(frustum, color);

	OrthogonalProjection proj(frustum);
	auto &vertices = proj.points;
	Vec3f orthoColor = Vec3f(0.0f, 1.0f, 0.0f);
	const auto h = 6.0f;
	// draw lines of the frustum
	for (size_t i = 0; i < vertices.size() - 1; i++) {
		drawLine(toVec3(vertices[i], h), toVec3(vertices[i + 1], h), orthoColor);
	}
	drawLine(toVec3(vertices.back(), h), toVec3(vertices.front(), h), orthoColor);
}

void SpatialIndexDebug::traverse(regen::RenderState *rs) {
	state()->enable(rs);
	rs->arrayBuffer().apply(vbo_);
	for (auto &shape: index_->shapes()) {
		for (auto &instance: shape.second) {
			switch (instance->shapeType()) {
				case BoundingShapeType::BOX: {
					auto box = dynamic_cast<BoundingBox *>(instance.get());
					drawBox(*box);
					break;
				}
				case BoundingShapeType::SPHERE: {
					auto sphere = dynamic_cast<BoundingSphere *>(instance.get());
					drawSphere(*sphere);
					break;
				}
				case BoundingShapeType::FRUSTUM:
					auto frustum = dynamic_cast<Frustum *>(instance.get());
					drawFrustum(*frustum);
					break;
			}
		}
	}
	for (auto &camera: index_->cameras()) {
		for (auto &frustum: camera->frustum()) {
			debugFrustum(frustum, Vec3f(1.0f, 0.0f, 1.0f));
		}
	}
	index_->debugDraw(*this);
	state()->disable(rs);
}

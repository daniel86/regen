#include "spatial-index-debug.h"

using namespace regen;

SpatialIndexDebug::SpatialIndexDebug(const ref_ptr<SpatialIndex> &index)
		: StateNode(),
		  HasShader("regen.models.lines"),
		  HasInput(VBO::USAGE_DYNAMIC),
		  index_(index),
		  lineLocation_(-1),
		  vbo_(0) {
	lineColor_ = ref_ptr<ShaderInput3f>::alloc("lineColor");
	lineColor_->setUniformData(Vec3f(1.0f));
	state()->joinShaderInput(lineColor_);
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
	glBufferData(GL_ARRAY_BUFFER, bufferSize_, lineVertices_->clientData(), GL_DYNAMIC_DRAW);
	// draw the line
	rs->vao().push(vao_->id());
	lineVertices_->enableAttribute(0);
	glDrawArrays(GL_LINES, 0, 2);
	rs->vao().pop();
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
	auto center = sphere.getCenterPosition();
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

void SpatialIndexDebug::drawFrustum(const Frustum &frustum) {

}

void SpatialIndexDebug::traverse(regen::RenderState *rs) {
	state()->enable(rs);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	for (auto &shape: index_->shapes()) {
		switch (shape->shapeType()) {
			case BoundingShapeType::BOX: {
				auto box = dynamic_cast<BoundingBox *>(shape.get());
				drawBox(*box);
				break;
			}
			case BoundingShapeType::SPHERE: {
				auto sphere = dynamic_cast<BoundingSphere *>(shape.get());
				drawSphere(*sphere);
				break;
			}
			case BoundingShapeType::FRUSTUM:
				auto frustum = dynamic_cast<Frustum *>(shape.get());
				drawFrustum(*frustum);
				break;
		}
	}
	index_->debugDraw(*this);
	state()->disable(rs);
	GL_ERROR_LOG();
}

#include "regen/physics/bullet-debug-drawer.h"

using namespace regen;

BulletDebugDrawer::BulletDebugDrawer(const ref_ptr<BulletPhysics> &physics)
		: btIDebugDraw(),
		  StateNode(),
		  HasShader("regen.models.lines"),
		  HasInput(VBO::USAGE_DYNAMIC),
		  physics_(physics),
		  vbo_(0),
		  m_debugMode(DBG_DrawContactPoints | DBG_DrawWireframe){
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

// btIDebugDraw interface
void BulletDebugDrawer::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
	lineColor_->setUniformData(Vec3f(color.getX(), color.getY(), color.getZ()));
	// update cpu-side vertex data
	lineVertices_->setVertex(0, Vec3f(from.getX(), from.getY(), from.getZ()));
	lineVertices_->setVertex(1, Vec3f(to.getX(), to.getY(), to.getZ()));
	// update gpu-side vertex data
	glBufferData(GL_ARRAY_BUFFER, bufferSize_, lineVertices_->clientData(), GL_DYNAMIC_DRAW);
	// draw the line
	renderState_->vao().push(vao_->id());
	lineVertices_->enableAttribute(0);
	glDrawArrays(GL_LINES, 0, 2);
	renderState_->vao().pop();
}

void
BulletDebugDrawer::drawContactPoint(
		const btVector3 &PointOnB,
		const btVector3 &normalOnB,
		btScalar distance,
		int lifeTime,
		const btVector3 &color)
{
    // Set the color for the contact point
    lineColor_->setUniformData(Vec3f(color.getX(), color.getY(), color.getZ()));
    // Calculate the end point of the normal vector
    btVector3 to = PointOnB + normalOnB * distance;
    // Update CPU-side vertex data for the contact point
    lineVertices_->setVertex(0,
    	Vec3f(PointOnB.getX(), PointOnB.getY(), PointOnB.getZ()));
    lineVertices_->setVertex(1,
    	Vec3f(to.getX(), to.getY(), to.getZ()));
    // Update GPU-side vertex data
    glBufferData(GL_ARRAY_BUFFER,
    		bufferSize_,
    		lineVertices_->clientData(),
    		GL_DYNAMIC_DRAW);
    // Draw the contact point as a small line
    renderState_->vao().push(vao_->id());
    lineVertices_->enableAttribute(0);
    glDrawArrays(GL_LINES, 0, 2);
    renderState_->vao().pop();
}

void BulletDebugDrawer::reportErrorWarning(const char *warningString) {
	REGEN_WARN("Bullet Warning: " << warningString);
}

void BulletDebugDrawer::draw3dText(const btVector3 &location, const char *textString)
{}

void BulletDebugDrawer::setDebugMode(int debugMode) {
	m_debugMode = debugMode;
}

int BulletDebugDrawer::getDebugMode() const {
	return m_debugMode;
}

// StateNode interface
void BulletDebugDrawer::traverse(regen::RenderState *rs) {
	renderState_ = rs;
	state()->enable(rs);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	physics_->dynamicsWorld()->debugDrawWorld();
	state()->disable(rs);
	renderState_ = nullptr;
	GL_ERROR_LOG();
}

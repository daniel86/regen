#include "uniform-block.h"

using namespace regen;

// declare the private struct
struct UniformBlock::UniformBlockData {
	ref_ptr<UBO> ubo;
};

UniformBlock::UniformBlock(const std::string &name) :
		UniformBlock(name, ref_ptr<UBO>::alloc()) {
}

UniformBlock::UniformBlock(const std::string &name, const ref_ptr<UBO> &ubo) :
		ShaderInput(name, GL_INVALID_ENUM, 0, 0, 0, GL_FALSE) {
	enableUniform_ = [this](GLint loc) { enableUniformBlock(loc); };
	isUniformBlock_ = GL_TRUE;
	isVertexAttribute_ = GL_FALSE;
	isVertexAttribute_ = GL_FALSE;
	priv_ = new UniformBlockData();
	priv_->ubo = ubo;
}

UniformBlock::~UniformBlock() {
	delete priv_;
}

void UniformBlock::enableUniformBlock(GLint loc) const {
	priv_->ubo->update();
	priv_->ubo->bindBufferBase(loc);
}

const std::vector<NamedShaderInput> &UniformBlock::uniforms() const {
	return priv_->ubo->uniforms();
}

ref_ptr<UBO> UniformBlock::ubo() const {
	return priv_->ubo;
}

void UniformBlock::addUniform(const ref_ptr<ShaderInput> &input, const std::string &name) {
	priv_->ubo->addUniform(input, name);
}

void UniformBlock::updateUniform(const ref_ptr<ShaderInput> &input) {
	priv_->ubo->updateUniform(input);
}

void UniformBlock::write(std::ostream &out) const {
	out << "uniform " << name() << " {\n";
	out << "};";
}

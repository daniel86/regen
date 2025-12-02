#include "atmosphere.h"
#include <regen/objects/primitives/rectangle.h>
#include "regen/memory/dibo.h"
#include "regen/gl/draw-command.h"
#include "regen/scene/state-configurer.h"

using namespace regen;

Atmosphere::Atmosphere(
		const ref_ptr<Sky> &sky,
		unsigned int cubeMapSize,
		bool useFloatBuffer,
		unsigned int levelOfDetail)
		: SkyLayer(sky) {
	updateMesh_ = Rectangle::getUnitQuad();
	updateMesh_->setIndirectDrawBuffer(createIndirectDrawBuffer(), 0, 5);

	state()->joinStates(ref_ptr<BlendFuncState>::alloc(
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

	ref_ptr<TextureCube> cubeMap = ref_ptr<TextureCube>::alloc(1);
	cubeMap->set_format(GL_RGBA);
	if (useFloatBuffer) {
		cubeMap->set_internalFormat(GL_RGBA16F);
	} else {
		cubeMap->set_internalFormat(GL_RGBA8);
	}
	cubeMap->set_rectangleSize(cubeMapSize, cubeMapSize);
	cubeMap->allocTexture();
	cubeMap->set_wrapping(TextureWrapping::create(GL_CLAMP_TO_EDGE));
	cubeMap->set_filter(TextureFilter::create(GL_LINEAR));

	// create render target for updating the sky cube map
	fbo_ = ref_ptr<FBO>::alloc(cubeMapSize, cubeMapSize);
	fbo_->applyDrawBuffers(DrawBuffers::attachment0());
	// clear negative y to black, -y cube face is not updated
	glNamedFramebufferTextureLayer(
		fbo_->id(),
		GL_COLOR_ATTACHMENT0,
		cubeMap->id(),
		0,
		3);
	static const float CLEAR_BLACK[4] = {0, 0, 0, 0};
	glClearNamedFramebufferfv(
		fbo_->id(),
		GL_COLOR,
		0,
		CLEAR_BLACK);
	// for updating bind all layers to GL_COLOR_ATTACHMENT0
	glNamedFramebufferTexture(
		fbo_->id(),
		GL_COLOR_ATTACHMENT0,
		cubeMap->id(),
		0);

	drawState_ = ref_ptr<SkyBox>::alloc(levelOfDetail);
	drawState_->setCubeMap(cubeMap);
	//state()->joinStates(drawState_);

	///////
	/// Update Uniforms
	///////
	rayleigh_ = ref_ptr<ShaderInput3f>::alloc("rayleigh");
	rayleigh_->setUniformData(Vec3f::zero());
	mie_ = ref_ptr<ShaderInput4f>::alloc("mie");
	mie_->setUniformData(Vec4f::zero());
	spotBrightness_ = ref_ptr<ShaderInput1f>::alloc("spotBrightness");
	spotBrightness_->setUniformData(0.0f);
	scatterStrength_ = ref_ptr<ShaderInput1f>::alloc("scatterStrength");
	scatterStrength_->setUniformData(0.0f);
	skyAbsorption_ = ref_ptr<ShaderInput3f>::alloc("skyAbsorption");
	skyAbsorption_->setUniformData(Vec3f::zero());
	///////
	/// Update State
	///////
	updateState_->shaderDefine("RENDER_LAYER", "5");
	updateState_->setInput(sky->sun()->lightUBO(), "SunLight", "_Sun");
	updateState_->setInput(mie_);
	updateState_->setInput(rayleigh_);
	updateState_->setInput(spotBrightness_);
	updateState_->setInput(skyAbsorption_);
	updateState_->setInput(scatterStrength_);
	updateShader_ = ref_ptr<ShaderState>::alloc();
	updateState_->joinStates(updateShader_);
	updateState_->setInput(sky_->worldTime()->in);
}

void Atmosphere::createUpdateShader() {
	StateConfig shaderConfig = StateConfigurer::configure(updateState_.get());
	updateShader_->createShader(shaderConfig, "regen.weather.atmosphere");
	updateMesh_->updateVAO(shaderConfig, updateShader_->shader());
}

void Atmosphere::setRayleighBrightness(float v) {
	auto v_rayleigh = rayleigh_->mapClientVertex<Vec3f>(BUFFER_GPU_READ | BUFFER_GPU_WRITE, 0);
	v_rayleigh.w = Vec3f(v / 10.0f, v_rayleigh.r.y, v_rayleigh.r.z);
}

void Atmosphere::setRayleighStrength(float v) {
	auto v_rayleigh = rayleigh_->mapClientVertex<Vec3f>(BUFFER_GPU_READ | BUFFER_GPU_WRITE, 0);
	v_rayleigh.w = Vec3f(v_rayleigh.r.x, v / 1000.0f, v_rayleigh.r.z);
}

void Atmosphere::setRayleighCollect(float v) {
	auto v_rayleigh = rayleigh_->mapClientVertex<Vec3f>(BUFFER_GPU_READ | BUFFER_GPU_WRITE, 0);
	v_rayleigh.w = Vec3f(v_rayleigh.r.x, v_rayleigh.r.y, v / 100.0f);
}

void Atmosphere::setMieBrightness(float v) {
	auto v_mie = mie_->mapClientVertex<Vec4f>(BUFFER_GPU_READ | BUFFER_GPU_WRITE, 0);
	v_mie.w = Vec4f(v / 1000.0f, v_mie.r.y, v_mie.r.z, v_mie.r.w);
}

void Atmosphere::setMieStrength(float v) {
	auto v_mie = mie_->mapClientVertex<Vec4f>(BUFFER_GPU_READ | BUFFER_GPU_WRITE, 0);
	v_mie.w = Vec4f(v_mie.r.x, v / 10000.0f, v_mie.r.z, v_mie.r.w);
}

void Atmosphere::setMieCollect(float v) {
	auto v_mie = mie_->mapClientVertex<Vec4f>(BUFFER_GPU_READ | BUFFER_GPU_WRITE, 0);
	v_mie.w = Vec4f(v_mie.r.x, v_mie.r.y, v / 100.0f, v_mie.r.w);
}

void Atmosphere::setMieDistribution(float v) {
	auto v_mie = mie_->mapClientVertex<Vec4f>(BUFFER_GPU_READ | BUFFER_GPU_WRITE, 0);
	v_mie.w = Vec4f(v_mie.r.x, v_mie.r.y, v_mie.r.z, v / 100.0f);
}

void Atmosphere::setSpotBrightness(float v) {
	spotBrightness_->setVertex(0, v);
}

void Atmosphere::setScatterStrength(float v) {
	scatterStrength_->setVertex(0, v / 1000.0f);
}

void Atmosphere::setAbsorption(const Vec3f &color) {
	skyAbsorption_->setVertex(0, color);
}

void Atmosphere::setEarth() {
	AtmosphereProperties prop;
	prop.rayleigh = Vec3f(19.0, 359.0, 81.0);
	prop.mie = Vec4f(44.0, 308.0, 39.0, 74.0);
	prop.spot = 8.0;
	prop.scatterStrength = 54.0;
	prop.absorption = Vec3f(
			0.18867780436772762,
			0.4978442963618773,
			0.6616065586417131);
	setProperties(prop);
}

void Atmosphere::setMars() {
	AtmosphereProperties prop;
	prop.rayleigh = Vec3f(33.0, 139.0, 81.0);
	prop.mie = Vec4f(100.0, 264.0, 39.0, 63.0);
	prop.spot = 1000.0;
	prop.scatterStrength = 28.0;
	prop.absorption = Vec3f(0.66015625, 0.5078125, 0.1953125);
	setProperties(prop);
}

void Atmosphere::setUranus() {
	AtmosphereProperties prop;
	prop.rayleigh = Vec3f(80.0, 136.0, 71.0);
	prop.mie = Vec4f(67.0, 68.0, 0.0, 56.0);
	prop.spot = 0.0;
	prop.scatterStrength = 18.0;
	prop.absorption = Vec3f(0.26953125, 0.5234375, 0.8867187);
	setProperties(prop);
}

void Atmosphere::setVenus() {
	AtmosphereProperties prop;
	prop.rayleigh = Vec3f(25.0, 397.0, 34.0);
	prop.mie = Vec4f(124.0, 298.0, 76.0, 81.0);
	prop.spot = 0.0;
	prop.scatterStrength = 140.0;
	prop.absorption = Vec3f(0.6640625, 0.5703125, 0.29296875);
	setProperties(prop);
}

void Atmosphere::setAlien() {
	AtmosphereProperties prop;
	prop.rayleigh = Vec3f(44.0, 169.0, 71.0);
	prop.mie = Vec4f(60.0, 139.0, 46.0, 86.0);
	prop.spot = 0.0;
	prop.scatterStrength = 26.0;
	prop.absorption = Vec3f(0.24609375, 0.53125, 0.3515625);
	setProperties(prop);
}

void Atmosphere::setProperties(AtmosphereProperties &p) {
	setRayleighBrightness(p.rayleigh.x);
	setRayleighStrength(p.rayleigh.y);
	setRayleighCollect(p.rayleigh.z);
	setMieBrightness(p.mie.x);
	setMieStrength(p.mie.y);
	setMieCollect(p.mie.z);
	setMieDistribution(p.mie.w);
	setSpotBrightness(p.spot);
	setScatterStrength(p.scatterStrength);
	setAbsorption(p.absorption);
}

const ref_ptr<TextureCube> &Atmosphere::cubeMap() const {
	return drawState_->cubeMap();
}

void Atmosphere::updateSkyLayer(RenderState *rs, double dt) {
	rs->drawFrameBuffer().apply(fbo_->id());
	rs->viewport().apply(fbo_->glViewport());
	updateState_->enable(rs);
	updateMesh_->draw(rs);
	updateState_->disable(rs);
}

ref_ptr<SSBO> Atmosphere::createIndirectDrawBuffer() {
	constexpr uint32_t numLayer = 5;
	constexpr uint32_t numLODs = 1;
	auto &lodData = updateMesh_->meshLODs()[0];

	// One draw command per cube map face (except bottom face).
	std::array<DrawCommand,5> drawCommands;
	DrawCommand &drawParams = drawCommands[0];
	drawParams.mode = 1u; // 1=elements, 2=arrays
	drawParams.setCount(lodData.d->numIndices);
	drawParams.setFirstElement(lodData.d->indexOffset / updateMesh_->indices()->dataTypeBytes());
	drawParams.data[3] = 0; // base vertex
	drawParams.setInstanceCount(1);
	drawParams.setBaseInstance(0);
	// Copy over the data for the remaining layers.
	// Actually, we currently use the same mesh for each face, so the draw commands
	// are identical, however the vertex shader selects render layer based on gl_DrawID,
	// and maps the vertex positions accordingly.
	for (uint32_t layerIdx = 1; layerIdx < numLayer; ++layerIdx) {
		const uint32_t lodLayerIdx = layerIdx;;
		std::memcpy(&drawCommands[lodLayerIdx], &drawCommands[0], sizeof(DrawCommand));
	}

	indirectDrawBuffer_ = ref_ptr<DrawIndirectBuffer>::alloc(
			"IndirectDrawBuffer", BufferUpdateFlags::NEVER);
	auto input = ref_ptr<ShaderInputStruct<DrawCommand>>::alloc(
			"DrawCommand", "drawParams", numLODs * numLayer);
	input->setInstanceData(1, 1, (byte*)drawCommands.data());
	indirectDrawBuffer_->addStagedInput(input);
	indirectDrawBuffer_->update();
	//indirectDrawBuffer_->setBufferData((byte*)initialData.data());
	return indirectDrawBuffer_;
}

#include <regen/utility/strings.h>
#include <regen/gl/gl-util.h>
#include <regen/gl/gl-enum.h>

#include "mesh-animation.h"

#include "regen/compute/compute-pass.h"

using namespace regen;

void MeshAnimation::findFrameAfterTick(
		double tick,
		int &frame,
		std::vector<KeyFrame> &keys) {
	while (frame < (int) (keys.size() - 1)) {
		if (tick <= keys[frame].endTick) {
			return;
		}
		frame += 1;
	}
}

void MeshAnimation::findFrameBeforeTick(
		double &tick,
		uint32_t &frame,
		std::vector<KeyFrame> &keys) {
	for (frame = keys.size() - 1; frame > 0;) {
		if (tick >= keys[frame].startTick) {
			return;
		}
		frame -= 1;
	}
}

MeshAnimation::MeshAnimation(
		const ref_ptr<Mesh> &mesh,
		const std::list<Interpolation> &interpolations)
		: Animation(true, false),
		  mesh_(mesh),
		  lastFrame_(-1),
		  nextFrame_(-1),
		  pingFrame_(-1),
		  pongFrame_(-1),
		  elapsedTime_(0.0),
		  ticksPerSecond_(1.0),
		  lastTime_(0.0),
		  tickRange_(0.0, 0.0),
		  lastFramePosition_(0u),
		  startFramePosition_(0u) {
	const uint32_t numVertices = mesh->numVertices();
	StateConfigurer shaderConfigurer;

	for (auto jt = interpolations.begin(); jt != interpolations.end(); ++jt) {
		animAttributes_.insert(jt->attributeName);
	}

	// used to save two frames
	pingBuffer_ = ref_ptr<SSBO>::alloc("PingFrame",
		BufferUpdateFlags::FULL_RARELY | BUFFER_COMPUTABLE);
	pongBuffer_ = ref_ptr<SSBO>::alloc("PongFrame",
		BufferUpdateFlags::FULL_RARELY | BUFFER_COMPUTABLE);

	// find buffer size
	bufferSize_ = 0u;
	uint32_t i = 0u;
	for (auto it = mesh->inputs().begin(); it != mesh->inputs().end(); ++it) {
		const ref_ptr<ShaderInput> &in = it->in_;
		if (!in->isVertexAttribute()) continue;
		if (!animAttributes_.contains(in->name())) continue;

		bufferSize_ += in->alignedInputSize();

		auto pingAtt = ShaderInput::copy(in, false);
		pingAtt->setVertexData(numVertices, nullptr);
		pingBuffer_->addStagedInput(pingAtt);

		auto pongAtt = ShaderInput::copy(in, false);
		pongAtt->setVertexData(numVertices, nullptr);
		pongBuffer_->addStagedInput(pongAtt);

		std::string interpolationName = "interpolate_linear";
		std::string interpolationKey;
		for (auto jt = interpolations.begin(); jt != interpolations.end(); ++jt) {
			if (jt->attributeName == in->name()) {
				interpolationName = jt->interpolationName;
				interpolationKey = jt->interpolationKey;
				break;
			}
		}

		shaderConfigurer.define(REGEN_STRING("ATTRIBUTE" << i << "_INTERPOLATION_NAME"), interpolationName);
		if (!interpolationKey.empty()) {
			shaderConfigurer.define(REGEN_STRING("ATTRIBUTE" << i << "_INTERPOLATION_KEY"), interpolationKey);
		}
		shaderConfigurer.define(REGEN_STRING("ATTRIBUTE" << i << "_NAME"), in->name());
		shaderConfigurer.define(REGEN_STRING("ATTRIBUTE" << i << "_TYPE"),
				glenum::glslDataType(in->baseType(), in->valsPerElement()));

		i += 1;
	}
	shaderConfigurer.define("NUM_ATTRIBUTES", REGEN_STRING(i));

	pingBuffer_->update();
	pongBuffer_->update();

	// create initial frame
	addMeshFrame(0.0);

	interpolationState_ = ref_ptr<State>::alloc();
	// join shader uniforms into animation state such that they can be
	// configured from the outside
	meshAnimState_ = ref_ptr<State>::alloc();

	auto meshBuffer = ref_ptr<SSBO>::alloc(*mesh_->vertexBuffer().get(), "VertexData");
	interpolationState_->setInput(meshBuffer);

	const std::string updateShaderKey = "regen.animation.morph.interpolate";;
	auto cs = ref_ptr<ComputePass>::alloc(updateShaderKey);
	cs->computeState()->setNumWorkUnits(numVertices, 1, 1);
	cs->computeState()->setGroupSize(256, 1, 1);
	interpolationState_->joinStates(cs);

	shaderConfigurer.define("NUM_VERTICES", REGEN_STRING(numVertices));
	shaderConfigurer.addState(animationState_.get());
	shaderConfigurer.addState(interpolationState_.get());
	cs->createShader(shaderConfigurer.cfg());

	auto csShader = cs->shaderState()->shader();
	ref_ptr<ShaderInput> in = csShader->createUniform("frameTimeNormalized");
	frameTimeUniform_ = (ShaderInput1f *) in.get();
	frameTimeUniform_->setUniformData(0.0f);
	csShader->setInput(in);

	in = csShader->createUniform("friction");
	frictionUniform_ = (ShaderInput1f *) in.get();
	frictionUniform_->setUniformData(8.0f);
	csShader->setInput(in);
	meshAnimState_->setInput(in, in->name());

	in = csShader->createUniform("frequency");
	frequencyUniform_ = (ShaderInput1f *) in.get();
	frequencyUniform_->setUniformData(5.0f);
	csShader->setInput(in);
	meshAnimState_->setInput(in, in->name());

	lastFrameBindingPoint_ = csShader->uniformLocation("LastFrame");
	nextFrameBindingPoint_ = csShader->uniformLocation("NextFrame");

	joinAnimationState(meshAnimState_);
}

void MeshAnimation::setFriction(float friction) {
	frictionUniform_->setUniformData(friction);
}

void MeshAnimation::setFrequency(float frequency) {
	frequencyUniform_->setUniformData(frequency);
}

void MeshAnimation::setTickRange(const Vec2d &forcedTickRange) {
	// get first and last tick of animation
	if (forcedTickRange.x < 0.0f || forcedTickRange.y < 0.0f) {
		tickRange_.x = 0.0;
		tickRange_.y = 0.0;
		for (auto & frame : frames_) {
			tickRange_.y += frame.timeInTicks;
		}
	} else {
		tickRange_ = forcedTickRange;
	}

	// set start frames
	if (tickRange_.x < 0.00001) {
		startFramePosition_ = 0u;
	} else {
		uint32_t framePos;
		findFrameBeforeTick(tickRange_.x, framePos, frames_);
		startFramePosition_ = framePos;
	}
	// initial last frame to start frame
	lastFramePosition_ = startFramePosition_;

	// set to start pos of the new tick range
	lastTime_ = 0.0;
	elapsedTime_ = 0.0;
}

void MeshAnimation::loadFrame(uint32_t frameIndex, bool isPongFrame) {
	KeyFrame &frame = frames_[frameIndex];

	auto &dataBuffer = (isPongFrame ? pongBuffer_ : pingBuffer_);
	auto m_staged = dataBuffer->mapClientDataRaw(BUFFER_GPU_WRITE);
	byte* d_staged = m_staged.w;

	uint32_t offset = 0u;
	for (auto & attribute : frame.attributes) {
		const ref_ptr<ShaderInput> &input = attribute.input;
		const uint32_t numBytes = input->alignedInputSize();
		const auto m_frame = input->mapClientDataRaw(BUFFER_GPU_READ);
		// align to base alignment
		offset = (offset + input->baseAlignment() - 1) & ~(input->baseAlignment() - 1);
		std::memcpy(
			d_staged + offset,
			m_frame.r, numBytes);
		offset += numBytes;
	}

	if (isPongFrame) {
		pongFrame_ = frameIndex;
		frame.buffer = pongBuffer_;
	} else {
		pingFrame_ = frameIndex;
		frame.buffer = pingBuffer_;
	}
}

void MeshAnimation::gpuUpdate(RenderState *rs, double dt) {
	if (dt <= 0.00001) return;

	elapsedTime_ += dt;

	// map into anim's duration
	const double duration = tickRange_.y - tickRange_.x;
	const double timeInTicks = elapsedTime_ * ticksPerSecond_ / 1000.0;
	if (timeInTicks > duration) {
		REGEN_DEBUG("Mesh animation stopped at tick " <<
				timeInTicks << " (duration: " << duration << ", frame:" << nextFrame_ << ")");
		elapsedTime_ = 0.0;
		lastTime_ = 0.0;
		tickRange_.x = 0.0;
		tickRange_.y = 0.0;
		stopAnimation();
		return;
	}

	// Look for present frame number.
	int lastFrame = lastFramePosition_;
	int frame = (timeInTicks >= lastTime_ ? lastFrame : startFramePosition_);
	findFrameAfterTick(timeInTicks, frame, frames_);
	lastFramePosition_ = frame;

	// keep two frames in animation buffer
	lastFrame = frame - 1;

	KeyFrame &frame0 = frames_[lastFrame];
	if (lastFrame != pingFrame_ && lastFrame != pongFrame_) {
		loadFrame(lastFrame, frame == pingFrame_);
	}
	if (lastFrame != lastFrame_) {
		lastFrame_ = lastFrame;
	}

	KeyFrame &frame1 = frames_[frame];
	if (frame != pingFrame_ && frame != pongFrame_) {
		loadFrame(frame, lastFrame == pingFrame_);
	}
	if (frame != nextFrame_) {
		nextFrame_ = frame;
		REGEN_DEBUG("Next frame: " << nextFrame_ << " (time: " << timeInTicks << ")");
	}

	frameTimeUniform_->setVertex(0,
		(timeInTicks - frame1.startTick) / frame1.timeInTicks);

	// bind frame data buffers
	frame0.buffer->bind(lastFrameBindingPoint_);
	frame1.buffer->bind(nextFrameBindingPoint_);

	// Compute next vertex data
	interpolationState_->enable(rs);
	interpolationState_->disable(rs);

	lastTime_ = tickRange_.x + timeInTicks;
}

////////

void MeshAnimation::addFrame(
		const std::list<ref_ptr<ShaderInput> > &attributes,
		double timeInTicks) {
	MeshAnimation::KeyFrame frame;

	frame.timeInTicks = timeInTicks;
	frame.startTick = 0.0;
	for (auto it = frames_.rbegin(); it != frames_.rend(); ++it) {
		MeshAnimation::KeyFrame &parentFrame = *it;
		frame.startTick += parentFrame.timeInTicks;
	}
	frame.endTick = frame.startTick + frame.timeInTicks;

	REGEN_DEBUG("Adding frame at tick " << frame.startTick << " (duration: " << frame.timeInTicks << ")");

	// add attributes
	for (const auto & input : mesh_->inputs()) {
		const ref_ptr<ShaderInput> &in0 = input.in_;
		if (!in0->isVertexAttribute()) continue;
		if (!animAttributes_.contains(in0->name())) continue;
		ref_ptr<ShaderInput> att;

		// find specified attribute
		for (const auto & in1 : attributes) {
				if (in0->name() == in1->name()) {
				att = in1;
				break;
			}
		}
		if (att.get() == nullptr) {
			// find attribute from previous frames
			att = findLastAttribute(in0->name());
		}
		if (att.get() != nullptr) {
			frame.attributes.emplace_back(att, -1);
		} else {
			REGEN_WARN("No attribute found for '" << in0->name() << "'");
		}
	}

	frames_.push_back(frame);
}

void MeshAnimation::addMeshFrame(double timeInTicks) {
	std::list<ref_ptr<ShaderInput> > meshAttributes;
	for (auto &it : mesh_->inputs()) {
		if (!it.in_->isVertexAttribute()) continue;
		if (!animAttributes_.contains(it.in_->name())) continue;
		meshAttributes.push_back(ShaderInput::copy(it.in_, true));
	}
	addFrame(meshAttributes, timeInTicks);
}

ref_ptr<ShaderInput> MeshAnimation::findLastAttribute(const std::string &name) {
	for (auto it = frames_.rbegin(); it != frames_.rend(); ++it) {
		MeshAnimation::KeyFrame &f = *it;
		for (auto jt = f.attributes.begin(); jt != f.attributes.end(); ++jt) {
			const ref_ptr<ShaderInput> &att = jt->input;
			if (att->name() == name) {
				return ShaderInput::copy(att, true);
			}
		}
	}
	return {};
}

void MeshAnimation::addSphereAttributes(
		float horizontalRadius,
		float verticalRadius,
		double timeInTicks,
		const Vec3f &offset) {
	if (!mesh_->hasInput(ATTRIBUTE_NAME_POS)) {
		REGEN_WARN("mesh has no input named '" << ATTRIBUTE_NAME_POS << "'");
		return;
	}
	if (!mesh_->hasInput(ATTRIBUTE_NAME_NOR)) {
		REGEN_WARN("mesh has no input named '" << ATTRIBUTE_NAME_NOR << "'");
		return;
	}
	//float radiusScale = horizontalRadius / verticalRadius;
	//Vec3f scale(radiusScale, 1.0, radiusScale);

	ref_ptr<ShaderInput3f> posAtt = ref_ptr<ShaderInput3f>::dynamicCast(mesh_->positions());
	ref_ptr<ShaderInput3f> norAtt = ref_ptr<ShaderInput3f>::dynamicCast(mesh_->normals());
	// allocate memory for the animation attributes
	ref_ptr<ShaderInput3f> spherePos = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	ref_ptr<ShaderInput3f> sphereNor = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	spherePos->setMemoryLayout(BUFFER_MEMORY_STD430);
	sphereNor->setMemoryLayout(BUFFER_MEMORY_STD430);
	spherePos->setVertexData(posAtt->numVertices(), nullptr);
	sphereNor->setVertexData(norAtt->numVertices(), nullptr);

	// find the centroid of the mesh
	Vec3f minPos = posAtt->getVertex(0).r;
	Vec3f maxPos = posAtt->getVertex(0).r;
	for (uint32_t i = 1; i < posAtt->numVertices(); ++i) {
		auto v = posAtt->getVertex(i);
		if(minPos.x > v.r.x) minPos.x = v.r.x;
		if(minPos.y > v.r.y) minPos.y = v.r.y;
		if(minPos.z > v.r.z) minPos.z = v.r.z;
		if(maxPos.x < v.r.x) maxPos.x = v.r.x;
		if(maxPos.y < v.r.y) maxPos.y = v.r.y;
		if(maxPos.z < v.r.z) maxPos.z = v.r.z;
	}
	Vec3f centroid = (minPos + maxPos) * 0.5f + offset;

	// set sphere vertex data
	// Note: this is a very simple sphere mapping, it is not perfect.
	//       There might be artifacts caused by faces that are flipped.
	//       Also make sure to use polygon offset to avoid shadow map fighting.
	// NOTE: It should be possible to do this with fewer artifacts by taking the faces into account.
	for (uint32_t i = 0; i < spherePos->numVertices(); ++i) {
		Vec3f v = posAtt->getVertex(i).r;
		Vec3f direction = v - centroid;
		Vec3f n;
		double l = direction.length();
		if (l == 0) {
			continue;
		}

		// take normalized direction vector as normal
		n = direction;
		n.normalize();
		// and scaled normal as sphere position
		// 1e-1 to avoid fighting
		v = centroid + n * verticalRadius * (1.0f + l * 1e-6);

		spherePos->setVertex(i, v);
		sphereNor->setVertex(i, n);
	}

	std::list<ref_ptr<ShaderInput> > attributes;
	attributes.emplace_back(spherePos);
	attributes.emplace_back(sphereNor);
	addFrame(attributes, timeInTicks);
}

#if 0
static void cubizePoint(Vec3f& position)
{
  double x,y,z;
  x = position.x;
  y = position.y;
  z = position.z;

  double fx, fy, fz;
  fx = fabsf(x);
  fy = fabsf(y);
  fz = fabsf(z);

  const double inverseSqrt2 = 0.70710676908493042;

  if (fy >= fx && fy >= fz) {
	double a2 = x * x * 2.0;
	double b2 = z * z * 2.0;
	double inner = -a2 + b2 -3;
	double innersqrt = -sqrtf((inner * inner) - 12.0 * a2);

	if(x == 0.0 || x == -0.0) {
	  position.x = 0.0;
	} else {
	  position.x = sqrtf(innersqrt + a2 - b2 + 3.0) * inverseSqrt2;
	}

	if(z == 0.0 || z == -0.0) {
	  position.z = 0.0;
	} else {
	  position.z = sqrtf(innersqrt - a2 + b2 + 3.0) * inverseSqrt2;
	}

	if(position.x > 1.0) position.x = 1.0;
	if(position.z > 1.0) position.z = 1.0;

	if(x < 0) position.x = -position.x;
	if(z < 0) position.z = -position.z;

	if (y > 0) {
	  // top face
	  position.y = 1.0;
	} else {
	  // bottom face
	  position.y = -1.0;
	}
  }
  else if (fx >= fy && fx >= fz) {
	double a2 = y * y * 2.0;
	double b2 = z * z * 2.0;
	double inner = -a2 + b2 -3;
	double innersqrt = -sqrtf((inner * inner) - 12.0 * a2);

	if(y == 0.0 || y == -0.0) {
	  position.y = 0.0;
	} else {
	  position.y = sqrtf(innersqrt + a2 - b2 + 3.0) * inverseSqrt2;
	}

	if(z == 0.0 || z == -0.0) {
	  position.z = 0.0;
	} else {
	  position.z = sqrtf(innersqrt - a2 + b2 + 3.0) * inverseSqrt2;
	}

	if(position.y > 1.0) position.y = 1.0;
	if(position.z > 1.0) position.z = 1.0;

	if(y < 0) position.y = -position.y;
	if(z < 0) position.z = -position.z;

	if (x > 0) {
	  // right face
	  position.x = 1.0;
	} else {
	  // left face
	  position.x = -1.0;
	}
  }
  else {
	double a2 = x * x * 2.0;
	double b2 = y * y * 2.0;
	double inner = -a2 + b2 -3;
	double innersqrt = -sqrtf((inner * inner) - 12.0 * a2);

	if(x == 0.0 || x == -0.0) {
	  position.x = 0.0;
	} else {
	  position.x = sqrtf(innersqrt + a2 - b2 + 3.0) * inverseSqrt2;
	}

	if(y == 0.0 || y == -0.0) {
	  position.y = 0.0;
	} else {
	  position.y = sqrtf(innersqrt - a2 + b2 + 3.0) * inverseSqrt2;
	}

	if(position.x > 1.0) position.x = 1.0;
	if(position.y > 1.0) position.y = 1.0;

	if(x < 0) position.x = -position.x;
	if(y < 0) position.y = -position.y;

	if (z > 0) {
	  // front face
	  position.z = 1.0;
	} else {
	  // back face
	  position.z = -1.0;
	}
  }
}
#endif

void MeshAnimation::addBoxAttributes(
		float width,
		float height,
		float depth,
		double timeInTicks,
		const Vec3f &offset) {
	if (!mesh_->hasInput(ATTRIBUTE_NAME_POS)) {
		REGEN_WARN("mesh has no input named '" << ATTRIBUTE_NAME_POS << "'");
		return;
	}
	if (!mesh_->hasInput(ATTRIBUTE_NAME_NOR)) {
		REGEN_WARN("mesh has no input named '" << ATTRIBUTE_NAME_NOR << "'");
		return;
	}

	Vec3f boxSize(width, height, depth);
	double radius = sqrt(0.5f);

	ref_ptr<ShaderInput3f> posAtt = ref_ptr<ShaderInput3f>::dynamicCast(mesh_->positions());
	ref_ptr<ShaderInput3f> norAtt = ref_ptr<ShaderInput3f>::dynamicCast(mesh_->normals());
	// allocate memory for the animation attributes
	ref_ptr<ShaderInput3f> boxPos = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	ref_ptr<ShaderInput3f> boxNor = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	boxPos->setMemoryLayout(BUFFER_MEMORY_STD430);
	boxNor->setMemoryLayout(BUFFER_MEMORY_STD430);
	boxPos->setVertexData(posAtt->numVertices(), nullptr);
	boxNor->setVertexData(norAtt->numVertices(), nullptr);

	// set cube vertex data
	for (uint32_t i = 0; i < boxPos->numVertices(); ++i) {
		Vec3f v = posAtt->getVertex(i).r;
		Vec3f n;
		double l = v.length();
		if (l == 0) {
			continue;
		}

		// first map to sphere, a bit ugly but avoids intersection calculations
		// and scaled normal as sphere position
		Vec3f vCopy = v;
		vCopy.normalize();

#if 0
		// check the coordinate values to choose the right face
		double xAbs = abs(vCopy.x);
		double yAbs = abs(vCopy.y);
		double zAbs = abs(vCopy.z);
		double factor;
		// set the coordinate for the face to the cube size
		if(xAbs > yAbs && xAbs > zAbs) { // left/right face
			factor = (v.x<0 ? -1 : 1);
			n = (Vec3f(1,0,0))*factor;
		} else if(yAbs > zAbs) { // top/bottom face
			factor = (v.y<0 ? -1 : 1);
			n = (Vec3f(0,1,0))*factor;
		} else { //front/back face
			factor = (v.z<0 ? -1 : 1);
			n = (Vec3f(0,0,1))*factor;
		}

		cubizePoint(vCopy);
		v = vCopy * boxSize * 0.5f;
#else
		vCopy *= radius;

		// check the coordinate values to choose the right face
		double xAbs = abs(vCopy.x);
		double yAbs = abs(vCopy.y);
		double zAbs = abs(vCopy.z);
		float h, factor;
		// set the coordinate for the face to the cube size
		if (xAbs > yAbs && xAbs > zAbs) { // left/right face
			factor = (v.x < 0.0f ? -1.0f : 1.0f);
			n = (Vec3f(1, 0, 0)) * factor;
			h = vCopy.x;
		} else if (yAbs > zAbs) { // top/bottom face
			factor = (v.y < 0.0f ? -1.0f : 1.0f);
			n = (Vec3f(0, 1, 0)) * factor;
			h = vCopy.y;
		} else { //front/back face
			factor = (v.z < 0.0f ? -1.0f : 1.0f);
			n = (Vec3f(0, 0, 1)) * factor;
			h = vCopy.z;
		}

		Vec3f r = vCopy - n * n.dot(vCopy) * 2.0f;
		// reflect vector on cube face plane (-r*(factor*0.5f-h)/h) and
		// delete component of face direction (-n*0.5f , 0.5f because thats the sphere radius)
		vCopy += -r * (factor * 0.5f - h) / h - n * 0.5f;

		double maxDim = std::max(std::max(abs(vCopy.x), abs(vCopy.y)), abs(vCopy.z));
		// we divide by maxDim, so it is not allowed to be zero,
		// this happens for vCopy with only a single component not zero.
		if (maxDim != 0.0f) {
			// the distortion scale is calculated by dividing
			// the length of the vector pointing on the square surface
			// by the length of the vector pointing on the circle surface (equals circle radius).
			// size2/maxDim calculates scale factor for d to point on the square surface
			double distortionScale = ((vCopy * 0.5f / maxDim).length()) / 0.5f;
			vCopy *= distortionScale;
		}

		// -l*1e-6 to avoid fighting
		v = offset + (vCopy + n * 0.5f) * boxSize * (1.0f + l * 1e-4);
#endif

		boxPos->setVertex(i, v);
		boxNor->setVertex(i, n);
	}

	std::list<ref_ptr<ShaderInput> > attributes;
	attributes.emplace_back(boxPos);
	attributes.emplace_back(boxNor);
	addFrame(attributes, timeInTicks);
}

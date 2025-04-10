/*
 * mesh-animation-gpu.cpp
 *
 *  Created on: 29.10.2012
 *      Author: daniel
 */

#include <limits.h>
#include <regen/utility/string-util.h>
#include <regen/gl-types/gl-util.h>
#include <regen/gl-types/gl-enum.h>

#include "mesh-animation.h"

using namespace regen;

void MeshAnimation::findFrameAfterTick(
		GLdouble tick,
		GLint &frame,
		std::vector<KeyFrame> &keys) {
	while (frame < (GLint) (keys.size() - 1)) {
		if (tick <= keys[frame].endTick) {
			return;
		}
		frame += 1;
	}
}

void MeshAnimation::findFrameBeforeTick(
		GLdouble &tick,
		GLuint &frame,
		std::vector<KeyFrame> &keys) {
	for (frame = keys.size() - 1; frame > 0;) {
		if (tick >= keys[frame].startTick) {
			return;
		}
		frame -= 1;
	}
}

// TODO: client data mapping can be improved in this file

MeshAnimation::MeshAnimation(
		const ref_ptr<Mesh> &mesh,
		const std::list<Interpolation> &interpolations)
		: Animation(true, false),
		  mesh_(mesh),
		  meshBufferOffset_(-1),
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
	const ref_ptr<InputContainer> &inputContainer = mesh->inputContainer();
	const ShaderInputList &inputs = inputContainer->inputs();
	std::map<GLenum, std::string> shaderNames;
	std::map<std::string, std::string> shaderConfig;
	std::map<std::string, std::string> functions;
	std::list<std::string> transformFeedback;

	shaderNames[GL_VERTEX_SHADER] = "regen.animation.morph.interpolate";

	// find buffer size
	bufferSize_ = 0u;
	GLuint i = 0u;
	for (auto it = inputs.begin(); it != inputs.end(); ++it) {
		const ref_ptr<ShaderInput> &in = it->in_;
		if (!in->isVertexAttribute()) continue;
		bufferSize_ += in->inputSize();
		transformFeedback.push_back(in->name());

		if(in->stride()==0) {
			hasMeshInterleavedAttributes_ = GL_FALSE;
		} else {
			hasMeshInterleavedAttributes_ = GL_TRUE;
		}

		std::string interpolationName = "interpolate_linear";
		std::string interpolationKey;
		for (auto jt = interpolations.begin(); jt != interpolations.end(); ++jt) {
			if (jt->attributeName == in->name()) {
				interpolationName = jt->interpolationName;
				interpolationKey = jt->interpolationKey;
				break;
			}
		}

		shaderConfig[REGEN_STRING("ATTRIBUTE" << i << "_INTERPOLATION_NAME")] = interpolationName;
		if (!interpolationKey.empty()) {
			shaderConfig[REGEN_STRING("ATTRIBUTE" << i << "_INTERPOLATION_KEY")] = interpolationKey;
		}
		shaderConfig[REGEN_STRING("ATTRIBUTE" << i << "_NAME")] = in->name();
		shaderConfig[REGEN_STRING("ATTRIBUTE" << i << "_TYPE")] =
				glenum::glslDataType(in->baseType(), in->valsPerElement());
		i += 1;
	}
	shaderConfig["NUM_ATTRIBUTES"] = REGEN_STRING(i);

	// used to save two frames
	animationBuffer_ = ref_ptr<VBO>::alloc(ARRAY_BUFFER, USAGE_STREAM);
	feedbackBuffer_ = ref_ptr<VBO>::alloc(TRANSFORM_FEEDBACK_BUFFER, USAGE_STREAM);
	feedbackRef_ = feedbackBuffer_->allocBytes(bufferSize_);
	if (!feedbackRef_.get()) {
		REGEN_WARN("Unable to allocate VBO for animation. Animation will not work.");
		return;
	}

	bufferRange_.buffer_ = feedbackRef_->bufferID();
	bufferRange_.offset_ = 0;
	bufferRange_.size_ = bufferSize_;

	// create initial frame
	addMeshFrame(0.0);

	// init interpolation shader
	{
		std::map<GLenum, std::string> preProcessed;
		Shader::preProcess(preProcessed,
						   PreProcessorConfig(330, shaderNames, shaderConfig));
		interpolationShader_ = ref_ptr<Shader>::alloc(preProcessed);
	}
	if (hasMeshInterleavedAttributes_) {
		interpolationShader_->setTransformFeedback(
				transformFeedback, GL_INTERLEAVED_ATTRIBS, GL_VERTEX_SHADER);
	} else {
		interpolationShader_->setTransformFeedback(
				transformFeedback, GL_SEPARATE_ATTRIBS, GL_VERTEX_SHADER);
	}
	if (interpolationShader_.get() != nullptr &&
		interpolationShader_->compile() && interpolationShader_->link()) {
		ref_ptr<ShaderInput> in = interpolationShader_->createUniform("frameTimeNormalized");
		frameTimeUniform_ = (ShaderInput1f *) in.get();
		frameTimeUniform_->setUniformData(0.0f);
		interpolationShader_->setInput(in);
		// join shader uniforms into animation state such that they can be
		// configured from the outside
		meshAnimState_ = ref_ptr<State>::alloc();

		in = interpolationShader_->createUniform("friction");
		frictionUniform_ = (ShaderInput1f *) in.get();
		frictionUniform_->setUniformData(8.0f);
		interpolationShader_->setInput(in);
		meshAnimState_->joinShaderInput(in, in->name());

		in = interpolationShader_->createUniform("frequency");
		frequencyUniform_ = (ShaderInput1f *) in.get();
		frequencyUniform_->setUniformData(5.0f);
		interpolationShader_->setInput(in);
		meshAnimState_->joinShaderInput(in, in->name());

		joinAnimationState(meshAnimState_);
	} else {
		interpolationShader_ = ref_ptr<Shader>();
	}
}

void MeshAnimation::setFriction(GLfloat friction) {
	frictionUniform_->setUniformData(friction);
}

void MeshAnimation::setFrequency(GLfloat frequency) {
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
		GLuint framePos;
		findFrameBeforeTick(tickRange_.x, framePos, frames_);
		startFramePosition_ = framePos;
	}
	// initial last frame to start frame
	lastFramePosition_ = startFramePosition_;

	// set to start pos of the new tick range
	lastTime_ = 0.0;
	elapsedTime_ = 0.0;
}

void MeshAnimation::loadFrame(GLuint frameIndex, GLboolean isPongFrame) {
	MeshAnimation::KeyFrame &frame = frames_[frameIndex];

	std::list<ref_ptr<ShaderInput> > atts;
	for (auto & attribute : frame.attributes) {
		atts.push_back(attribute.input);
	}

	if (isPongFrame) {
		if (pongFrame_ != -1) { BufferObject::free(pongIt_.get()); }
		pongFrame_ = frameIndex;
		if (hasMeshInterleavedAttributes_) {
			pongIt_ = animationBuffer_->allocInterleaved(atts);
		} else {
			pongIt_ = animationBuffer_->allocSequential(atts);
		}
		frame.ref = pongIt_;
	} else {
		if (pingFrame_ != -1) { BufferObject::free(pingIt_.get()); }
		pingFrame_ = frameIndex;
		if (hasMeshInterleavedAttributes_) {
			pingIt_ = animationBuffer_->allocInterleaved(atts);
		} else {
			pingIt_ = animationBuffer_->allocSequential(atts);
		}
		frame.ref = pingIt_;
	}
}

void MeshAnimation::glAnimate(RenderState *rs, GLdouble dt) {
	if (dt <= 0.00001) return;
	if (rs->isTransformFeedbackAcive()) {
		REGEN_WARN("Transform Feedback was active when the MeshAnimation was updated.");
		stopAnimation();
		return;
	}

	// find offst in the mesh vbo.
	// in the constructor data may not be set or data moved in vbo
	// so we lookup the offset here.
	const ref_ptr<InputContainer> &inputContainer = mesh_->inputContainer();
	const ShaderInputList &inputs = inputContainer->inputs();
	std::list<ContiguousBlock> blocks;

	if (hasMeshInterleavedAttributes_) {
		meshBufferOffset_ = (inputs.empty() ? 0 : (inputs.begin()->in_)->offset());
		for (auto it = inputs.rbegin(); it != inputs.rend(); ++it) {
			const ref_ptr<ShaderInput> &in = it->in_;
			if (!in->isVertexAttribute()) continue;
			if (in->offset() < meshBufferOffset_) {
				meshBufferOffset_ = in->offset();
			}
		}
	} else {
		// find contiguous blocks of memory in the mesh buffers.
		auto it = inputs.begin();
		blocks.emplace_back(it->in_);

		for (++it; it != inputs.end(); ++it) {
			const ref_ptr<ShaderInput> &in = it->in_;
			if (!in->isVertexAttribute()) continue;
			ContiguousBlock &activeBlock = *blocks.rbegin();
			if (activeBlock.buffer != in->buffer()) {
				blocks.emplace_back(in);
			} else if (in->offset() + in->inputSize() == activeBlock.offset) {
				// join left
				activeBlock.offset = in->offset();
				activeBlock.size += in->inputSize();
			} else if (activeBlock.offset + activeBlock.size == in->offset()) {
				// join right
				activeBlock.size += in->inputSize();
			} else {
				blocks.emplace_back(in);
			}
		}
	}

	elapsedTime_ += dt;

	// map into anim's duration
	const GLdouble duration = tickRange_.y - tickRange_.x;
	const GLdouble timeInTicks = elapsedTime_ * ticksPerSecond_ / 1000.0;
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

	GLboolean framesChanged = GL_FALSE;
	// Look for present frame number.
	GLint lastFrame = lastFramePosition_;
	GLint frame = (timeInTicks >= lastTime_ ? lastFrame : startFramePosition_);
	findFrameAfterTick(timeInTicks, frame, frames_);
	lastFramePosition_ = frame;

	// keep two frames in animation buffer
	lastFrame = frame - 1;
	MeshAnimation::KeyFrame &frame0 = frames_[lastFrame];
	if (lastFrame != pingFrame_ && lastFrame != pongFrame_) {
		loadFrame(lastFrame, frame == pingFrame_);
		framesChanged = GL_TRUE;
	}
	if (lastFrame != lastFrame_) {
		for (auto & attribute : frame0.attributes) {
			attribute.location = interpolationShader_->attributeLocation("next_" + attribute.input->name());
		}
		lastFrame_ = lastFrame;
		framesChanged = GL_TRUE;
	}
	MeshAnimation::KeyFrame &frame1 = frames_[frame];
	if (frame != pingFrame_ && frame != pongFrame_) {
		loadFrame(frame, lastFrame == pingFrame_);
		framesChanged = GL_TRUE;
	}
	if (frame != nextFrame_) {
		for (auto it = frame1.attributes.begin(); it != frame1.attributes.end(); ++it) {
			it->location = interpolationShader_->attributeLocation("last_" + it->input->name());
		}
		nextFrame_ = frame;
		framesChanged = GL_TRUE;
		REGEN_DEBUG("Next frame: " << nextFrame_ << " (time: " << timeInTicks << ")");
	}
	if (framesChanged) {
		vao_ = ref_ptr<VAO>::alloc();
		rs->vao().push(vao_->id());

		// setup attributes
		glBindBuffer(GL_ARRAY_BUFFER, frame0.ref->bufferID());
		for (auto & attribute : frame0.attributes) {
			attribute.input->enableAttribute(attribute.location);
		}
		glBindBuffer(GL_ARRAY_BUFFER, frame1.ref->bufferID());
		for (auto & attribute : frame1.attributes) {
			attribute.input->enableAttribute(attribute.location);
		}

		rs->vao().pop();
	}

	frameTimeUniform_->setVertex(0,
								 (timeInTicks - frame1.startTick) / frame1.timeInTicks);

	{ // Write interpolated attributes to transform feedback buffer
		// no FS used
		rs->toggles().push(RenderState::RASTERIZER_DISCARD, GL_TRUE);
		rs->depthMask().push(GL_FALSE);
		// setup the interpolation shader
		rs->shader().push(interpolationShader_->id());
		interpolationShader_->enable(rs);
		rs->vao().push(vao_->id());

		// setup the transform feedback
		if (hasMeshInterleavedAttributes_) {
			rs->feedbackBufferRange().push(0, bufferRange_);
		} else {
			GLint index = inputs.size() - 1;
			bufferRange_.offset_ = 0;
			for (auto it = inputs.rbegin(); it != inputs.rend(); ++it) {
				const ref_ptr<ShaderInput> &in = it->in_;
				index -= 1;
				if (!in->isVertexAttribute()) continue;
				bufferRange_.size_ = in->inputSize();
				rs->feedbackBufferRange().push(index+1, bufferRange_);
				bufferRange_.offset_ += bufferRange_.size_;
			}
		}
		rs->beginTransformFeedback(GL_POINTS);

		// finally the draw call
		glDrawArrays(GL_POINTS, 0, inputContainer->numVertices());

		rs->endTransformFeedback();
		if (hasMeshInterleavedAttributes_) {
			rs->feedbackBufferRange().pop(0);
		} else {
			GLint index = inputs.size() - 1;
			for (auto it = inputs.rbegin(); it != inputs.rend(); ++it) {
				const ref_ptr<ShaderInput> &in = it->in_;
				index -= 1;
				if (!in->isVertexAttribute()) continue;
				rs->feedbackBufferRange().pop(index+1);
			}
		}
		rs->vao().pop();
		rs->shader().pop();
		rs->depthMask().pop();
		rs->toggles().pop(RenderState::RASTERIZER_DISCARD);
	}

	// copy transform feedback buffer content to mesh buffer
	if (hasMeshInterleavedAttributes_) {
		BufferObject::copy(
				feedbackRef_->bufferID(),
				inputs.begin()->in_->buffer(),
				bufferSize_,
				0, // feedback buffer offset
				meshBufferOffset_);
	} else {
		GLuint feedbackBufferOffset = 0;
		for (auto & block : blocks) {
			BufferObject::copy(
					feedbackRef_->bufferID(),
					block.buffer,
					block.size,
					feedbackBufferOffset,
					block.offset);
			feedbackBufferOffset += block.size;
		}
	}

	lastTime_ = tickRange_.x + timeInTicks;
	GL_ERROR_LOG();
}

////////

void MeshAnimation::addFrame(
		const std::list<ref_ptr<ShaderInput> > &attributes,
		GLdouble timeInTicks) {
	const ref_ptr<InputContainer> &inputContainer = mesh_->inputContainer();
	const ShaderInputList &inputs = inputContainer->inputs();

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
	for (const auto & input : inputs) {
		const ref_ptr<ShaderInput> &in0 = input.in_;
		if (!in0->isVertexAttribute()) continue;
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

void MeshAnimation::addMeshFrame(GLdouble timeInTicks) {
	const ref_ptr<InputContainer> &inputContainer = mesh_->inputContainer();
	const ShaderInputList &inputs = inputContainer->inputs();

	std::list<ref_ptr<ShaderInput> > meshAttributes;
	for (auto it = inputs.begin(); it != inputs.end(); ++it) {
		if (!it->in_->isVertexAttribute()) continue;
		meshAttributes.push_back(ShaderInput::copy(it->in_, GL_TRUE));
	}
	addFrame(meshAttributes, timeInTicks);
}

ref_ptr<ShaderInput> MeshAnimation::findLastAttribute(const std::string &name) {
	for (auto it = frames_.rbegin(); it != frames_.rend(); ++it) {
		MeshAnimation::KeyFrame &f = *it;
		for (auto jt = f.attributes.begin(); jt != f.attributes.end(); ++jt) {
			const ref_ptr<ShaderInput> &att = jt->input;
			if (att->name() == name) {
				return ShaderInput::copy(att, GL_TRUE);
			}
		}
	}
	return {};
}

void MeshAnimation::addSphereAttributes(
		GLfloat horizontalRadius,
		GLfloat verticalRadius,
		GLdouble timeInTicks,
		const Vec3f &offset) {
	const ref_ptr<InputContainer> &inputContainer = mesh_->inputContainer();

	if (!inputContainer->hasInput(ATTRIBUTE_NAME_POS)) {
		REGEN_WARN("mesh has no input named '" << ATTRIBUTE_NAME_POS << "'");
		return;
	}
	if (!inputContainer->hasInput(ATTRIBUTE_NAME_NOR)) {
		REGEN_WARN("mesh has no input named '" << ATTRIBUTE_NAME_NOR << "'");
		return;
	}

	GLfloat radiusScale = horizontalRadius / verticalRadius;
	Vec3f scale(radiusScale, 1.0, radiusScale);

	ref_ptr<ShaderInput3f> posAtt = ref_ptr<ShaderInput3f>::dynamicCast(mesh_->positions());
	ref_ptr<ShaderInput3f> norAtt = ref_ptr<ShaderInput3f>::dynamicCast(mesh_->normals());
	// allocate memory for the animation attributes
	ref_ptr<ShaderInput3f> spherePos = ref_ptr<ShaderInput3f>::dynamicCast(
			ShaderInput::copy(posAtt, GL_FALSE));
	ref_ptr<ShaderInput3f> sphereNor = ref_ptr<ShaderInput3f>::dynamicCast(
			ShaderInput::copy(norAtt, GL_FALSE));

	// find the centroid of the mesh
	Vec3f minPos = posAtt->getVertex(0).r;
	Vec3f maxPos = posAtt->getVertex(0).r;
	for (GLuint i = 1; i < posAtt->numVertices(); ++i) {
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
	// TODO: it should be possible to do this with less artifacts by taking the faces into account.
	for (GLuint i = 0; i < spherePos->numVertices(); ++i) {
		Vec3f v = posAtt->getVertex(i).r;
		Vec3f direction = v - centroid;
		Vec3f n;
		GLdouble l = direction.length();
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
		GLfloat width,
		GLfloat height,
		GLfloat depth,
		GLdouble timeInTicks,
		const Vec3f &offset) {
	const ref_ptr<InputContainer> &inputContainer = mesh_->inputContainer();

	if (!inputContainer->hasInput(ATTRIBUTE_NAME_POS)) {
		REGEN_WARN("mesh has no input named '" << ATTRIBUTE_NAME_POS << "'");
		return;
	}
	if (!inputContainer->hasInput(ATTRIBUTE_NAME_NOR)) {
		REGEN_WARN("mesh has no input named '" << ATTRIBUTE_NAME_NOR << "'");
		return;
	}

	Vec3f boxSize(width, height, depth);
	GLdouble radius = sqrt(0.5f);

	ref_ptr<ShaderInput3f> posAtt = ref_ptr<ShaderInput3f>::dynamicCast(mesh_->positions());
	ref_ptr<ShaderInput3f> norAtt = ref_ptr<ShaderInput3f>::dynamicCast(mesh_->normals());
	// allocate memory for the animation attributes
	ref_ptr<ShaderInput3f> boxPos = ref_ptr<ShaderInput3f>::dynamicCast(
			ShaderInput::copy(posAtt, GL_FALSE));
	ref_ptr<ShaderInput3f> boxNor = ref_ptr<ShaderInput3f>::dynamicCast(
			ShaderInput::copy(norAtt, GL_FALSE));

	// set cube vertex data
	for (GLuint i = 0; i < boxPos->numVertices(); ++i) {
		Vec3f v = posAtt->getVertex(i).r;
		Vec3f n;
		GLdouble l = v.length();
		if (l == 0) {
			continue;
		}

		// first map to sphere, a bit ugly but avoids intersection calculations
		// and scaled normal as sphere position
		Vec3f vCopy = v;
		vCopy.normalize();

#if 0
		// check the coordinate values to choose the right face
		GLdouble xAbs = abs(vCopy.x);
		GLdouble yAbs = abs(vCopy.y);
		GLdouble zAbs = abs(vCopy.z);
		GLdouble factor;
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
		GLdouble xAbs = abs(vCopy.x);
		GLdouble yAbs = abs(vCopy.y);
		GLdouble zAbs = abs(vCopy.z);
		GLfloat h, factor;
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

		GLdouble maxDim = std::max(std::max(abs(vCopy.x), abs(vCopy.y)), abs(vCopy.z));
		// we divide by maxDim, so it is not allowed to be zero,
		// this happens for vCopy with only a single component not zero.
		if (maxDim != 0.0f) {
			// the distortion scale is calculated by dividing
			// the length of the vector pointing on the square surface
			// by the length of the vector pointing on the circle surface (equals circle radius).
			// size2/maxDim calculates scale factor for d to point on the square surface
			GLdouble distortionScale = ((vCopy * 0.5f / maxDim).length()) / 0.5f;
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

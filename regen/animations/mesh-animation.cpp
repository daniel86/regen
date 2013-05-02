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
    vector<KeyFrame> &keys)
{
  while(frame < (GLint) (keys.size()-1))
  {
    if (tick <= keys[frame].endTick)
    {
      return;
    }
    frame += 1;
  }
}
void MeshAnimation::findFrameBeforeTick(
    GLdouble &tick,
    GLuint &frame,
    vector<KeyFrame> &keys)
{
  for (frame=keys.size()-1; frame>0;)
  {
    if (tick >= keys[frame].startTick) {
      return;
    }
    frame -= 1;
  }
}

MeshAnimation::MeshAnimation(
    const ref_ptr<Mesh> &mesh,
    list<Interpoation> &interpolations)
: Animation(GL_TRUE,GL_FALSE),
  mesh_(mesh),
  meshBufferOffset_(-1),
  lastFrame_(-1),
  nextFrame_(-1),
  pingFrame_(-1),
  pongFrame_(-1),
  elapsedTime_(0.0),
  ticksPerSecond_(1.0),
  lastTime_(0.0),
  tickRange_(0.0,0.0),
  lastFramePosition_(0u),
  startFramePosition_(0u)
{
  const ref_ptr<ShaderInputContainer> &inputContainer = mesh->inputContainer();
  const ShaderInputList &inputs = inputContainer->inputs();
  map<GLenum,string> shaderNames;
  map<string,string> shaderConfig;
  map<string,string> functions;
  list<string> transformFeedback;

  hasMeshInterleavedAttributes_ = GL_FALSE;

  shaderNames[GL_VERTEX_SHADER] = "mesh-animation.interpolateLinear";

  // find buffer size
  bufferSize_ = 0u;
  GLuint i=0u;
  for(ShaderInputList::const_iterator it=inputs.begin(); it!=inputs.end(); ++it)
  {
    const ref_ptr<ShaderInput> &in = it->in_;
    if(!in->isVertexAttribute()) continue;
    bufferSize_ += in->size();
    transformFeedback.push_back(in->name());

    string interpolationName = "interpolate_linear";
    string interpolationKey = "";
    for(list<Interpoation>::const_iterator
        it=interpolations.begin(); it!=interpolations.end(); ++it)
    {
      if(it->attributeName == in->name()) {
        interpolationName = it->interpolationName;
        interpolationKey = it->interpolationKey;
        break;
      }
    }

    shaderConfig[REGEN_STRING("ATTRIBUTE"<<i<<"_INTERPOLATION_NAME")] = interpolationName;
    if(!interpolationKey.empty()) {
      shaderConfig[REGEN_STRING("ATTRIBUTE"<<i<<"_INTERPOLATION_KEY")] = interpolationKey;
    }
    shaderConfig[REGEN_STRING("ATTRIBUTE"<<i<<"_NAME")] = in->name();
    shaderConfig[REGEN_STRING("ATTRIBUTE"<<i<<"_TYPE")] =
        GLEnum::glslDataType(in->dataType(), in->valsPerElement());
    i += 1;
  }
  shaderConfig["NUM_ATTRIBUTES"] = REGEN_STRING(i);

  // used to save two frames
  animationBuffer_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_DYNAMIC));
  feedbackBuffer_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_FEEDBACK));
  feedbackRef_ = feedbackBuffer_->alloc(bufferSize_);

  bufferRange_.buffer_ = feedbackRef_->bufferID();
  bufferRange_.offset_ = 0;
  bufferRange_.size_ = bufferSize_;

  // create initial frame
  addMeshFrame(0.0);

  // init interpolation shader
  interpolationShader_ = Shader::create(330, shaderConfig,functions,shaderNames);
  if(hasMeshInterleavedAttributes_) {
    interpolationShader_->setTransformFeedback(
        transformFeedback, GL_INTERLEAVED_ATTRIBS, GL_VERTEX_SHADER);
  }
  else {
    interpolationShader_->setTransformFeedback(
        transformFeedback, GL_SEPARATE_ATTRIBS, GL_VERTEX_SHADER);
  }
  if(interpolationShader_.get()!=NULL &&
      interpolationShader_->compile() && interpolationShader_->link())
  {
    ref_ptr<ShaderInput> in = interpolationShader_->input("frameTimeNormalized");
    frameTimeUniform_ = (ShaderInput1f*)in.get();
    frameTimeUniform_->setUniformData(0.0f);
    interpolationShader_->setInput(in);

    in = interpolationShader_->input("friction");
    frictionUniform_ = (ShaderInput1f*)in.get();
    frictionUniform_->setUniformData(6.0f);
    interpolationShader_->setInput(in);

    in = interpolationShader_->input("frequency");
    frequencyUniform_ = (ShaderInput1f*)in.get();
    frequencyUniform_->setUniformData(3.0f);
    interpolationShader_->setInput(in);
  }
  else {
    interpolationShader_ = ref_ptr<Shader>();
  }
}

const ref_ptr<Shader>& MeshAnimation::interpolationShader() const
{
  return interpolationShader_;
}

void MeshAnimation::setTickRange(const Vec2d &forcedTickRange)
{
  // get first and last tick of animation
  if( forcedTickRange.x < 0.0f || forcedTickRange.y < 0.0f ) {
    tickRange_.x = 0.0;
    tickRange_.y = 0.0;
    for(vector<MeshAnimation::KeyFrame>::iterator
        it=frames_.begin(); it!=frames_.end(); ++it)
    {
      tickRange_.y += it->timeInTicks;
    }
  } else {
    tickRange_ = forcedTickRange;
  }

  // set start frames
  if(tickRange_.x < 0.00001) {
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

void MeshAnimation::loadFrame(GLuint frameIndex, GLboolean isPongFrame)
{
  MeshAnimation::KeyFrame& frame = frames_[frameIndex];

  list< ref_ptr<VertexAttribute> > atts;
  for(list< ShaderAttributeLocation >::iterator
      it=frame.attributes.begin(); it!=frame.attributes.end(); ++it)
  {
    atts.push_back(it->att);
  }

  if(isPongFrame) {
    if(pongFrame_!=-1) { animationBuffer_->free(pongIt_.get()); }
    pongFrame_ = frameIndex;
    if(hasMeshInterleavedAttributes_) {
      pongIt_ = animationBuffer_->allocInterleaved(atts);
    } else {
      pongIt_ = animationBuffer_->allocSequential(atts);
    }
    frame.ref = pongIt_;
  } else {
    if(pingFrame_!=-1) { animationBuffer_->free(pingIt_.get()); }
    pingFrame_ = frameIndex;
    if(hasMeshInterleavedAttributes_) {
      pingIt_ = animationBuffer_->allocInterleaved(atts);
    } else {
      pingIt_ = animationBuffer_->allocSequential(atts);
    }
    frame.ref = pingIt_;
  }
}

void MeshAnimation::glAnimate(RenderState *rs, GLdouble dt)
{
  if(rs->isTransformFeedbackAcive()) {
    REGEN_WARN("Transform Feedback was active when the MeshAnimation was updated.");
    stopAnimation();
    return;
  }

  // find offst in the mesh vbo.
  // in the constructor data may not be set or data moved in vbo
  // so we lookup the offset here.
  const ref_ptr<ShaderInputContainer> &inputContainer = mesh_->inputContainer();
  const ShaderInputList &inputs = inputContainer->inputs();
  list<ContiguousBlock> blocks;

  if(hasMeshInterleavedAttributes_) {
    meshBufferOffset_ = (inputs.empty() ? 0 : (inputs.begin()->in_)->offset());
    for(ShaderInputList::const_reverse_iterator
        it=inputs.rbegin(); it!=inputs.rend(); ++it)
    {
      const ref_ptr<ShaderInput> &in = it->in_;
      if(!in->isVertexAttribute()) continue;
      if(in->offset() < meshBufferOffset_) {
        meshBufferOffset_ = in->offset();
      }
    }
  }
  else {
    // find contiguous blocks of memory in the mesh buffers.
    ShaderInputList::const_iterator it = inputs.begin();
    blocks.push_back(ContiguousBlock(it->in_));

    for(++it; it!=inputs.end(); ++it)
    {
      const ref_ptr<ShaderInput> &in = it->in_;
      if(!in->isVertexAttribute()) continue;
      ContiguousBlock &activeBlock = *blocks.rbegin();
      if(activeBlock.buffer != in->buffer()) {
        blocks.push_back(ContiguousBlock(in));
      }
      else if(in->offset()+in->size() == activeBlock.offset) {
        // join left
        activeBlock.offset = in->offset();
        activeBlock.size += in->size();
      }
      else if(activeBlock.offset+activeBlock.size == in->offset()) {
        // join right
        activeBlock.size += in->size();
      }
      else {
        blocks.push_back(ContiguousBlock(in));
      }
    }
  }

  elapsedTime_ += dt;

  // map into anim's duration
  const GLdouble duration = tickRange_.y - tickRange_.x;
  const GLdouble timeInTicks = elapsedTime_*ticksPerSecond_/1000.0;
  if(timeInTicks > duration)
  {
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
  lastFrame = frame-1;
  MeshAnimation::KeyFrame& frame0 = frames_[lastFrame];
  if(lastFrame!=pingFrame_ && lastFrame!=pongFrame_) {
    loadFrame(lastFrame, frame==pingFrame_);
    framesChanged = GL_TRUE;
  }
  if(lastFrame!=lastFrame_) {
    for(list< ShaderAttributeLocation >::iterator
        it=frame0.attributes.begin(); it!=frame0.attributes.end(); ++it)
    {
      it->location = interpolationShader_->attributeLocation("next_"+it->att->name());
    }
    lastFrame_ = lastFrame;
    framesChanged = GL_TRUE;
  }
  MeshAnimation::KeyFrame& frame1 = frames_[frame];
  if(frame!=pingFrame_ && frame!=pongFrame_) {
    loadFrame(frame, lastFrame==pingFrame_);
    framesChanged = GL_TRUE;
  }
  if(frame!=nextFrame_) {
    for(list< ShaderAttributeLocation >::iterator
        it=frame1.attributes.begin(); it!=frame1.attributes.end(); ++it)
    {
      it->location = interpolationShader_->attributeLocation("last_"+it->att->name());
    }
    nextFrame_ = frame;
    framesChanged = GL_TRUE;
  }
  if(framesChanged) {
    vao_ = ref_ptr<VertexArrayObject>::manage(new VertexArrayObject);
    rs->vao().push(vao_->id());

    // setup attributes
    glBindBuffer(GL_ARRAY_BUFFER, frame0.ref->bufferID());
    for(list<ShaderAttributeLocation>::iterator
        it=frame0.attributes.begin(); it!=frame0.attributes.end(); ++it)
    { it->att->enable(it->location); }
    glBindBuffer(GL_ARRAY_BUFFER, frame1.ref->bufferID());
    for(list<ShaderAttributeLocation>::iterator
        it=frame1.attributes.begin(); it!=frame1.attributes.end(); ++it)
    { it->att->enable(it->location); }

    rs->vao().pop();
  }

  frameTimeUniform_->setVertex1f(0,
      (timeInTicks-frame1.startTick)/frame1.timeInTicks);

  { // Write interpolated attributes to transform feedback buffer
    // no FS used
    rs->toggles().push(RenderState::RASTARIZER_DISCARD, GL_TRUE);
    rs->depthMask().push(GL_FALSE);
    // setup the interpolation shader
    rs->shader().push(interpolationShader_->id());
    interpolationShader_->enable(rs);
    rs->vao().push(vao_->id());

    // setup the transform feedback
    if(hasMeshInterleavedAttributes_) {
      rs->feedbackBufferRange().push(0,bufferRange_);
    }
    else {
      GLint index = inputs.size()-1;
      bufferRange_.offset_ = 0;
      for(ShaderInputList::const_reverse_iterator it=inputs.rbegin(); it!=inputs.rend(); ++it)
      {
        const ref_ptr<ShaderInput> &in = it->in_;
        if(!in->isVertexAttribute()) continue;
        bufferRange_.size_ = in->size();
        rs->feedbackBufferRange().push(index,bufferRange_);
        bufferRange_.offset_ += bufferRange_.size_;
        index -= 1;
      }
    }
    rs->beginTransformFeedback(GL_POINTS);

    // finally the draw call
    glDrawArrays(GL_POINTS, 0, inputContainer->numVertices());

    rs->endTransformFeedback();
    if(hasMeshInterleavedAttributes_) {
      rs->feedbackBufferRange().pop(0);
    }
    else {
      GLint index = inputs.size()-1;
      for(ShaderInputList::const_reverse_iterator it=inputs.rbegin(); it!=inputs.rend(); ++it)
      {
        const ref_ptr<ShaderInput> &in = it->in_;
        if(!in->isVertexAttribute()) continue;
        rs->feedbackBufferRange().pop(index);
        index -= 1;
      }
    }
    rs->vao().pop();
    rs->shader().pop();
    rs->depthMask().pop();
    rs->toggles().pop(RenderState::RASTARIZER_DISCARD);
  }

  // copy transform feedback buffer content to mesh buffer
  if(hasMeshInterleavedAttributes_) {
    VertexBufferObject::copy(
        feedbackRef_->bufferID(),
        inputs.begin()->in_->buffer(),
        bufferSize_,
        0, // feedback buffer offset
        meshBufferOffset_);
  }
  else {
    GLuint feedbackBufferOffset = 0;
    for(list<ContiguousBlock>::iterator
        it=blocks.begin(); it!=blocks.end(); ++it)
    {
      ContiguousBlock &block = *it;
      VertexBufferObject::copy(
          feedbackRef_->bufferID(),
          block.buffer,
          block.size,
          feedbackBufferOffset,
          block.offset);
      feedbackBufferOffset += block.size;
    }
  }

  lastTime_ = tickRange_.x + timeInTicks;
}

////////

void MeshAnimation::addFrame(
    const list< ref_ptr<VertexAttribute> > &attributes,
    GLdouble timeInTicks)
{
  const ref_ptr<ShaderInputContainer> &inputContainer = mesh_->inputContainer();
  const ShaderInputList &inputs = inputContainer->inputs();

  MeshAnimation::KeyFrame frame;

  frame.timeInTicks = timeInTicks;
  frame.startTick = 0.0;
  for(vector<MeshAnimation::KeyFrame>::reverse_iterator it=frames_.rbegin(); it!=frames_.rend(); ++it)
  {
    MeshAnimation::KeyFrame &parentFrame = *it;
    frame.startTick += parentFrame.timeInTicks;
  }
  frame.endTick = frame.startTick + frame.timeInTicks;

  // add attributes
  for(ShaderInputList::const_iterator it=inputs.begin(); it!=inputs.end(); ++it)
  {
    const ref_ptr<ShaderInput> &in0 = it->in_;
    if(!in0->isVertexAttribute()) continue;
    ref_ptr<VertexAttribute> att;
    // find specified attribute
    for(list< ref_ptr<VertexAttribute> >::const_iterator
        jt=attributes.begin(); jt!=attributes.end(); ++jt)
    {
      const ref_ptr<VertexAttribute> &in1 = *jt;
      if(in0->name() == in1->name()) {
        att = in1;
        break;
      }
    }
    if(att.get() == NULL) {
      // find attribute from previous frames
      att = findLastAttribute(in0->name());
    }
    if(att.get() != NULL) {
      frame.attributes.push_back(ShaderAttributeLocation(att,-1));
    }
  }

  frames_.push_back(frame);
}

void MeshAnimation::addMeshFrame(GLdouble timeInTicks)
{
  const ref_ptr<ShaderInputContainer> &inputContainer = mesh_->inputContainer();
  const ShaderInputList &inputs = inputContainer->inputs();

  list< ref_ptr<VertexAttribute> > meshAttributes;
  for(ShaderInputList::const_iterator it=inputs.begin(); it!=inputs.end(); ++it)
  {
    if(!it->in_->isVertexAttribute()) continue;
    meshAttributes.push_back(ref_ptr<VertexAttribute>::manage(
        new VertexAttribute(*(it->in_.get()), GL_TRUE) ));
  }
  addFrame(meshAttributes, timeInTicks);
}

ref_ptr<VertexAttribute> MeshAnimation::findLastAttribute(const string &name)
{
  for(vector<MeshAnimation::KeyFrame>::reverse_iterator
      it=frames_.rbegin(); it!=frames_.rend(); ++it)
  {
    MeshAnimation::KeyFrame &f = *it;
    for(list<ShaderAttributeLocation>::const_iterator
        jt=f.attributes.begin(); jt!=f.attributes.end(); ++jt)
    {
      const ref_ptr<VertexAttribute> &att = jt->att;
      if(att->name() == name) {
        return ref_ptr<VertexAttribute>::manage(
            new VertexAttribute(*att.get(), GL_TRUE));
      }
    }
  }
  return ref_ptr<VertexAttribute>();
}

void MeshAnimation::addSphereAttributes(
    GLfloat horizontalRadius,
    GLfloat verticalRadius,
    GLdouble timeInTicks)
{
  const ref_ptr<ShaderInputContainer> &inputContainer = mesh_->inputContainer();

  if(!inputContainer->hasInput(ATTRIBUTE_NAME_POS)) {
    REGEN_WARN("mesh has no input named '" << ATTRIBUTE_NAME_POS << "'");
    return;
  }
  if(!inputContainer->hasInput(ATTRIBUTE_NAME_NOR)) {
    REGEN_WARN("mesh has no input named '" << ATTRIBUTE_NAME_NOR << "'");
    return;
  }

  GLdouble radiusScale = horizontalRadius/verticalRadius;
  Vec3f scale(radiusScale, 1.0, radiusScale);

  ref_ptr<ShaderInput> posAtt = mesh_->positions();
  ref_ptr<ShaderInput> norAtt = mesh_->normals();
  // allocate memory for the animation attributes
  ref_ptr<VertexAttribute> spherePos = ref_ptr<VertexAttribute>::manage(
      new VertexAttribute(*posAtt.get(), GL_FALSE)
  );
  ref_ptr<VertexAttribute> sphereNor = ref_ptr<VertexAttribute>::manage(
      new VertexAttribute(*norAtt.get(), GL_FALSE)
  );

  // set sphere vertex data
  for(GLuint i=0; i<spherePos->numVertices(); ++i)
  {
    Vec3f v = posAtt->getVertex3f(i);
    Vec3f n;
    GLdouble l = v.length();
    if(l == 0) {
      continue;
    }

    // take normalized direction vector as normal
    n = v;
    n.normalize();
    // and scaled normal as sphere position
    // 1e-1 to avoid fighting
    v = n*scale*verticalRadius*(1.0f + l*1e-1);

    spherePos->setVertex3f(i, v);
    sphereNor->setVertex3f(i, n);
  }

  list< ref_ptr<VertexAttribute> > attributes;
  attributes.push_back(spherePos);
  attributes.push_back(sphereNor);
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
    GLdouble timeInTicks)
{
  const ref_ptr<ShaderInputContainer> &inputContainer = mesh_->inputContainer();

  if(!inputContainer->hasInput(ATTRIBUTE_NAME_POS)) {
    REGEN_WARN("mesh has no input named '" << ATTRIBUTE_NAME_POS << "'");
    return;
  }
  if(!inputContainer->hasInput(ATTRIBUTE_NAME_NOR)) {
    REGEN_WARN("mesh has no input named '" << ATTRIBUTE_NAME_NOR << "'");
    return;
  }

  Vec3f boxSize(width, height, depth);
  GLdouble radius = sqrt(0.5f);

  ref_ptr<ShaderInput> posAtt = mesh_->positions();
  ref_ptr<ShaderInput> norAtt = mesh_->normals();
  // allocate memory for the animation attributes
  ref_ptr<VertexAttribute> boxPos = ref_ptr<VertexAttribute>::manage(
      new VertexAttribute(*posAtt.get(), GL_FALSE)
  );
  ref_ptr<VertexAttribute> boxNor = ref_ptr<VertexAttribute>::manage(
      new VertexAttribute(*norAtt.get(), GL_FALSE)
  );

  // set cube vertex data
  for(GLuint i=0; i<boxPos->numVertices(); ++i)
  {
    Vec3f v = posAtt->getVertex3f(i);
    Vec3f n;
    GLdouble l = v.length();
    if(l == 0) {
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
    GLdouble h, factor;
    // set the coordinate for the face to the cube size
    if(xAbs > yAbs && xAbs > zAbs) { // left/right face
      factor = (v.x<0 ? -1 : 1);
      n = (Vec3f(1,0,0))*factor;
      h = vCopy.x;
    } else if(yAbs > zAbs) { // top/bottom face
      factor = (v.y<0 ? -1 : 1);
      n = (Vec3f(0,1,0))*factor;
      h = vCopy.y;
    } else { //front/back face
      factor = (v.z<0 ? -1 : 1);
      n = (Vec3f(0,0,1))*factor;
      h = vCopy.z;
    }

    Vec3f r = vCopy - n*n.dot(vCopy)*2.0f;
    // reflect vector on cube face plane (-r*(factor*0.5f-h)/h) and
    // delete component of face direction (-n*0.5f , 0.5f because thats the sphere radius)
    vCopy += -r*(factor*0.5f-h)/h - n*0.5f;

    GLdouble maxDim = max(max(abs(vCopy.x),abs(vCopy.y)),abs(vCopy.z));
    // we divide by maxDim, so it is not allowed to be zero,
    // this happens for vCopy with only a single component not zero.
    if(maxDim!=0.0f) {
      // the distortion scale is calculated by dividing
      // the length of the vector pointing on the square surface
      // by the length of the vector pointing on the circle surface (equals circle radius).
      // size2/maxDim calculates scale factor for d to point on the square surface
      GLdouble distortionScale = ( ( vCopy * 0.5f/maxDim ).length() ) / 0.5f;
      vCopy *= distortionScale;
    }

    // -l*1e-6 to avoid fighting
    v = (vCopy+n*0.5f)*boxSize*(1.0f + l*1e-4);
#endif

    boxPos->setVertex3f(i, v);
    boxNor->setVertex3f(i, n);
  }

  list< ref_ptr<VertexAttribute> > attributes;
  attributes.push_back(boxPos);
  attributes.push_back(boxNor);
  addFrame(attributes, timeInTicks);
}

/*
 * mesh-animation-gpu.cpp
 *
 *  Created on: 29.10.2012
 *      Author: daniel
 */

#include "mesh-animation.h"

#include <limits.h>
#include <ogle/utility/gl-error.h>
#include <ogle/utility/string-util.h>

static void findFrameAfterTick(
    GLdouble tick,
    GLint &frame,
    vector<MeshKeyFrame> &keys)
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
static void findFrameBeforeTick(
    GLdouble &tick,
    GLuint &frame,
    vector<MeshKeyFrame> &keys)
{
  for (frame=keys.size()-1; frame>0;)
  {
    if (tick >= keys[frame].startTick) {
      return;
    }
    frame -= 1;
  }
}

unsigned int MeshAnimation::ANIMATION_STOPPED =
    EventObject::registerEvent("animationStopped");

MeshAnimation::MeshAnimation(ref_ptr<MeshState> &mesh)
: Animation(),
  mesh_(mesh),
  renderBufferOffset_(-1),
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
  const list< ref_ptr<ShaderInput> > &inputs = mesh_->inputs();
  map<GLenum,string> shaderNames;
  map<string,string> shaderConfig;
  map<string,string> functions;
  list<string> transformFeedback;

  shaderNames[GL_VERTEX_SHADER] = "mesh-animation.interpolateLinear";
  shaderConfig["NUM_ATTRIBUTES"] = FORMAT_STRING(inputs.size());

  // find buffer size
  GLuint bufferSize = 0, i=0;
  for(list< ref_ptr<ShaderInput> >::const_iterator it=inputs.begin(); it!=inputs.end(); ++it)
  {
    const ref_ptr<ShaderInput> &in = *it;
    bufferSize += in->size();
    transformFeedback.push_back(in->name());

    shaderConfig[FORMAT_STRING("ATTRIBUTE"<<i<<"_NAME")] = in->name();
    shaderConfig[FORMAT_STRING("ATTRIBUTE"<<i<<"_TYPE")] = in->shaderDataType();
    i += 1;
  }

  // used to save two frames
  animationBuffer_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_DYNAMIC, 2*bufferSize));
  // target where interpolated values are saved
  feedbackBuffer_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_DYNAMIC, bufferSize));

  // create initial frame
  addMeshFrame(0.0);

  // init interpolation shader
  interpolationShader_ = Shader::create(shaderConfig,functions,shaderNames);
  interpolationShader_->setTransformFeedback(transformFeedback, GL_INTERLEAVED_ATTRIBS);
  if(interpolationShader_.get()!=NULL &&
      interpolationShader_->compile() && interpolationShader_->link())
  {
    INFO_LOG("mesh animation shader compiled successful.");
    ref_ptr<ShaderInput> in = interpolationShader_->input("frameTimeNormalized");
    frameTimeUniform_ = (ShaderInput1f*)in.get();
    frameTimeUniform_->setUniformData(0.0f);
    interpolationShader_->setInput(in);
  }
  else {
    WARN_LOG("mesh animation shader failed to compiled.");
    interpolationShader_ = ref_ptr<Shader>();
  }
}

void MeshAnimation::setTickRange(const Vec2d &forcedTickRange)
{
  // get first and last tick of animation
  if( forcedTickRange.x < 0.0f || forcedTickRange.y < 0.0f ) {
    tickRange_.x = 0.0;
    tickRange_.y = 0.0;
    for(vector<MeshKeyFrame>::iterator
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

void MeshAnimation::loadFrame(GLuint frameIndex,
    GLboolean isPongFrame)
{
  MeshKeyFrame& frame = frames_[frameIndex];
  list< ref_ptr<VertexAttribute> > atts;

  // update locations
  for(list< ShaderAttributeLocation >::iterator
      it=frame.attributes.begin(); it!=frame.attributes.end(); ++it)
  {
    atts.push_back(it->att);
  }

  if(isPongFrame) {
    if(pongFrame_!=-1) { animationBuffer_->free(pongIt_); }
    pongFrame_ = frameIndex;
    pongIt_ = animationBuffer_->allocateSequential(atts);
  } else {
    if(pingFrame_!=-1) { animationBuffer_->free(pingIt_); }
    pingFrame_ = frameIndex;
    pingIt_ = animationBuffer_->allocateSequential(atts);
  }
}

void MeshAnimation::animate(GLdouble dt){}

void MeshAnimation::updateGraphics(GLdouble dt)
{
  if(!mesh_->isBufferSet()) { return; }

  // find offst in the mesh vbo.
  // in the constructor data may not be set or data moved in vbo
  // so we lookup the offset here.
  const list< ref_ptr<ShaderInput> > &inputs = mesh_->inputs();
  renderBufferOffset_ = (inputs.empty() ? 0 : (*inputs.begin())->offset());
  for(list< ref_ptr<ShaderInput> >::const_iterator
      it=inputs.begin(); it!=inputs.end(); ++it)
  {
    const ref_ptr<ShaderInput> &in = *it;
    if(in->offset() < renderBufferOffset_) {
      renderBufferOffset_ = in->offset();
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
    emitEvent(ANIMATION_STOPPED);
    return;
  }

  // Look for present frame number.
  GLint lastFrame = lastFramePosition_;
  GLint frame = (timeInTicks >= lastTime_ ? lastFrame : startFramePosition_);
  findFrameAfterTick(timeInTicks, frame, frames_);
  lastFramePosition_ = frame;

  // keep two frames in animation buffer
  lastFrame = frame-1;
  MeshKeyFrame& frame0 = frames_[lastFrame];
  if(lastFrame!=pingFrame_ && lastFrame!=pongFrame_) {
    loadFrame(lastFrame, frame==pingFrame_);
  }
  if(lastFrame!=lastFrame_) {
    for(list< ShaderAttributeLocation >::iterator
        it=frame0.attributes.begin(); it!=frame0.attributes.end(); ++it)
    {
      it->location = interpolationShader_->attributeLocation("next_"+it->att->name());
    }
    lastFrame_ = lastFrame;
  }
  MeshKeyFrame& frame1 = frames_[frame];
  if(frame!=pingFrame_ && frame!=pongFrame_) {
    loadFrame(frame, lastFrame==pingFrame_);
  }
  if(frame!=nextFrame_) {
    for(list< ShaderAttributeLocation >::iterator
        it=frame1.attributes.begin(); it!=frame1.attributes.end(); ++it)
    {
      it->location = interpolationShader_->attributeLocation("last_"+it->att->name());
    }
    nextFrame_ = frame;
  }

  { // Write interpolated attributes to transform feedback buffer
    // no FS used
    glEnable(GL_RASTERIZER_DISCARD);
    glDepthMask(GL_FALSE);

    // setup the interpolation shader
    glUseProgram(interpolationShader_->id());

    frameTimeUniform_->setVertex1f(0,
        (timeInTicks-frame1.startTick)/frame1.timeInTicks);

    interpolationShader_->uploadInputs();

    // currently active frames are saved in animation buffer
    glBindBuffer(GL_ARRAY_BUFFER, animationBuffer_->id());
    // setup the transform feedback
    glBindBufferRange(
        GL_TRANSFORM_FEEDBACK_BUFFER,
        0, feedbackBuffer_->id(),
        0, feedbackBuffer_->bufferSize()
    );

    glBeginTransformFeedback(GL_POINTS);

    // setup attributes
    for(list<ShaderAttributeLocation>::iterator
        it=frame0.attributes.begin(); it!=frame0.attributes.end(); ++it)
    {
      it->att->enable(it->location);
    }
    for(list<ShaderAttributeLocation>::iterator
        it=frame1.attributes.begin(); it!=frame1.attributes.end(); ++it)
    {
      it->att->enable(it->location);
    }

    // finally the draw call
    glDrawArrays(GL_POINTS, 0, mesh_->numVertices());

    // cleanup
    glEndTransformFeedback();
    glDisable(GL_RASTERIZER_DISCARD);
    glDepthMask(GL_TRUE);
  }

  // copy transform feedback buffer content to render buffer
  VertexBufferObject::copy(
      feedbackBuffer_->id(),
      mesh_->vertexBuffer(),
      feedbackBuffer_->bufferSize(),
      0, // feedback buffer offset
      renderBufferOffset_);

  lastTime_ = tickRange_.x + timeInTicks;
}

////////

void MeshAnimation::addFrame(
    list< ref_ptr<VertexAttribute> > attributes,
    GLdouble timeInTicks)
{
  MeshKeyFrame frame;

  frame.timeInTicks = timeInTicks;
  frame.startTick = 0.0;
  for(vector<MeshKeyFrame>::reverse_iterator it=frames_.rbegin(); it!=frames_.rend(); ++it)
  {
    MeshKeyFrame &parentFrame = *it;
    frame.startTick += parentFrame.timeInTicks;
  }
  frame.endTick = frame.startTick + frame.timeInTicks;

  // add attributes
  for(list< ref_ptr<ShaderInput> >::const_iterator
      it=mesh_->inputs().begin(); it!=mesh_->inputs().end(); ++it)
  {
    const ref_ptr<ShaderInput> &in0 = *it;
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

  list< ref_ptr<VertexAttribute> > meshAttributes;
  for(list< ref_ptr<ShaderInput> >::const_iterator
      it=mesh_->inputs().begin(); it!=mesh_->inputs().end(); ++it)
  {
    meshAttributes.push_back(ref_ptr<VertexAttribute>::manage(
        new VertexAttribute(*it->get(), GL_TRUE) ));
  }
  addFrame(meshAttributes, timeInTicks);
}

ref_ptr<VertexAttribute> MeshAnimation::findLastAttribute(const string &name)
{
  for(vector<MeshKeyFrame>::reverse_iterator
      it=frames_.rbegin(); it!=frames_.rend(); ++it)
  {
    MeshKeyFrame &f = *it;
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
  if(!mesh_->hasInput(ATTRIBUTE_NAME_POS)) {
    WARN_LOG("mesh has no input named '" << ATTRIBUTE_NAME_POS << "'");
    return;
  }
  if(!mesh_->hasInput(ATTRIBUTE_NAME_NOR)) {
    WARN_LOG("mesh has no input named '" << ATTRIBUTE_NAME_NOR << "'");
    return;
  }

  GLdouble radiusScale = horizontalRadius/verticalRadius;
  Vec3f scale(radiusScale, 1.0, radiusScale);

  const ref_ptr<ShaderInput> &posAtt = *mesh_->vertices();
  const ref_ptr<ShaderInput> &norAtt = *mesh_->normals();
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
    GLdouble l = length(v);
    if(l == 0) {
      continue;
    }

    // take normalized direction vector as normal
    n = v;
    normalize(n);
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

void MeshAnimation::addBoxAttributes(
    GLfloat width,
    GLfloat height,
    GLfloat depth,
    GLdouble timeInTicks)
{
  if(!mesh_->hasInput(ATTRIBUTE_NAME_POS)) {
    WARN_LOG("mesh has no input named '" << ATTRIBUTE_NAME_POS << "'");
    return;
  }
  if(!mesh_->hasInput(ATTRIBUTE_NAME_NOR)) {
    WARN_LOG("mesh has no input named '" << ATTRIBUTE_NAME_NOR << "'");
    return;
  }

  Vec3f boxSize(width, height, depth);
  GLdouble radius = sqrt(0.5f);

  const ref_ptr<ShaderInput> &posAtt = *mesh_->vertices();
  const ref_ptr<ShaderInput> &norAtt = *mesh_->normals();
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
    GLdouble l = length(v);
    if(l == 0) {
      continue;
    }

    // first map to sphere, a bit ugly but avoids intersection calculations
    // and scaled normal as sphere position
    Vec3f vCopy = v;
    normalize(vCopy);
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

    Vec3f r = vCopy - n*dot(n,vCopy)*2.0f;
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
      GLdouble distortionScale = ( length( vCopy * 0.5f/maxDim ) ) / 0.5f;
      vCopy *= distortionScale;
    }

    // -l*1e-6 to avoid fighting
    v = (vCopy+n*0.5f)*boxSize*(1.0f + l*1e-4);

    boxPos->setVertex3f(i, v);
    boxNor->setVertex3f(i, n);
  }

  list< ref_ptr<VertexAttribute> > attributes;
  attributes.push_back(boxPos);
  attributes.push_back(boxNor);
  addFrame(attributes, timeInTicks);
}

/*
 * mesh-animation.cpp
 *
 *  Created on: 21.08.2012
 *      Author: daniel
 */

#include "mesh-animation.h"

#include <limits.h>
#include <ogle/utility/gl-error.h>

static void findFrameAfterTick(
    GLdouble tick,
    GLuint &frame,
    vector<MeshKeyFrame> &keys)
{
  while (frame < keys.size()-1)
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
  elapsedTime_(0.0),
  ticksPerSecond_(1.0),
  lastTime_(0.0),
  tickRange_(0.0,0.0),
  lastFramePosition_(0u),
  startFramePosition_(0u)
{

  MeshKeyFrame initialFrame;
  initialFrame.timeInTicks = 0.0;
  mapOffset_ = UINT_MAX;
  GLuint mapOffsetEnd_ = 0u;
  for(list< ref_ptr<ShaderInput> >::const_iterator
      it=mesh_->inputs().begin(); it!=mesh_->inputs().end(); ++it)
  {
    const ref_ptr<ShaderInput> &in = *it;
    if(in->offset()<mapOffset_)
    {
      mapOffset_ = in->offset();
    }

    GLuint end = in->offset();
    if(in->stride()==0) {
      end += in->size();
    } else {
      if(in->numInstances()>1) {
        end += in->stride() * in->numInstances()/in->divisor();
      } else {
        end += in->stride() * in->numVertices();
      }
    }
    if(end>mapOffsetEnd_)
    {
      mapOffsetEnd_ = end;
    }

    initialFrame.attributes.push_back(ref_ptr<VertexAttribute>::cast( *it ));
  }
  mapSize_ = mapOffsetEnd_ - mapOffset_;
  addFrame(initialFrame);
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

void MeshAnimation::animate(GLdouble dt)
{
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
  }
  else
  {
    // Look for present frame number.
    GLuint lastFrame = lastFramePosition_;
    GLuint frame = (timeInTicks >= lastTime_ ? lastFrame : startFramePosition_);
    findFrameAfterTick(timeInTicks, frame, frames_);
    lastFramePosition_ = frame;

    MeshKeyFrame& key = frames_[frame];

    GLdouble frameTimeNormalized = (timeInTicks-key.startTick)/key.timeInTicks;

    for(list< ref_ptr<VertexAttribute> >::iterator
        it=key.attributes.begin(); it!=key.attributes.end(); ++it)
    {
      ref_ptr<VertexAttribute> &nextAttribute = *it;
      ref_ptr<VertexAttribute> &lastAttribute = key.previousAttributes[nextAttribute->name()];
      if(lastAttribute.get()==NULL) {
        WARN_LOG("unable to animate attribute '" << nextAttribute->name() << "'");
      } else {
        ref_ptr<VertexAttribute> interpolated = key.interpolator->operator()(
            nextAttribute, lastAttribute, frameTimeNormalized);
        lock(); {
          attributes_[nextAttribute->name()] = interpolated ;
        } unlock();
      }
    }

    lastTime_ = tickRange_.x + timeInTicks;
  }
}

void MeshAnimation::updateGraphics(GLdouble dt)
{
  if(attributes_.empty()) { return; }
  if(!mesh_->isBufferSet()) { return; }

  list< ref_ptr<VertexAttribute> > atts;
  lock(); {
    for(map< string, ref_ptr<VertexAttribute> >::iterator
        it=attributes_.begin(); it!=attributes_.end(); ++it)
    {
      atts.push_back(it->second);
    }
    attributes_.clear();
  } unlock();

  // this here is slow but it was easy to implement.
  // Later this should be improved:
  // * consider ping pong vbo.
  // * use less glBufferSubData calls
  GLuint vbo = mesh_->vertexBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  for(list< ref_ptr<VertexAttribute> >::iterator
      it=atts.begin(); it!=atts.end(); ++it)
  {
    ref_ptr<VertexAttribute> &attribute = *it;
    // get animation data pointer. attribute is tightly packed here.
    byte *animData = attribute->dataPtr();
    // get the pointer to first attribute element in vbo.
    // the attribute may be added interleaved to the vbo (then stride>0 is true)
    GLuint offset_ = attribute->offset();

    if(attribute->stride() > 0)
    {
      // vbo data is interleaved, we have to copy single elements here
      const GLuint numVertices = attribute->size()/attribute->elementSize();
      for(GLuint i=0; i<numVertices; ++i)
      {
        glBufferSubData(GL_ARRAY_BUFFER, offset_, attribute->elementSize(), animData);
        animData += attribute->elementSize();
        offset_ += attribute->stride();
      }
    }
    else
    {
      glBufferSubData(GL_ARRAY_BUFFER, offset_, attribute->size(), animData);
    }
  }
}

////////

void MeshAnimation::addFrame(MeshKeyFrame &frame)
{
  frame.startTick = 0.0;
  // remember parent attributes
  for(vector<MeshKeyFrame>::reverse_iterator
      it=frames_.rbegin(); it!=frames_.rend(); ++it)
  {
    MeshKeyFrame &parentFrame = *it;
    frame.startTick += parentFrame.timeInTicks;
    for(list< ref_ptr<VertexAttribute> >::iterator
        jt=parentFrame.attributes.begin(); jt!=parentFrame.attributes.end(); ++jt)
    {
      ref_ptr<VertexAttribute> &parentAtt = *jt;

      if(frame.previousAttributes.count(parentAtt->name())==0)
      {
        frame.previousAttributes[parentAtt->name()] = parentAtt;
      }
    }
  }
  frame.endTick = frame.startTick + frame.timeInTicks;
  if(frame.interpolator.get()==NULL)
  {
    frame.interpolator = ref_ptr<VertexInterpolator>::manage(new LinearVertexInterpolator);
  }
  frames_.push_back(frame);
}

void MeshAnimation::addMeshAttribute(
    MeshKeyFrame &frame, const string &attributeName)
{
  if(!mesh_->hasInput(attributeName)) {
    WARN_LOG("mesh has no input named '" << attributeName << "'");
    return;
  }
  frame.attributes.push_back(
      ref_ptr<VertexAttribute>::cast(mesh_->getInputPtr(attributeName)));
}

void MeshAnimation::addSphereAttributes(
    MeshKeyFrame &frame,
    GLfloat horizontalRadius,
    GLfloat verticalRadius)
{
  if(!mesh_->hasInput(ATTRIBUTE_NAME_POS)) {
    WARN_LOG("mesh has no input named '" << ATTRIBUTE_NAME_POS << "'");
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

  frame.attributes.push_back(spherePos);
  frame.attributes.push_back(sphereNor);
}

void MeshAnimation::addBoxAttributes(
    MeshKeyFrame &frame,
    GLfloat width,
    GLfloat height,
    GLfloat depth)
{
  if(!mesh_->hasInput(ATTRIBUTE_NAME_POS)) {
    WARN_LOG("mesh has no input named '" << ATTRIBUTE_NAME_POS << "'");
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
    v = (vCopy+n*0.5f)*boxSize*(1.0f + l*1e-6);

    //n = v;
    normalize(n);

    boxPos->setVertex3f(i, v);
    boxNor->setVertex3f(i, n);
  }

  frame.attributes.push_back(boxPos);
  frame.attributes.push_back(boxNor);
}

///////

VertexInterpolator::VertexInterpolator()
{
}

///////

template< typename T >
static void interpolateLinear(
    ref_ptr<VertexAttribute> previousAtt,
    ref_ptr<VertexAttribute> nextAtt,
    ref_ptr<VertexAttribute> animAtt,
    GLdouble frameTimeNormalized
    )
{
  T *next = (T*)nextAtt->dataPtr();
  T *previous = (T*)previousAtt->dataPtr();
  T *anim = (T*)animAtt->dataPtr();
  GLuint count = nextAtt->size()/sizeof(T);
  for(GLuint i=0; i<count; ++i) {
    anim[i] = frameTimeNormalized*next[i] + (1.0-frameTimeNormalized)*previous[i];
  }
}

LinearVertexInterpolator::LinearVertexInterpolator()
: VertexInterpolator()
{

}

ref_ptr<VertexAttribute> LinearVertexInterpolator::operator()(
    ref_ptr<VertexAttribute> &previousAtt,
    ref_ptr<VertexAttribute> &nextAtt,
    GLdouble frameTimeNormalized)
{
  ref_ptr<VertexAttribute> animAtt = ref_ptr<VertexAttribute>::manage(
      new VertexAttribute(*nextAtt.get(), GL_FALSE));
  switch(nextAtt->dataType())
  {
  case GL_DOUBLE:
    interpolateLinear<GLdouble>(nextAtt, previousAtt, animAtt, frameTimeNormalized);
    break;
  case GL_FLOAT:
    interpolateLinear<GLfloat>(nextAtt, previousAtt, animAtt, frameTimeNormalized);
    break;
  case GL_INT:
    interpolateLinear<GLint>(nextAtt, previousAtt, animAtt, frameTimeNormalized);
    break;
  case GL_UNSIGNED_INT:
    interpolateLinear<GLuint>(nextAtt, previousAtt, animAtt, frameTimeNormalized);
    break;
  default:
    WARN_LOG("unknown data type '" << nextAtt->dataType() << "'");
  }
  return animAtt;
}

/////////

NearestVertexInterpolator::NearestVertexInterpolator()
: VertexInterpolator()
{

}

ref_ptr<VertexAttribute> NearestVertexInterpolator::operator()(
    ref_ptr<VertexAttribute> &previousAtt,
    ref_ptr<VertexAttribute> &nextAtt,
    GLdouble frameTimeNormalized)
{
  if(frameTimeNormalized < 0.5)
  {
    return previousAtt;
  }
  else
  {
    return nextAtt;
  }
}

/////////

OscillateVertexInterpolator::OscillateVertexInterpolator()
: LinearVertexInterpolator(),
  friction_(6.0),
  frequency_(3.0)
{
}

void OscillateVertexInterpolator::set_friction(GLdouble friction)
{
  friction_ = friction;
}
void OscillateVertexInterpolator::set_frequency(GLdouble frequency)
{
  frequency_ = frequency;
}

ref_ptr<VertexAttribute> OscillateVertexInterpolator::operator()(
    ref_ptr<VertexAttribute> &previousAtt,
    ref_ptr<VertexAttribute> &nextAtt,
    GLdouble t)
{
  GLfloat amplitude = exp(-friction_*t);
  GLfloat pos = abs( amplitude*cos(frequency_*t*2.5*M_PI) );
  return LinearVertexInterpolator::operator()(previousAtt,nextAtt,1.0-pos);
}

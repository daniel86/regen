/*
 * picking.cpp
 *
 *  Created on: 17.02.2013
 *      Author: daniel
 */

#include <ogle/states/discard-rasterizer.h>
#include <ogle/states/depth-state.h>

#include "picking.h"

GLuint Picking::PICK_EVENT = EventObject::registerEvent("pickEvent");

Picking::Picking()
: State()
{
  pickedMesh_ = NULL;
  pickedInstance_ = 0;
  pickedObject_ = 0;
}

const MeshState* Picking::pickedMesh() const
{
  return pickedMesh_;
}
GLint Picking::pickedInstance() const
{
  return pickedInstance_;
}
GLint Picking::pickedObject() const
{
  return pickedObject_;
}

void Picking::emitPickEvent()
{
  PickEvent ev;
  ev.instanceId = pickedInstance_;
  ev.objectId = pickedObject_;
  ev.state = pickedMesh_;
  emitEvent(PICK_EVENT, &ev);
}

//////////////
//////////////

PickingGeom::PickingGeom(GLuint maxPickedObjects)
: Picking(), RenderState()
{
  // TODO: PICKING: invalid operation
  shaderMap_ = new map< Shader*, ref_ptr<Shader> >();
  nextShaderMap_ = new map< Shader*, ref_ptr<Shader> >();

  pickObjectID_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("pickObjectID"));
  pickObjectID_->setUniformData(0);

  feedbackBuffer_ = ref_ptr<VertexBufferObject>::manage(new VertexBufferObject(
      VertexBufferObject::USAGE_DYNAMIC,
      sizeof(PickData)*maxPickedObjects)
  );
  feedbackCount_ = 0;
  lastFeedbackOffset_ = 0;

  joinStates(ref_ptr<State>::manage(new DiscardRasterizer));

  ref_ptr<DepthState> depth = ref_ptr<DepthState>::manage(new DepthState);
  depth->set_useDepthWrite(GL_FALSE);
  joinStates(ref_ptr<State>::cast(depth));

  {
    pickerCode_ = Shader::load("picking.gs");
    pickerShader_ = glCreateShader(GL_GEOMETRY_SHADER);

    GLint length = -1, status = 0;
    const char *cstr = pickerCode_.c_str();
    glShaderSource(pickerShader_, 1, &cstr, &length);
    glCompileShader(pickerShader_);

    glGetShaderiv(pickerShader_, GL_COMPILE_STATUS, &status);
    if(!status) {
      Shader::printLog(pickerShader_,
          GL_GEOMETRY_SHADER, pickerCode_.c_str(), GL_FALSE);
    }
  }

  glGenQueries(1, &countQuery_);
}
PickingGeom::~PickingGeom()
{
  delete shaderMap_;
  delete nextShaderMap_;
  glDeleteQueries(1, &countQuery_);
}

ref_ptr<Shader> PickingGeom::createPickShader(Shader *shader)
{
  static const GLenum stages[] =
  {
      GL_VERTEX_SHADER,
      GL_TESS_CONTROL_SHADER,
      GL_TESS_EVALUATION_SHADER
  };
  map< GLenum, string > shaderCode;
  map< GLenum, ref_ptr<GLuint> > shaders;

  // set picking geometry shader
  shaderCode[GL_GEOMETRY_SHADER] = pickerCode_;
  GLuint *gsID = new GLuint;
  *gsID = pickerShader_;
  shaders[GL_GEOMETRY_SHADER] = ref_ptr<GLuint>::manage(gsID);

  // copy stages from provided shader
  for(GLint i=0; i<3; ++i) {
    GLenum stage = stages[i];
    if(shader->hasStage(stage)) {
      shaderCode[stage] = shader->stageCode(stage);
      shaders[stage] = shader->stage(stage);
    }
  }

  ref_ptr<Shader> pickShader =
      ref_ptr<Shader>::manage(new Shader(shaderCode,shaders));

  list<string> tfNames;
  tfNames.push_back("pickObjectID");
  tfNames.push_back("pickInstanceID");
  tfNames.push_back("pickDepth");
  pickShader->setTransformFeedback(tfNames, GL_SEPARATE_ATTRIBS, GL_GEOMETRY_SHADER);

  if(pickShader->link()) {
    // TODO: PICKING: viewport and mouse position set ?
    pickShader->setInputs(shader->inputs());
    pickShader->setInput(ref_ptr<ShaderInput>::cast(pickObjectID_));
    return pickShader;
  } else {
    return ref_ptr<Shader>();
  }
}
Shader* PickingGeom::getPickShader(Shader *shader)
{
  map< Shader*, ref_ptr<Shader> >::iterator needle = shaderMap_->find(shader);
  if(needle == shaderMap_->end()) {
    ref_ptr<Shader> pickShader = createPickShader(shader);
    (*shaderMap_)[shader] = pickShader;
    (*nextShaderMap_)[shader] = pickShader;
    return pickShader.get();
  }
  else {
    (*nextShaderMap_)[shader] = needle->second;
    return needle->second.get();
  }
}

// XXX: do not use virtual
void PickingGeom::pushShader(Shader *shader)
{
  Shader *pickShader = getPickShader(shader);
  RenderState::pushShader(pickShader);

  GLint feedbackOffset = feedbackCount_*sizeof(PickData);
  if(lastFeedbackOffset_!=feedbackOffset) {
    // we have to re-bind the buffer with offset each time
    // there was something written to it.
    // not sure if there is a better way offsetting the buffer
    // access then rebinding it.
    glBindBufferRange(
        GL_TRANSFORM_FEEDBACK_BUFFER,
        0,
        feedbackBuffer_->id(),
        feedbackOffset,
        feedbackBuffer_->bufferSize()-feedbackOffset
    );
    lastFeedbackOffset_ = feedbackOffset;
  }
  glBeginQuery(GL_PRIMITIVES_GENERATED, countQuery_);
  glBeginTransformFeedback(GL_POINTS);
}
// XXX: do not use virtual
void PickingGeom::popShader()
{
  glEndTransformFeedback();
  glEndQuery(GL_PRIMITIVES_GENERATED);
  // remember number of hovered objects,
  // depth test is done later on CPU
  GLint count;
  glGetQueryObjectiv(countQuery_, GL_QUERY_RESULT, &count);
  feedbackCount_ += count;

  RenderState::popShader();
}

// XXX: do not use virtual
void PickingGeom::pushMesh(MeshState *mesh)
{
#if 0
  RenderState::pushMesh(mesh);
  // map mesh to pickObjectID_
  meshes_.push_back(mesh);
#endif
}
// XXX: do not use virtual
void PickingGeom::popMesh()
{
#if 0
  RenderState::popMesh();
  // next object id
  pickObjectID_->getVertex1i(0) += 1;
#endif
}

void PickingGeom::enable(RenderState *rs)
{
  // bind buffer for first mesh
  lastFeedbackOffset_ = -1;
  // first mesh gets id=1
  pickObjectID_->setVertex1i(0, 1);
  State::enable(this);
}
void PickingGeom::disable(RenderState *rs)
{
  State::disable(this);
  updatePickedObject();
  // remember used shaders.
  // this will remove all unused pick shaders.
  // we could do this by event handlers...
  map< Shader*, ref_ptr<Shader> > *buf = shaderMap_;
  shaderMap_ = nextShaderMap_;
  nextShaderMap_ = buf;
  nextShaderMap_->clear();

  handleGLError("After PickingGeom.");
}

void PickingGeom::updatePickedObject()
{
  if(feedbackCount_==0) { // no mesh hovered
    if(pickedMesh_ != NULL) {
      pickedMesh_ = NULL;
      pickedInstance_ = 0;
      pickedObject_ = 0;
      emitPickEvent();
    }
    return;
  }

  GLint maxObjectId = pickObjectID_->getVertex1i(0);

  glBindBuffer(GL_ARRAY_BUFFER, feedbackBuffer_->id());
  // tell GL that we do not care for buffer data after
  // mapping
  glBufferData(GL_ARRAY_BUFFER, feedbackBuffer_->bufferSize(), NULL, GL_STREAM_DRAW);

  PickData *bufferData = (PickData*) glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
  // find pick result with min depth
  PickData *bestPicked = &bufferData[0];
  for(GLuint i=1; i<feedbackCount_; ++i) {
    PickData &picked = bufferData[i];
    if(picked.depth<bestPicked->depth) {
      bestPicked = &picked;
    }
  }
  PickData picked = *bestPicked;
  glUnmapBuffer(GL_ARRAY_BUFFER);

  if(maxObjectId<picked.objectID || picked.objectID==0) {
    ERROR_LOG("Invalid pick object ID '" << (picked.objectID-1) << "'");
  }
  else {
    // find picked mesh and emit signal
    MeshState *pickedMesh = meshes_[picked.objectID-1];
    if(pickedMesh != pickedMesh_ ||  picked.instanceID != pickedInstance_)
    {
      pickedMesh_ = pickedMesh;
      pickedInstance_ = picked.instanceID;
      pickedObject_ = picked.objectID;
      emitPickEvent();
    }
  }

  meshes_.clear();
  feedbackCount_ = 0;
}

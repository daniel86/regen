/*
 * picker.cpp
 *
 *  Created on: 03.09.2012
 *      Author: daniel
 */

#include "picker.h"

#include <ogle/states/render-state.h>
#include <ogle/states/mesh-state.h>
#include <ogle/utility/gl-error.h>
#include <ogle/models/sky-box.h>

GLboolean Picker::pickerInitialled = GL_FALSE;
string Picker::pickerCode[3];
GLuint Picker::pickerShader[3];

struct PickData {
  GLint objectID;
  GLint instanceID;
  GLfloat depth;
};

class PickerRenderState : public RenderState
{
public:
  PickerRenderState(Picker *picker)
  : RenderState(),
    picker_(picker)
  {
  }

  virtual void pushShader(Shader *shader)
  {
    // TODO: what input ?
    RenderState::pushShader(picker_->getPickShader(shader,GS_INPUT_TRIANGLES));
    picker_->pushPickShader();
  }
  virtual void popShader()
  {
    picker_->popPickShader();
    RenderState::popShader();
  }

  virtual void pushMesh(MeshState *mesh)
  {
    RenderState::pushMesh(mesh);
    picker_->meshes_.push_back(mesh);
  }
  virtual void popMesh()
  {
    ++picker_->pickObjectID_->getVertex1i(0);
    RenderState::popMesh();
  }

  virtual GLboolean isStateHidden(State *s)
  {
    return RenderState::isStateHidden(s) ||
        dynamic_cast<SkyBox*>(s)!=NULL;
  }

protected:
  Picker *picker_;
};

GLuint Picker::PICK_EVENT =
    EventObject::registerEvent("oglePickEvent");

Picker::Picker(
    ref_ptr<StateNode> &node,
    GLuint maxPickedObjects)
: Animation(),
  node_(node),
  feedbackCount_(0u),
  pickInterval_(50.0),
  dt_(0.0)
{
  if(pickerInitialled==GL_FALSE) {
    initPicker();
  }

  shaderMap_ = new map< Shader*, ref_ptr<Shader> >();
  nextShaderMap_ = new map< Shader*, ref_ptr<Shader> >();

  glGenQueries(1, &countQuery_);

  pickObjectID_ = ref_ptr<ShaderInput1i>::manage(
      new ShaderInput1i("pickObjectID"));
  pickObjectID_->setUniformData(0);

  feedbackBuffer_ = ref_ptr<VertexBufferObject>::manage(new VertexBufferObject(
      VertexBufferObject::USAGE_DYNAMIC,
      sizeof(PickData)*maxPickedObjects)
  );
}

Picker::~Picker()
{
  delete shaderMap_;
  delete nextShaderMap_;
  glDeleteQueries(1, &countQuery_);
}

void Picker::set_pickInterval(GLdouble pickInterval)
{
  pickInterval_ = pickInterval;
}


void Picker::initPicker()
{
  const string shaderCfg[] = {"IS_POINT","IS_LINE","IS_TRIANGLE"};
  string pickerGS = Shader::load("utility.picking.gs");
  for(GLint i=0; i<3; ++i)
  {
    GLint length = -1, status;
    stringstream code;
    code << "#version 150" << endl;
    code << "#define " << shaderCfg[i] << endl;
    code << pickerGS << endl;

    pickerCode[i] = code.str();
    pickerShader[i] = glCreateShader(GL_GEOMETRY_SHADER);

    const char *cstr = pickerCode[i].c_str();
    glShaderSource(pickerShader[i], 1, &cstr, &length);
    glCompileShader(pickerShader[i]);

    glGetShaderiv(pickerShader[i], GL_COMPILE_STATUS, &status);
    if (!status) {
      Shader::printLog(pickerShader[i], GL_GEOMETRY_SHADER, pickerCode[i].c_str(), GL_FALSE);
    }
  }
  pickerInitialled = GL_TRUE;
}

Shader* Picker::getPickShader(Shader *shader, GeometryShaderInput in)
{
  map< Shader*, ref_ptr<Shader> >::iterator needle = shaderMap_->find(shader);
  if(needle == shaderMap_->end()) {
    ref_ptr<Shader> pickShader = createPickShader(shader,in);
    (*shaderMap_)[shader] = pickShader;
    (*nextShaderMap_)[shader] = pickShader;
    return pickShader.get();
  } else {
    (*nextShaderMap_)[shader] = needle->second;
    return needle->second.get();
  }
}

void Picker::pushPickShader()
{
  GLint feedbackOffset = feedbackCount_*sizeof(PickData);
  if(lastFeedbackOffset_!=feedbackOffset)
  {
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

void Picker::popPickShader()
{
  glEndTransformFeedback();
  glEndQuery(GL_PRIMITIVES_GENERATED);
  // remember number of hovered objects,
  // depth test is done later on CPU
  GLint count;
  glGetQueryObjectiv(countQuery_, GL_QUERY_RESULT, &count);
  feedbackCount_ += count;
}

ref_ptr<Shader> Picker::createPickShader(
    Shader *shader, GeometryShaderInput in)
{
  static const GLenum stages[] =
  {
      GL_VERTEX_SHADER,
      GL_TESS_CONTROL_SHADER,
      GL_TESS_EVALUATION_SHADER
  };
  map< GLenum, string > shaderCode;
  map< GLenum, GLuint > shaders;
  map<string, ref_ptr<ShaderInput> > inputs = shader->inputs();
  inputs[pickObjectID_->name()] = ref_ptr<ShaderInput>::cast(pickObjectID_);

  switch(in) {
  case GS_INPUT_POINTS:
    shaderCode[GL_GEOMETRY_SHADER] = pickerCode[0];
    shaders[GL_GEOMETRY_SHADER] = pickerShader[0];
    break;
  case GS_INPUT_LINES:
  case GS_INPUT_LINES_ADJACENCY:
    shaderCode[GL_GEOMETRY_SHADER] = pickerCode[1];
    shaders[GL_GEOMETRY_SHADER] = pickerShader[1];
    break;
  case GS_INPUT_TRIANGLES:
  case GS_INPUT_TRIANGLES_ADJACENCY:
    shaderCode[GL_GEOMETRY_SHADER] = pickerCode[2];
    shaders[GL_GEOMETRY_SHADER] = pickerShader[2];
    break;
  }

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
  tfNames.push_back("out_pickObjectID");
  tfNames.push_back("out_pickInstanceID");
  tfNames.push_back("out_pickDepth");
  pickShader->setTransformFeedback(tfNames, GL_INTERLEAVED_ATTRIBS);

  if(pickShader->link()) {
    pickShader->setInputs(inputs);
    return pickShader;
  } else {
    return ref_ptr<Shader>();
  }
}

void Picker::animate(GLdouble dt)
{
}

void Picker::updateGraphics(GLdouble dt)
{
  dt_ += dt;
  if(dt_ < pickInterval_) { return; }
  dt_ = 0.0;

  {
    GLint lastSize = meshes_.size();
    meshes_ = vector<MeshState*>();
    meshes_.reserve(lastSize);
  }
  // bind buffer for first mesh
  lastFeedbackOffset_ = -1;
  // find mesh gets id=1
  pickObjectID_->setVertex1i(0, 1);

  PickerRenderState rs(this);
  rs.set_useTransformFeedback(GL_TRUE);

  // do not use fragment shader
  glEnable(GL_RASTERIZER_DISCARD);
  glDepthMask(GL_FALSE);

  // find parents of pick node
  list<StateNode*> parents;
  StateNode* parent = node_->parent().get();
  while(parent!=NULL) {
    parents.push_front(parent);
    parent = parent->parent().get();
  }

  // enable parent nodes
  for(list<StateNode*>::iterator
      it=parents.begin(); it!=parents.end(); ++it)
  {
    (*it)->enable(&rs);
  }

  // tree traversal
  node_->traverse(&rs, dt);

  // disable parent nodes
  for(list<StateNode*>::reverse_iterator
      it=parents.rbegin(); it!=parents.rend(); ++it)
  {
    (*it)->disable(&rs);
  }

  // remember used shaders.
  // this will remove all unused pick shaders.
  // we could do this by event handlers...
  map< Shader*, ref_ptr<Shader> > *buf = shaderMap_;
  shaderMap_ = nextShaderMap_;
  nextShaderMap_ = buf;
  nextShaderMap_->clear();

  updatePickedObject();

  // revert states
  glDisable(GL_RASTERIZER_DISCARD);
  glDepthMask(GL_TRUE);
}

void Picker::updatePickedObject()
{
  if(feedbackCount_==0)
  {
    if(pickedMesh_ != NULL)
    {
      pickedMesh_ = NULL;
      pickedInstance_ = 0;

      PickEvent pickEvent;
      pickEvent.instanceId = pickedInstance_;
      pickEvent.objectId = 0;
      pickEvent.state = pickedMesh_;
      emitEvent(PICK_EVENT, &pickEvent);
    }
  }
  else
  {
    glBindBuffer(GL_ARRAY_BUFFER, feedbackBuffer_->id());
    // tell GL that we do not care for buffer data after
    // mapping
    glBufferData(GL_ARRAY_BUFFER, feedbackBuffer_->bufferSize(), NULL, GL_STREAM_DRAW);
    PickData *bufferData = (PickData*) glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);

    PickData *bestPicked = &bufferData[0];

    // find pick result with min depth
    for(GLint i=1; i<feedbackCount_; ++i) {
      PickData &picked = bufferData[i];
      if(picked.depth<bestPicked->depth) {
        bestPicked = &picked;
      }
    }

    // find picked mesh and emit signal
    MeshState *pickedMesh = meshes_[bestPicked->objectID-1];
    if(pickedMesh != pickedMesh_ ||
        bestPicked->instanceID != pickedInstance_)
    {
      pickedMesh_ = pickedMesh;
      pickedInstance_ = bestPicked->instanceID;

      PickEvent pickEvent;
      pickEvent.instanceId = pickedInstance_;
      pickEvent.objectId = bestPicked->objectID;
      pickEvent.state = pickedMesh_;
      emitEvent(PICK_EVENT, &pickEvent);
    }

    feedbackCount_ = 0;

    glUnmapBuffer(GL_ARRAY_BUFFER);
  }
}

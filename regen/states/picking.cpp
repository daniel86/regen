/*
 * picking.cpp
 *
 *  Created on: 17.02.2013
 *      Author: daniel
 */

#include <regen/states/atomic-states.h>
#include <regen/states/depth-state.h>
#include <regen/states/vao-state.h>

#include "picking.h"
using namespace regen;

GLuint PickingGeom::PICK_EVENT = EventObject::registerEvent("pickEvent");

PickingGeom::PickingGeom(const ref_ptr<Texture> &depthTexture, GLuint maxPickedObjects)
: State(), Animation(GL_TRUE,GL_FALSE), pickMeshID_(1)
{
  pickedMesh_ = NULL;
  pickedInstance_ = 0;
  pickedObject_ = 0;
  dt_ = 0.0;
  pickInterval_ = 50.0;

  pickObjectID_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("pickObjectID"));
  pickObjectID_->setUniformData(0);

  feedbackBuffer_ = ref_ptr<VertexBufferObject>::manage(new VertexBufferObject(
      VertexBufferObject::USAGE_DYNAMIC,
      sizeof(PickData)*maxPickedObjects)
  );
  bufferRange_.buffer_ = feedbackBuffer_->id();

  joinStates(ref_ptr<State>::manage(
      new ToggleState(RenderState::RASTARIZER_DISCARD, GL_TRUE)));

  //depthTexture_ = ref_ptr<TextureState>::manage(new TextureState(depthTexture, "depthTexture"));
  //joinStates(ref_ptr<State>::cast(depthTexture_));

  ref_ptr<DepthState> depth = ref_ptr<DepthState>::manage(new DepthState);
  depth->set_useDepthWrite(GL_FALSE);
  depth->set_useDepthTest(GL_FALSE);
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
  glDeleteQueries(1, &countQuery_);
}

void PickingGeom::set_pickInterval(GLdouble interval)
{
  pickInterval_ = interval;
}

void PickingGeom::emitPickEvent()
{
  PickEvent ev;
  ev.instanceId = pickedInstance_;
  ev.objectId = pickedObject_;
  ev.state = pickedMesh_;
  State::emitEvent(PICK_EVENT, &ev);
}

const Mesh* PickingGeom::pickedMesh() const
{
  return pickedMesh_;
}
GLint PickingGeom::pickedInstance() const
{
  return pickedInstance_;
}
GLint PickingGeom::pickedObject() const
{
  return pickedObject_;
}

ref_ptr<ShaderState> PickingGeom::createPickShader(Shader *shader)
{
  static const GLenum stages[] =
  {
        GL_VERTEX_SHADER
#ifdef GL_TESS_CONTROL_SHADER
      , GL_TESS_CONTROL_SHADER
#endif
#ifdef GL_TESS_EVALUATION_SHADER
      , GL_TESS_EVALUATION_SHADER
#endif
  };
  map< GLenum, string > shaderCode;
  map< GLenum, ref_ptr<GLuint> > shaders;

  // set picking geometry shader
  shaderCode[GL_GEOMETRY_SHADER] = pickerCode_;
  GLuint *gsID = new GLuint;
  *gsID = pickerShader_;
  shaders[GL_GEOMETRY_SHADER] = ref_ptr<GLuint>::manage(gsID);

  // copy stages from provided shader
  for(GLuint i=0; i<sizeof(stages)/sizeof(GLenum); ++i) {
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
  pickShader->setTransformFeedback(tfNames, GL_INTERLEAVED_ATTRIBS, GL_GEOMETRY_SHADER);

  if(!pickShader->link()) return ref_ptr<ShaderState>();

  pickShader->setInputs(shader->inputs());
  pickShader->setInput(ref_ptr<ShaderInput>::cast(pickObjectID_));
  for(list<ShaderTextureLocation>::const_iterator
      it=shader->textures().begin(); it!=shader->textures().end(); ++it)
  { pickShader->setTexture(it->channel, it->name); }
  //pickShader->setTexture(depthTexture_->channel(), depthTexture_->name());

  ref_ptr<ShaderState> state =
      ref_ptr<ShaderState>::manage(new ShaderState(pickShader));
  return state;
}

GLboolean PickingGeom::add(
    const ref_ptr<Mesh> &mesh,
    const ref_ptr<StateNode> &meshNode,
    const ref_ptr<Shader> &meshShader)
{
  PickMesh pickMesh;
  pickMesh.mesh_ = mesh;
  pickMesh.meshNode_ = meshNode;
  do {
    pickMesh.id_ = ++pickMeshID_;
  } while( pickMesh.id_==0 ||
      meshes_.count(pickMesh.id_)>0);
  pickMesh.pickShader_ = createPickShader(meshShader.get());
  if(pickMesh.pickShader_.get() == NULL) { return GL_FALSE; }

  meshes_[pickMesh.id_] = pickMesh;
  meshToID_[mesh.get()] = pickMesh.id_;

  return GL_TRUE;
}

void PickingGeom::remove(Mesh *mesh)
{
  if(meshToID_.count(mesh)==0) { return; }

  GLint id = meshToID_[mesh];
  meshToID_.erase(mesh);
  meshes_.erase(id);
}

void PickingGeom::glAnimate(RenderState *rs, GLdouble dt)
{
  dt_ += dt;
  if(dt_ < pickInterval_) { return; }
  dt_ = 0.0;
  update(rs);
}

void PickingGeom::update(RenderState *rs)
{
  // bind buffer for first mesh
  GLuint feedbackCount=0;

  if(rs->isTransformFeedbackAcive()) {
    WARN_LOG("Transform Feedback was active when the Geometry Picker was updated.");
    return;
  }

  State::enable(rs);

  for(map<GLint,PickMesh>::iterator
      it=meshes_.begin(); it!=meshes_.end(); ++it)
  {
    PickMesh &m = it->second;

    pickObjectID_->setVertex1i(0, m.id_);
    m.pickShader_->enable(rs);
    rs->shader().lock();

    bufferRange_.offset_ = feedbackCount*sizeof(PickData);
    bufferRange_.size_ = feedbackBuffer_->bufferSize()-bufferRange_.offset_;
    rs->feedbackBufferRange().push(0, bufferRange_);
    glBeginQuery(GL_PRIMITIVES_GENERATED, countQuery_);
    rs->beginTransformFeedback(GL_POINTS);

    RootNode::traverse(rs, m.meshNode_.get());

    rs->endTransformFeedback();
    rs->feedbackBufferRange().pop(0);
    // remember number of hovered objects,
    // depth test is done later on CPU
    glEndQuery(GL_PRIMITIVES_GENERATED);
    feedbackCount += getGLQueryResult(countQuery_);

    rs->shader().unlock();
    m.pickShader_->disable(rs);
  }

  State::disable(rs);

  updatePickedObject(rs,feedbackCount);

}

void PickingGeom::updatePickedObject(RenderState *rs, GLuint feedbackCount)
{
  if(feedbackCount==0) { // no mesh hovered
    if(pickedMesh_ != NULL) {
      pickedMesh_ = NULL;
      pickedInstance_ = 0;
      pickedObject_ = 0;
      emitPickEvent();
    }
    return;
  }

  rs->arrayBuffer().push(feedbackBuffer_->id());
  PickData *bufferData = (PickData*) glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
  // find pick result with min depth
  PickData *bestPicked = &bufferData[0];
  for(GLuint i=1; i<feedbackCount; ++i) {
    PickData &picked = bufferData[i];
    if(picked.depth<bestPicked->depth) {
      bestPicked = &picked;
    }
  }
  PickData picked = *bestPicked;
  glUnmapBuffer(GL_ARRAY_BUFFER);
  rs->arrayBuffer().pop();

  if(picked.objectID==0) {
    ERROR_LOG("Invalid zero pick object ID" <<
        " count=" << feedbackCount <<
        " depth=" << picked.depth <<
        " instance=" << picked.instanceID << ".");
    return;
  }
  map<GLint,PickMesh>::iterator it = meshes_.find(picked.objectID);
  if(it==meshes_.end()) {
    ERROR_LOG("Invalid pick object ID " << picked.objectID <<
        " count=" << feedbackCount <<
        " depth=" << picked.depth <<
        " instance=" << picked.instanceID << ".");
    return;
  }

  if(picked.objectID!=pickedObject_ ||  picked.instanceID!=pickedInstance_)
  {
    pickedMesh_ = it->second.mesh_.get();
    pickedInstance_ = picked.instanceID;
    pickedObject_ = picked.objectID;
    emitPickEvent();
  }
}

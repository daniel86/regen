/*
 * picker.cpp
 *
 *  Created on: 03.09.2012
 *      Author: daniel
 */

#include "picker.h"

#include <ogle/shader/shader-manager.h>
#include <ogle/states/render-state.h>
#include <ogle/states/mesh-state.h>
#include <ogle/utility/gl-error.h>
#include <ogle/models/sky-box.h>

GLboolean Picker::pickerInitialled = GL_FALSE;
string Picker::pickerCode[3];
GLuint Picker::pickerShader[3];
ShaderFunctions Picker::pickerFunction[3];

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
    RenderState::pushShader(picker_->getPickShader(shader));
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
  GeometryShaderConfig gsCfg;

  gsCfg.output = GS_OUTPUT_POINTS;
  gsCfg.maxVertices = 1;
  gsCfg.invocations = 1;

  GeometryShaderInput inputs[3] = {
      GS_INPUT_POINTS,
      GS_INPUT_LINES,
      GS_INPUT_TRIANGLES
  };

  string pickOutput[3];
  // pick shader for points
  pickOutput[0] =
      "vec2 deviceToScreenSpace(vec3 vertexDS, vec2 screen){\n"
      "    return (vertexDS.xy*0.5 + vec2(0.5))*screen;\n"
      "}\n"
      "void writePickOutput() {\n"
      "    vec3 dev0 = gl_in[0].gl_Position.xyz/gl_in[0].gl_Position.w;\n"
      "    vec2 winPos0 = deviceToScreenSpace(dev0, in_viewport);\n"
      "    float d = distance(winPos0,in_mousePosition);\n"
      "    if(d<gl_in[0].gl_PointSize) {\n"
      "        out_pickObjectID = in_pickObjectID;\n"
      "        out_pickInstanceID = in_instanceID[0];\n"
      "        out_pickDepth = dev0.z;\n"
      "        EmitVertex();\n"
      "        EndPrimitive();\n"
      "    }\n"
      "}\n";
  // pick shader for lines
  pickOutput[1] =
      "vec2 deviceToScreenSpace(vec3 vertexDS, vec2 screen){\n"
      "    return (vertexDS.xy*0.5 + vec2(0.5))*screen;\n"
      "}\n"
      "float intersectionDepth(vec3 dev0, vec3 dev1, vec2 mouseDev)"
      "{"
      "    float dm0 = distance(mouseDev,dev0.xy);\n"
      "    float dm1 = distance(mouseDev,dev1.xy);\n"
      "    return (dev0.z*dm1 + dev1.z*dm0)/(dm0+dm1);\n"
      "}\n"
      "bool intersectsLine(vec2 win0, vec2 win1, vec2 winMouse, float epsilon)"
      "{"
      "    float a = distance(winMouse,win0);\n"
      "    float b = distance(winMouse,win1);\n"
      "    float c = distance(win0,win1);\n"
      "    float a2 = a*a;\n"
      "    float b2 = b*b;\n"
      "    float c2 = c*c;\n"
      "    float ca = (a2 + c2 - b2)/(2.0*c);\n"
      "    float cb = (b2 + c2 - a2)/(2.0*c);\n"
      "    return (ca+cb)<=(c+epsilon) && (b2 - cb*cb)<epsilon;\n"
      "}\n"
      "void writePickOutput() {\n"
      "    vec3 dev0 = gl_in[0].gl_Position.xyz/gl_in[0].gl_Position.w;\n"
      "    vec3 dev1 = gl_in[1].gl_Position.xyz/gl_in[1].gl_Position.w;\n"
      "    vec2 win0 = deviceToScreenSpace(dev0, in_viewport);\n"
      "    vec2 win1 = deviceToScreenSpace(dev1, in_viewport);\n"
      "    if(intersectsLine(win0,win1,in_mousePosition,gl_in[0].gl_PointSize)) {"
      "        vec2 mouseDev = (2.0*(in_mousePosition/in_viewport) - vec2(1.0));\n"
      "        out_pickObjectID = in_pickObjectID;\n"
      "        out_pickInstanceID = in_instanceID[0];\n"
      "        out_pickDepth = intersectionDepth(dev0, dev1, mouseDev);\n"
      "        EmitVertex();\n"
      "        EndPrimitive();\n"
      "    }\n"
      "}\n";
  // pick shader for triangles
  pickOutput[2] =
      "vec2 barycentricCoordinate(vec3 dev0, vec3 dev1, vec3 dev2, vec2 mouseDev) {\n"
      "   vec2 u = dev2.xy - dev0.xy;\n"
      "   vec2 v = dev1.xy - dev0.xy;\n"
      "   vec2 r = mouseDev - dev0.xy;\n"
      "   float d00 = dot(u, u);\n"
      "   float d01 = dot(u, v);\n"
      "   float d02 = dot(u, r);\n"
      "   float d11 = dot(v, v);\n"
      "   float d12 = dot(v, r);\n"
      "   float id = 1.0 / (d00 * d11 - d01 * d01);\n"
      "   float ut = (d11 * d02 - d01 * d12) * id;\n"
      "   float vt = (d00 * d12 - d01 * d02) * id;\n"
      "   return vec2(ut, vt);\n"
      "}\n"
      "bool isInsideTriangle(vec2 b)\n"
      "{\n"
      "   return (\n"
      "       (b.x >= 0.0) &&\n"
      "       (b.y >= 0.0) &&\n"
      "       (b.x + b.y <= 1.0)\n"
      "   );\n"
      "}\n"
      "float intersectionDepth(vec3 dev0, vec3 dev1, vec3 dev2, vec2 mouseDev)"
      "{"
      "    float dm0 = distance(mouseDev,dev0.xy);\n"
      "    float dm1 = distance(mouseDev,dev1.xy);\n"
      "    float dm2 = distance(mouseDev,dev2.xy);\n"
      "    float dm12 = dm1+dm2;\n"
      "    return (dm2/dm12)*((dev0.z*dm1 + dev1.z*dm0)/(dm0+dm1)) +\n"
      "           (dm1/dm12)*((dev0.z*dm2 + dev2.z*dm0)/(dm0+dm2));\n"
      "}\n"
      "void writePickOutput() {\n"
      "    vec3 dev0 = gl_in[0].gl_Position.xyz/gl_in[0].gl_Position.w;\n"
      "    vec3 dev1 = gl_in[1].gl_Position.xyz/gl_in[1].gl_Position.w;\n"
      "    vec3 dev2 = gl_in[2].gl_Position.xyz/gl_in[2].gl_Position.w;\n"
      "    vec2 mouseDev = (2.0*(in_mousePosition/in_viewport) - vec2(1.0));\n"
      "    vec2 bc = barycentricCoordinate(dev0, dev1, dev2, mouseDev);\n"
      "    if(isInsideTriangle(bc)) {\n"
      "        out_pickObjectID = in_pickObjectID;\n"
      "        out_pickInstanceID = in_instanceID[0];\n"
      "        out_pickDepth = intersectionDepth(dev0,dev1,dev2,mouseDev);\n"
      "        EmitVertex();\n"
      "        EndPrimitive();\n"
      "    }\n"
      "}\n";

  for(GLint i=0; i<3; ++i)
  {
    ShaderFunctions gs;
    GLint length = -1, status;

    gsCfg.input = inputs[i];

    gs.addOutput(GLSLTransfer("int", "out_pickObjectID"));
    gs.addOutput(GLSLTransfer("int", "out_pickInstanceID"));
    gs.addOutput(GLSLTransfer("float", "out_pickDepth"));
    gs.addInput(GLSLTransfer("int", "in_instanceID", i+1, GL_TRUE));
    gs.addUniform(GLSLUniform("vec2", "in_mousePosition"));
    gs.addUniform(GLSLUniform("vec2", "in_viewport"));
    gs.addUniform(GLSLUniform("int", "in_pickObjectID"));
    gs.addStatement(GLSLStatement("writePickOutput();"));
    gs.addDependencyCode("writePickOutput", pickOutput[i]);
    gs.set_gsConfig(gsCfg);

    pickerFunction[i] = gs;
    // XXX: not nice forcing fs prefix for inputs here.
    //  this is done for name matching of instance id.
    //  better use layout(location=..) here ?
    pickerCode[i] = ShaderManager::generateSource(
        gs, GL_GEOMETRY_SHADER, GL_NONE, "fs");
    pickerShader[i] = glCreateShader(GL_GEOMETRY_SHADER);

    const char *cstr = pickerCode[i].c_str();
    glShaderSource(pickerShader[i], 1, &cstr, &length);
    glCompileShader(pickerShader[i]);

    glGetShaderiv(pickerShader[i], GL_COMPILE_STATUS, &status);
    if (!status) {
      Shader::printLog(pickerShader[i],
          GL_GEOMETRY_SHADER, pickerCode[i].c_str(), GL_FALSE);
    }
  }


  pickerInitialled = GL_TRUE;
}

Shader* Picker::getPickShader(Shader *shader)
{
  map< Shader*, ref_ptr<Shader> >::iterator needle = shaderMap_->find(shader);
  if(needle == shaderMap_->end()) {
    ref_ptr<Shader> pickShader = createPickShader(shader);
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

ref_ptr<Shader> Picker::createPickShader(Shader *shader)
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

  if(shader->isPointShader()) {
    shaderCode[GL_GEOMETRY_SHADER] = pickerCode[0];
    shaders[GL_GEOMETRY_SHADER] = pickerShader[0];
  } else if(shader->isLineShader()) {
    shaderCode[GL_GEOMETRY_SHADER] = pickerCode[1];
    shaders[GL_GEOMETRY_SHADER] = pickerShader[1];
  } else {
    shaderCode[GL_GEOMETRY_SHADER] = pickerCode[2];
    shaders[GL_GEOMETRY_SHADER] = pickerShader[2];
  }

  for(GLint i=0; i<3; ++i)
  {
    GLenum stage = stages[i];
    if(shader->hasShader(stage))
    {
      shaderCode[stage] = shader->shaderCode(stage);
      shaders[stage] = shader->shader(stage);
    }
  }

  ref_ptr<Shader> pickShader = ref_ptr<Shader>::manage(new Shader(shaderCode));
  pickShader->setShaders(shaders);

  list<string> tfNames;
  tfNames.push_back("out_pickObjectID");
  tfNames.push_back("out_pickInstanceID");
  tfNames.push_back("out_pickDepth");
  pickShader->setupTransformFeedback(tfNames, GL_INTERLEAVED_ATTRIBS);

  if(pickShader->link())
  {
    // load uniform and attribute locations
    ShaderManager::setupLocations(pickShader, inputs);
    pickShader->setupInputs(inputs);
  }

  return pickShader;
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
      emit(PICK_EVENT, &pickEvent);
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
      emit(PICK_EVENT, &pickEvent);
    }

    feedbackCount_ = 0;

    glUnmapBuffer(GL_ARRAY_BUFFER);
  }
}

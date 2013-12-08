/*
 * input.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "input.h"
using namespace regen::scene;
using namespace regen;

#include <regen/scene/value-generator.h>
#include <regen/scene/resource-manager.h>

#define REGEN_INPUT_STATE_CATEGORY "input"

template<class U, class T>
static ref_ptr<U> createShaderInput_(
    SceneInputNode &input, const T &defaultValue)
{
  if(!input.hasAttribute("name")) {
    REGEN_WARN("No name specified for " << input.getDescription() << ".");
    return ref_ptr<U>();
  }
  ref_ptr<U> v = ref_ptr<U>::alloc(input.getValue("name"));
  v->set_isConstant(input.getValue<bool>("is-constant", false));

  GLuint numInstances = input.getValue<GLuint>("num-instances",1u);
  GLuint numVertices  = input.getValue<GLuint>("num-vertices",1u);
  bool isInstanced    = input.getValue<bool>("is-instanced",false) && numInstances>1;
  bool isAttribute    = input.getValue<bool>("is-attribute",false) && numVertices>1;
  GLuint count=1;

  if(isInstanced) {
    v->setInstanceData(numInstances,1,NULL);
    count = numInstances;
  }
  else if(isAttribute) {
    v->setVertexData(numVertices,NULL);
    count = numVertices;
  }
  else {
    v->setUniformData(input.getValue<T>("value",defaultValue));
  }

  // Handle Attribute values.
  if(isInstanced || isAttribute) {
    T *values = (T*)v->clientDataPtr();
    for(GLuint i=0; i<count; i+=1) values[i] = defaultValue;

    const list< ref_ptr<SceneInputNode> > &childs = input.getChildren();
    for(list< ref_ptr<SceneInputNode> >::const_iterator
        it=childs.begin(); it!=childs.end(); ++it)
    {
      ref_ptr<SceneInputNode> child = *it;
      list<GLuint> indices = child->getIndexSequence(count);

      if(child->getCategory() == "set") {
        ValueGenerator<T> generator(child.get(),indices.size(),
            child->getValue<T>("value",T(0)));
        for(list<GLuint>::iterator it=indices.begin(); it!=indices.end(); ++it) {
          values[*it] += generator.next();
        }
      }
      else {
        REGEN_WARN("No processor registered for '" << child->getDescription() << "'.");
      }
    }
  }

  return v;
}

ref_ptr<ShaderInput> InputStateProvider::createShaderInput(
    SceneParser *parser, SceneInputNode &input)
{
  if(input.hasAttribute("state")) {
    // take uniform from state
    ref_ptr<State> state = parser->getState(input.getValue("state"));
    if(state.get()==NULL) {
      REGEN_WARN("No State found for for '" << input.getDescription() << "'.");
      return ref_ptr<ShaderInput>();
    }
    else {
      ref_ptr<ShaderInput> ret = state->findShaderInput(input.getValue("component"));
      if(ret.get()==NULL) {
        REGEN_WARN("No ShaderInput found for for '" << input.getDescription() << "'.");
      }
      return ret;
    }
  }

  const string type = input.getValue<string>("type", "");
  if(type == "int") {
    return createShaderInput_<ShaderInput1i,GLint>(input,GLint(0));
  }
  else if(type == "ivec2") {
    return createShaderInput_<ShaderInput2i,Vec2i>(input,Vec2i(0));
  }
  else if(type == "ivec3") {
    return createShaderInput_<ShaderInput3i,Vec3i>(input,Vec3i(0));
  }
  else if(type == "ivec4") {
    return createShaderInput_<ShaderInput4i,Vec4i>(input,Vec4i(0));
  }
  else if(type == "uint") {
    return createShaderInput_<ShaderInput1ui,GLuint>(input,GLuint(0));
  }
  else if(type == "uvec2") {
    return createShaderInput_<ShaderInput2ui,Vec2ui>(input,Vec2ui(0));
  }
  else if(type == "uvec3") {
    return createShaderInput_<ShaderInput3ui,Vec3ui>(input,Vec3ui(0));
  }
  else if(type == "uvec4") {
    return createShaderInput_<ShaderInput4ui,Vec4ui>(input,Vec4ui(0));
  }
  else if(type == "float") {
    return createShaderInput_<ShaderInput1f,GLfloat>(input,GLfloat(0));
  }
  else if(type == "vec2") {
    return createShaderInput_<ShaderInput2f,Vec2f>(input,Vec2f(0));
  }
  else if(type == "vec3") {
    return createShaderInput_<ShaderInput3f,Vec3f>(input,Vec3f(0));
  }
  else if(type == "vec4") {
    return createShaderInput_<ShaderInput4f,Vec4f>(input,Vec4f(0));
  }
  else if(type == "mat3") {
    return createShaderInput_<ShaderInputMat3,Mat3f>(input,Mat3f::identity());
  }
  else if(type == "mat4") {
    return createShaderInput_<ShaderInputMat4,Mat4f>(input,Mat4f::identity());
  }
  else {
    REGEN_WARN("Unknown input type '" << type << "'.");
    return ref_ptr<ShaderInput>();
  }
}

InputStateProvider::InputStateProvider()
: StateProcessor(REGEN_INPUT_STATE_CATEGORY)
{}

void InputStateProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<State> &state)
{
  ref_ptr<ShaderInput> in = createShaderInput(parser,input);
  if(in.get()==NULL) {
    REGEN_WARN("Failed to create input for " << input.getDescription() << ".");
    return;
  }

  ref_ptr<State> s = state;
  while(!s->joined().empty()) {
    s = *s->joined().rbegin();
  }
  HasInput *x = dynamic_cast<HasInput*>(s.get());

  if(x==NULL) {
    ref_ptr<HasInputState> inputState = ref_ptr<HasInputState>::alloc();
    inputState->setInput(in, input.getValue("name"));
    state->joinStates(inputState);
  }
  else {
    x->setInput(in, input.getValue("name"));
  }
}

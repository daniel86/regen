/*
 * debug-normal.cpp
 *
 *  Created on: 29.07.2012
 *      Author: daniel
 */

#include "debug-normal.h"

#include <ogle/shader/shader-manager.h>
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>

static const string createNormalVector =
"void createNormalVector(float length) {\n"
"    for(int i=0; i< gl_VerticesIn; i++) {\n"
"        vec4 posV = in_pos[i];\n"
"        vec3 norV = in_nor[i];\n"
"        gl_Position = posV; EmitVertex();\n"
"        gl_Position = posV + vec4(norV,0) * length; EmitVertex();\n"
"        EndPrimitive();\n"
"    }\n"
"}\n";

DebugNormal::DebugNormal(
    map<string, ref_ptr<ShaderInput> > &inputs,
    GeometryShaderInput inputPrimitive,
    GLfloat normalLength)
: ShaderState()
{
  ShaderFunctions fs, vs, gs;
  map<GLenum, string> stagesStr;
  map<GLenum, ShaderFunctions*> stages;

  GLuint numPrimitiveVertices;
  switch(inputPrimitive) {
  case GS_INPUT_POINTS:
    numPrimitiveVertices=1; break;
  case GS_INPUT_LINES:
    numPrimitiveVertices=2; break;
  case GS_INPUT_TRIANGLES:
    numPrimitiveVertices=3; break;
  case GS_INPUT_LINES_ADJACENCY:
    numPrimitiveVertices=2; break;
  case GS_INPUT_TRIANGLES_ADJACENCY:
    numPrimitiveVertices=3; break;
  }

  vs.enableExtension("GL_EXT_gpu_shader4");
  vs.setMinVersion(150);
  vs.addUniform(GLSLUniform(
      "mat4", "in_viewProjectionMatrix") );
  vs.addInput(GLSLTransfer(
      "vec4", "in_Position"));
  vs.addInput(GLSLTransfer(
      "vec3", "in_nor"));
  vs.addExport(GLSLExport(
      "out_pos", "in_Position") );
  vs.addExport(GLSLExport(
      "gl_Position", "in_Position") );
  vs.addExport(GLSLExport("out_nor",
      "normalize(in_viewProjectionMatrix * vec4(in_nor,0.0)).xyz" ) );
  vs.addOutput(GLSLTransfer(
      "vec3", "out_nor", 1, false, FRAGMENT_INTERPOLATION_SMOOTH ) );
  vs.addOutput(GLSLTransfer(
      "vec4", "out_pos", 1, false, FRAGMENT_INTERPOLATION_SMOOTH) );

  gs.setMinVersion(150);
  gs.enableExtension("GL_EXT_geometry_shader4");
  gs.addInput(GLSLTransfer(
      "vec3", "in_nor", numPrimitiveVertices, true, FRAGMENT_INTERPOLATION_SMOOTH ) );
  gs.addInput(GLSLTransfer(
      "vec4", "in_pos", numPrimitiveVertices, true, FRAGMENT_INTERPOLATION_SMOOTH ) );
  gs.addDependencyCode("createNormalVector", createNormalVector);
  gs.addStatement(GLSLStatement(
      FORMAT_STRING("createNormalVector(" << normalLength << ");")));
  GeometryShaderConfig gsConfig;
  gsConfig.input = inputPrimitive;
  gsConfig.output = GS_OUTPUT_LINE_STRIP;
  gsConfig.invocations = 1;
  gsConfig.maxVertices = numPrimitiveVertices*2;
  gs.set_gsConfig(gsConfig);

  fs.setMinVersion(150);
  fs.addExport(GLSLExport(
    "defaultColorOutput", "vec4(1.0, 1.0, 0.0, 1.0)" ));
  fs.addFragmentOutput(GLSLFragmentOutput(
    "vec4", "defaultColorOutput", GL_COLOR_ATTACHMENT0 ));

  stages[GL_FRAGMENT_SHADER] = &fs;
  stages[GL_VERTEX_SHADER] = &vs;
  stages[GL_GEOMETRY_SHADER] = &gs;
  ShaderManager::setupInputs(inputs, stages);

  stagesStr[GL_FRAGMENT_SHADER] =
      ShaderManager::generateSource(fs, GL_FRAGMENT_SHADER, GL_NONE);
  stagesStr[GL_VERTEX_SHADER] =
      ShaderManager::generateSource(vs, GL_VERTEX_SHADER, GL_GEOMETRY_SHADER);
  stagesStr[GL_GEOMETRY_SHADER] =
      ShaderManager::generateSource(gs, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER);

  shader_ = ref_ptr<Shader>::manage(new Shader(stagesStr));
  if(shader_->compile() && shader_->link())
  {
    ShaderManager::setupLocations(shader_, stages);
    shader_->setupInputs(inputs);
  }
}

string DebugNormal::name()
{
  return FORMAT_STRING("DebugNormal");
}

void DebugNormal::enable(RenderState *state)
{
  glDepthFunc(GL_LEQUAL);
  ShaderState::enable(state);
}

void DebugNormal::disable(RenderState *state)
{
  ShaderState::disable(state);
  glDepthFunc(GL_LESS);
}

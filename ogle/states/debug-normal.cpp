/*
 * debug-normal.cpp
 *
 *  Created on: 29.07.2012
 *      Author: daniel
 */

#include "debug-normal.h"

#include <ogle/shader/shader-manager.h>
#include <ogle/utility/string-util.h>

static const string createNormalVector =
"void createNormalVector(float length) {\n"
"    for(int i=0; i< gl_VerticesIn; i++) {\n"
"        vec4 pos = g_pos[i];\n"
"        vec3 nor = g_nor[i];\n"
"        gl_Position = posV; EmitVertex();\n"
"        gl_Position = posV + vec4(norV,0) * length; EmitVertex();\n"
"        EndPrimitive();\n"
"    }\n"
"}\n";

DebugNormalState::DebugNormalState(
    GeometryShaderInput inputPrimitive,
    GLfloat normalLength)
: ShaderState()
{
  ShaderFunctions fs, vs, gs;

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

  // feedback for pos in eye space
  //posAtt_ = ref_ptr<VertexAttribute>::manage(new VertexAttributefv( "Position", 4 ));
  //mesh_->addTransformFeedbackAttribute(posAtt_);
  // feedback for nor in world space
  //norAtt_ = ref_ptr<VertexAttribute>::manage(new VertexAttributefv( "nor" ));
  //mesh_->addTransformFeedbackAttribute(norAtt_);

  vs.enableExtension("GL_EXT_gpu_shader4");
  vs.setMinVersion(150);
  vs.addUniform(GLSLUniform(
      "mat4", "viewProjectionMatrix") );
  vs.addInput(GLSLTransfer(
      "vec4", "v_Position"));
  vs.addInput(GLSLTransfer(
      "vec3", "v_nor"));
  vs.addExport(GLSLExport(
      "g_pos", "v_Position") );
  vs.addExport(GLSLExport(
      "gl_Position", "v_Position") );
  vs.addExport(GLSLExport("g_nor",
      "normalize(viewProjectionMatrix * vec4(v_nor,0.0)).xyz" ) );
  vs.addOutput(GLSLTransfer(
      "vec3", "g_nor", 1, false, "smooth" ) );
  vs.addOutput(GLSLTransfer(
      "vec4", "g_pos", 1, false, "smooth") );

  gs.setMinVersion(150);
  gs.enableExtension("GL_EXT_geometry_shader4");
  gs.addInput(GLSLTransfer(
      "vec3", "g_nor", numPrimitiveVertices, true, "smooth" ) );
  gs.addInput(GLSLTransfer(
      "vec4", "g_pos", numPrimitiveVertices, true, "smooth" ) );
  gs.addDependencyCode("createNormalVector", createNormalVector);
  gs.addStatement(GLSLStatement(
      FORMAT_STRING("createNormalVector(" << normalLength << ")")));
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

  shader_ = ref_ptr<Shader>::manage(new Shader);
  map<GLenum, string> stages;
  stages[GL_FRAGMENT_SHADER] =
      ShaderManager::generateSource(fs, GL_FRAGMENT_SHADER);
  stages[GL_VERTEX_SHADER] =
      ShaderManager::generateSource(vs, GL_VERTEX_SHADER);
  stages[GL_GEOMETRY_SHADER] =
      ShaderManager::generateSource(gs, GL_GEOMETRY_SHADER);
  if(shader_->compile(stages) && shader_->link())
  {
    set<string> attributeNames, uniformNames;
    for(set<GLSLTransfer>::const_iterator
        it=vs.inputs().begin(); it!=vs.inputs().end(); ++it)
    {
      attributeNames.insert(it->name);
    }
    for(set<GLSLUniform>::const_iterator
        it=vs.uniforms().begin(); it!=vs.uniforms().end(); ++it)
    {
      uniformNames.insert(it->name);
    }
    for(set<GLSLUniform>::const_iterator
        it=fs.uniforms().begin(); it!=fs.uniforms().end(); ++it)
    {
      uniformNames.insert(it->name);
    }
    for(set<GLSLUniform>::const_iterator
        it=gs.uniforms().begin(); it!=gs.uniforms().end(); ++it)
    {
      uniformNames.insert(it->name);
    }
    shader_->setupLocations(attributeNames, uniformNames);
  }
}

void DebugNormalState::enable(RenderState *state)
{
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  ShaderState::enable(state);
}

void DebugNormalState::disable(RenderState *state)
{
  ShaderState::disable(state);
  glDepthFunc(GL_LESS);
  glDisable(GL_DEPTH_TEST);
}

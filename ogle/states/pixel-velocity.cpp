/*
 * pixel-velocity.cpp
 *
 *  Created on: 18.07.2012
 *      Author: daniel
 */

#include "pixel-velocity.h"

#include <ogle/utility/string-util.h>
#include <ogle/shader/shader-manager.h>

PixelVelocity::PixelVelocity(
    CoordinateSpace velocitySpace,
    GLboolean useDepthTestFS,
    GLfloat depthBias)
: ShaderState(),
  velocitySpace_(velocitySpace)
{
  ShaderFunctions fs, vs;

  // VS: just transfers last and current position to FS
  vs.addOutput(GLSLTransfer("vec2", "f_pos0"));
  vs.addOutput(GLSLTransfer("vec2", "f_pos1"));
  switch(velocitySpace_) {
  case WORLD_SPACE:
    vs.addInput(GLSLTransfer("vec4", "v_posWorld"));
    vs.addInput(GLSLTransfer("vec4", "v_lastPosWorld"));
    vs.addExport(GLSLExport("f_pos0", "v_posWorld.xy"));
    vs.addExport(GLSLExport("f_pos1", "v_lastPosWorld.xy"));
    vs.addExport(GLSLExport("gl_Position", "viewProjectionMatrix * v_posWorld"));
    break;
  case EYE_SPACE:
    vs.addInput(GLSLTransfer("vec4", "v_posEye"));
    vs.addInput(GLSLTransfer("vec4", "v_lastPosEye"));
    vs.addExport(GLSLExport("f_pos0", "v_posEye.xy"));
    vs.addExport(GLSLExport("f_pos1", "v_lastPosEye.xy"));
    vs.addExport(GLSLExport("gl_Position", "projectionMatrix * v_posEye"));
    break;
  case SCREEN_SPACE:
    vs.addInput(GLSLTransfer("vec4", "v_Position"));
    vs.addInput(GLSLTransfer("vec4", "v_lastPosition"));
    vs.addExport(GLSLExport("f_pos0", "v_Position.xy"));
    vs.addExport(GLSLExport("f_pos1", "v_lastPosition.xy"));
    vs.addExport(GLSLExport("gl_Position", "v_Position"));
    break;
  }

  // FS: calculates xy-velocity.
  // optional depth test against scene depth buffer
  fs.addUniform(GLSLUniform("uvec2", "viewport"));
  fs.addInput(GLSLTransfer("vec2", "f_pos0"));
  fs.addInput(GLSLTransfer("vec2", "f_pos1"));
  // output the velocity in velocitySpace_ units per second
  fs.addFragmentOutput(GLSLFragmentOutput(
      "vec2", "velocityOutput", GL_COLOR_ATTACHMENT0));
  if(useDepthTestFS)
  {
    fs.addUniform(GLSLUniform("sampler2D", "depthTexture"));
    // bias used for scene depth buffer access
    fs.addMainVar(GLSLVariable(
        "const float", "depthBias", FORMAT_STRING(depthBias)));
    // use the fragment coordinate to find the texture coordinates of
    // this fragment in the scene depth buffer.
    // gl_FragCoord.xy is in window space, divided by the buffer size
    // we get the range [0,1] that can be used for texture access.
    fs.addMainVar(GLSLVariable(
        "vec2", "depthTexco", "vec2(gl_FragCoord.x/viewport.x, gl_FragCoord.y/viewport.y)"));
    // depth at this pixel obtained in main pass.
    // this is non linear depth in the range [0,1].
    fs.addMainVar(GLSLVariable(
        "float", "sceneDepth", "texture(depthTexture, depthTexco).r"));
    // do the depth test against gl_FragCoord.z using a bias.
    // bias is used to avoid flickering
    fs.addStatement(GLSLStatement(
        "if( gl_FragCoord.z > sceneDepth+depthBias ) { discard; }"));
  }
  fs.addExport(GLSLExport(
      "velocityOutput", "(f_pos0 - f_pos1)/deltaT" ));

  shader_ = ref_ptr<Shader>::manage(new Shader);
  map<GLenum, string> stages;
  stages[GL_FRAGMENT_SHADER] =
      ShaderManager::generateSource(fs, GL_FRAGMENT_SHADER);
  stages[GL_VERTEX_SHADER] =
      ShaderManager::generateSource(vs, GL_VERTEX_SHADER);
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
    shader_->setupLocations(attributeNames, uniformNames);
  }
}

string PixelVelocity::name()
{
  return FORMAT_STRING("PixelVelocity");
}

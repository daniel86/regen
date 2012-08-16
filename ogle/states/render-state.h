/*
 * render-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef RENDER_STATE_H_
#define RENDER_STATE_H_

#include <stack>
#include <map>

#include <ogle/gl-types/vbo.h>
#include <ogle/gl-types/vertex-attribute.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/shader.h>
#include <ogle/gl-types/texture.h>

#include <ogle/utility/stack.h>

class RenderState
{
public:
  RenderState();
  ~RenderState();

  Stack<VertexBufferObject*> vbos;
  Stack<FrameBufferObject*> fbos;
  Stack<Shader*> shaders;
  set< Stack<ShaderTexture>* > activeTextures;

  void pushShaderInput(ShaderInput *att);
  void popShaderInput(const string &name);
  GLuint numInstances() const;

  void pushVBO(VertexBufferObject *vbo);
  void popVBO();

  void pushFBO(FrameBufferObject *tex);
  void popFBO();

  void pushShader(Shader *tex);
  void popShader();

  void pushTexture(GLuint unit, Texture *tex);
  void popTexture(GLuint unit);

  GLuint nextTextureUnit();
  void releaseTextureUnit();

protected:
  Stack<ShaderTexture> *textureArray;
  GLint maxTextureUnits_;
  GLint textureCounter_;

  map< string, Stack<ShaderInput*> > inputs_;
  list<ShaderInput*> uniforms_;
  list<ShaderInput*> attributes_;
  list<ShaderInput*> constants_;
};

#endif /* RENDER_STATE_H_ */

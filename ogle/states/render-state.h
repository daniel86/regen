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

class RenderState
{
public:
  RenderState();
  ~RenderState();

  Stack<VertexBufferObject*> vbos;
  Stack<FrameBufferObject*> fbos;
  Stack<Shader*> shaders;
  set< Stack<ShaderTexture>* > activeTextures;
  map< string, Stack<VertexAttribute*> > attributes;
  list<Uniform*> uniforms;

  void pushUniform(Uniform *u);
  void popUniform();

  void pushAttribute(VertexAttribute *att);
  void popAttribute(const string &name);

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
  GLuint textureCounter_;
};

#endif /* RENDER_STATE_H_ */

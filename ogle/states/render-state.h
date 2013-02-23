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

class StateNode;
class State;
class MeshState;
class TextureState;

class RenderState
{
public:
  RenderState();
  virtual ~RenderState();

  void set_isDepthTestEnabled(GLboolean v);
  GLboolean isDepthTestEnabled();

  void set_isDepthWriteEnabled(GLboolean v);
  GLboolean isDepthWriteEnabled();

  virtual GLboolean isNodeHidden(StateNode *node);
  virtual GLboolean isStateHidden(State *state);

  virtual GLboolean useTransformFeedback() const;
  virtual void set_useTransformFeedback(GLboolean);

  Stack<FrameBufferObject*> fbos;
  Stack<Shader*> shaders;

  virtual void pushMesh(MeshState *mesh);
  virtual void popMesh();

  virtual void pushShaderInput(ShaderInput *att);
  virtual void popShaderInput(const string &name);
  virtual GLuint numInstances() const;

  virtual void pushFBO(FrameBufferObject *tex);
  virtual void popFBO();

  virtual void pushShader(Shader *tex);
  virtual void popShader();

  virtual void pushTexture(TextureState *tex);
  virtual void popTexture(GLuint channel);

  virtual GLuint nextTexChannel();
  virtual void releaseTexChannel();

  virtual void set_bones(GLuint numWeights, GLuint numBones);
  virtual GLuint boneWeightCount() { return boneWeightCount_; }
  virtual GLuint boneCount() { return boneCount_; }

  virtual void set_ignoreViewRotation(GLboolean v) {}
  virtual GLboolean ignoreViewRotation() { return ignoreViewRotation_; }
  virtual void set_ignoreViewTranslation(GLboolean v) {}
  virtual GLboolean ignoreViewTranslation() { return ignoreViewTranslation_; }

  virtual void set_useTesselation(GLboolean v) {}
  virtual GLboolean useTesselation() { return useTesselation_; }

protected:
  static GLint maxTextureUnits_;

  GLboolean isDepthTestEnabled_;
  GLboolean isDepthWriteEnabled_;

  Stack< TextureState* > *textureArray;
  GLint textureCounter_;

  GLboolean useTransformFeedback_;

  map< string, Stack<ShaderInput*> > inputs_;
  list<ShaderInput*> uniforms_;
  list<ShaderInput*> attributes_;
  list<ShaderInput*> constants_;

  GLuint boneWeightCount_;
  GLuint boneCount_;
  GLboolean ignoreViewRotation_;
  GLboolean ignoreViewTranslation_;
  GLboolean useTesselation_;
};

#endif /* RENDER_STATE_H_ */

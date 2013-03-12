/*
 * bones.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef __BONES__H_
#define __BONES__H_

#include <ogle/states/state.h>
#include <ogle/animations/animation-node.h>
#include <ogle/gl-types/tbo.h>
#include <ogle/gl-types/vbo.h>
#include <ogle/states/texture-state.h>

namespace ogle {
/**
 * \brief Provides bone matrices.
 *
 * The data is provided to Shader's using a TBO.
 */
class Bones : public State, public Animation
{
public:
  Bones(list< ref_ptr<AnimationNode> > &bones, GLuint numBoneWeights);
  ~Bones();

  /**
   * @return maximum number of weights influencing a single bone.
   */
  GLint numBoneWeights() const;

  // override
  void animate(GLdouble dt);
  void glAnimate(RenderState *rs, GLdouble dt);
  GLboolean useAnimation() const;
  GLboolean useGLAnimation() const;

protected:
  list< ref_ptr<AnimationNode> > bones_;
  ref_ptr<ShaderInput1i> numBoneWeights_;

  ref_ptr<VertexBufferObject> boneMatrixVBO_;
  Mat4f *boneMatrixData_;

  GLuint lastBoneWeights_;
  GLuint lastBoneCount_;
};
} // namespace

#endif /* __BONES__H_ */

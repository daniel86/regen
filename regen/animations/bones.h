/*
 * bones.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef __BONES__H_
#define __BONES__H_

#include <regen/states/state.h>
#include <regen/animations/animation-node.h>
#include <regen/gl-types/tbo.h>
#include <regen/gl-types/vbo.h>
#include <regen/states/texture-state.h>

namespace regen {
  /**
   * \brief Provides bone matrices.
   *
   * The data is provided to Shader's using a TBO.
   */
  class Bones : public State, public Animation
  {
  public:
    /**
     * @param bones list of bone nodes.
     * @param numBoneWeights maximum number of bone weights.
     */
    Bones(list< ref_ptr<AnimationNode> > &bones, GLuint numBoneWeights);
    ~Bones();

    /**
     * @return maximum number of weights influencing a single bone.
     */
    GLint numBoneWeights() const;

    // override
    void glAnimate(RenderState *rs, GLdouble dt);

  protected:
    list< ref_ptr<AnimationNode> > bones_;
    ref_ptr<ShaderInput1i> numBoneWeights_;
    GLuint bufferSize_;

    ref_ptr<TextureBufferObject> boneMatrixTex_;
    VBOReference vboRef_;
    Mat4f *boneMatrixData_;

    GLuint lastBoneWeights_;
    GLuint lastBoneCount_;
  };
} // namespace

#endif /* __BONES__H_ */

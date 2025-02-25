/*
 * bones.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef REGEN_BONES_H
#define REGEN_BONES_H

#include <regen/states/state.h>
#include <regen/animations/animation-node.h>
#include <regen/gl-types/vbo.h>
#include <regen/textures/texture-state.h>

namespace regen {
	/**
	 * \brief Provides bone matrices.
	 *
	 * The data is provided to Shader's using a TextureBuffer.
	 */
	class Bones : public HasInputState, public Animation {
	public:
		/**
		 * @param numBoneWeights maximum number of bone weights.
		 * @param numBones number of bones per mesh.
		 */
		Bones(GLuint numBoneWeights, GLuint numBones);

		/**
		 * @param bones  the bone list
		 */
		void setBones(const std::list<ref_ptr<AnimationNode> > &bones);

		/**
		 * @return maximum number of weights influencing a single bone.
		 */
		auto numBoneWeights() const  { return numBoneWeights_->getVertex(0); }

		// override
		void animate(GLdouble dt) override;

		// override
		void glAnimate(RenderState *rs, GLdouble dt) override;

	protected:
		std::list<ref_ptr<AnimationNode> > bones_;
		ref_ptr<ShaderInput1i> numBoneWeights_;
		GLuint bufferSize_;

		ref_ptr<TextureBuffer> boneMatrixTex_;
		ref_ptr<TextureState> texState_;
		VBOReference vboRef_;
		ref_ptr<ShaderInputMat4> boneMatrices_;
	};
} // namespace

#endif /* REGEN_BONES_H */

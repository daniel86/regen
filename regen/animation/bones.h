#ifndef REGEN_BONES_H
#define REGEN_BONES_H

#include <regen/states/state.h>
#include <regen/animation/bone-tree.h>
#include <regen/buffer/vbo.h>
#include <regen/textures/texture-state.h>
#include <regen/textures/texture-buffer.h>

namespace regen {
	/**
	 * \brief Provides bone matrices.
	 *
	 * The data is provided to Shader's using a TextureBuffer.
	 */
	class Bones : public State, public Animation {
	public:
		Bones(const ref_ptr<BoneTree> &tree,
			const std::vector<ref_ptr<BoneNode>> &boneNodes,
			uint32_t numBoneWeights);

		/**
		 * @return maximum number of weights influencing a single bone.
		 */
		auto numBoneWeights() const  { return numBoneWeights_->getVertex(0); }

		// override
		void cpuUpdate(double dt) override;

	protected:
		ref_ptr<BoneTree> boneTree_;
		std::vector<ref_ptr<BoneNode>> boneNodes_;

		ref_ptr<ShaderInput1i> numBoneWeights_;
		uint32_t numInstances_;
		uint32_t bufferSize_;

		ref_ptr<TBO> boneMatrixTBO_;
		ref_ptr<TextureState> texState_;
		ref_ptr<ShaderInputMat4> boneMatrices_;
	};
} // namespace

#endif /* REGEN_BONES_H */

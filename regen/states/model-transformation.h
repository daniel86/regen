#ifndef REGEN_MODEL_TRANSFORMATION_H_
#define REGEN_MODEL_TRANSFORMATION_H_

#include <regen/av/audio.h>
#include <regen/math/quaternion.h>
#include <regen/states/state.h>
#include "buffer-container.h"

namespace regen {
	/**
	 * \brief matrix that transforms for model space to world space.
	 *
	 * Usually meshes should be defined at origin and then translated
	 * and rotated to the world position.
	 */
	class ModelTransformation : public State {
	public:
		static constexpr const char *TYPE_NAME = "ModelTransformation";

		/**
		 * @brief Flags for model transformation.
		 *
		 * These flags are used to determine which transformation
		 * is used for the model.
		 */
		enum ModeFlag {
			TF_MATRIX = 1 << 0, ///< Use model matrix.
			TF_OFFSET = 1 << 1, ///< Use model offset.
		};

		/**
		 * @brief Default constructor.
		 *
		 * Creates a model transformation with the default mode TF_MATRIX.
		 */
		explicit ModelTransformation(
				int tfMode = TF_MATRIX,
				const BufferUpdateFlags &tfUpdateFlags = BufferUpdateFlags::FULL_PER_FRAME);

		/**
		 * @brief Constructor with model offset.
		 *
		 * Creates a model transformation with TF_OFFSET mode.
		 */
		explicit ModelTransformation(
				const ref_ptr<ShaderInput4f> &modelOffset,
				const BufferUpdateFlags &tfUpdateFlags = BufferUpdateFlags::FULL_PER_FRAME);

		/**
		 * @brief Constructor with model matrix.
		 *
		 * Creates a model transformation with TF_MATRIX mode.
		 */
		explicit ModelTransformation(
				const ref_ptr<ShaderInputMat4> &modelMat,
				const BufferUpdateFlags &tfUpdateFlags = BufferUpdateFlags::FULL_PER_FRAME);

		/**
		 * @return The stamp of the model transformation.
		 */
		inline uint32_t stamp() const { return tfStamp_; }

		/**
		 * @return The buffer object that contains the model transformation matrix.
		 */
		const ref_ptr<BufferContainer>& tfBuffer() const { return tfBuffer_; }

		/**
		 * @return true if the model transformation has a model matrix.
		 */
		bool hasModelMat() const { return (tfMode_ & TF_MATRIX) != 0; }

		/**
		 * Get a vector of most recent model matrices used to transform
		 * the model from model space to world space.
		 * @return the model matrices.
		 */
		const std::vector<Mat4f> &modelMat() const { return modelMat_; }

		/**
		 * Get a writable pointer to the model matrix vector.
		 * This is used to write the thread-local model matrices directly.
		 * @return a writable pointer to the model matrix vector.
		 */
		Mat4f* modelMatMapWrite();

		/**
		 * Get the model matrix for a specific layer.
		 * @param idx the layer index.
		 * @return the model matrix position the specified layer.
		 */
		const Mat4f &modelMat(uint32_t idx) const { return modelMat_[idx]; }

		/**
		 * Get the shader input for the model matrix.
		 * @return the shader input for the model matrix.
		 */
		const ref_ptr<ShaderInputMat4> &sh_modelMat() const { return sh_modelMat_; }

		/**
		 * Set the model matrix for a specific layer.
		 * @param idx the layer index.
		 * @param mat the model matrix to set.
		 */
		void setModelMat(uint32_t idx, const Mat4f &mat) {
			setStamped(modelMat_, modelMatStamp_, idx, mat);
		}

		/**
		 * Set the model matrix for all layers.
		 * @param mat the model matrix to set.
		 */
		void setModelMat(const Mat4f *mat);

		/**
		 * Resize the model matrix vector to accommodate a new number of instances.
		 * @param numInstances the new number of instances.
		 */
		void resizeModelMat(uint32_t numInstances, const Mat4f *initialData=nullptr);

		/**
		 * @return the stamp indicating when the model matrix was last updated.
		 */
		uint32_t modelMatStamp() const { return modelMatStamp_; }

		/**
		 * @return true if the model transformation has a model offset.
		 */
		bool hasModelOffset() const { return (tfMode_ & TF_OFFSET) != 0; }

		/**
		 * Get a vector of model offsets used to transform
		 * the model from model space to world space.
		 * @return the model offsets.
		 */
		const std::vector<Vec4f> &modelOffset() const { return modelOffset_; }

		/**
		 * Get a writable pointer to the model offset vector.
		 * This is used to write the thread-local model offsets directly.
		 * @return a writable pointer to the model offset vector.
		 */
		Vec4f* modelOffsetMapWrite();

		/**
		 * Get the model offset for a specific layer.
		 * @param idx the layer index.
		 * @return the model offset for the specified layer.
		 */
		const Vec4f &modelOffset(uint32_t idx) const { return modelOffset_[idx]; }

		/**
		 * Get the shader input for the model offset.
		 * @return the shader input for the model offset.
		 */
		const ref_ptr<ShaderInput4f> &sh_modelOffset() const { return sh_modelOffset_; }

		/**
		 * Set the model offset for a specific layer.
		 * @param idx the layer index.
		 * @param offset the model offset to set.
		 */
		void setModelOffset(uint32_t idx, const Vec3f &offset) {
			setStamped3(modelOffset_, modelOffsetStamp_, idx, offset);
		}

		/**
		 * Resize the model offset vector to accommodate a new number of instances.
		 * @param numInstances the new number of instances.
		 */
		void resizeModelOffset(uint32_t numInstances, const Vec4f *initialData=nullptr);

		/**
		 * @return the stamp indicating when the model offset was last updated.
		 */
		uint32_t modelOffsetStamp() const { return modelOffsetStamp_; }

		const Vec3f &position(uint32_t idx) const;

		/**
		 * Get a vector of most recent velocities.
		 * @return the velocities.
		 */
		const std::vector<Vec3f> &velocity() const { return velocity_; }

		/**
		 * Get the velocity for a specific layer.
		 * @param idx the layer index.
		 * @return the velocity for the specified layer.
		 */
		const Vec3f &velocity(uint32_t idx) const { return velocity_[idx]; }

		/**
		 * Set the velocity for a specific layer.
		 * @param idx the layer index.
		 * @param vel the velocity to set.
		 */
		void setVelocity(uint32_t idx, const Vec3f &vel) {
			setStamped(velocity_, velocityStamp_, idx, vel);
		}

		/**
		 * @return the stamp indicating when the velocity was last updated.
		 */
		uint32_t velocityStamp() const { return velocityStamp_; }

		/**
		 * @param audioSource the audio source attached to the world position
		 * of the model.
		 */
		void setAudioSource(const ref_ptr<AudioSource> &audioSource) { audioSource_ = audioSource; }

		/**
		 * @return the audio source attached to the world position
		 * of the model.
		 */
		bool isAudioSource() const { return audioSource_.get() != nullptr; }

		// Override
		void enable(RenderState *rs) override;

		void updateShaderData();

		static ref_ptr<ModelTransformation> load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<State> &state);

	protected:
		int tfMode_;
		BufferUpdateFlags tfUpdateFlags_;

		ref_ptr<BufferContainer> tfBuffer_;
		ref_ptr<ShaderInputMat4> sh_modelMat_;
		ref_ptr<ShaderInput4f> sh_modelOffset_;
		ref_ptr<ShaderInput3f> sh_velocity_;

		std::vector<Mat4f> modelMat_;
		std::vector<Vec4f> modelOffset_;
		std::vector<Vec3f> velocity_;
		mutable Vec3f tmpPos_ = Vec3f::zero();

		uint32_t modelMatStamp_ = 1;
		uint32_t modelOffsetStamp_ = 1;
		uint32_t velocityStamp_ = 1;
		uint32_t tfStamp_ = 1;

		uint32_t lastModelMatStamp_ = 0;
		uint32_t lastModelOffsetStamp_ = 0;
		uint32_t lastVelocityStamp_ = 0;

		ref_ptr<AudioSource> audioSource_;
		boost::posix_time::ptime lastTime_ = boost::posix_time::microsec_clock::local_time();

		void initBufferContainer();

		template<typename T>
		inline void setStamped(
				std::vector<T> &vec, uint32_t &stamp, uint32_t idx, const T &value) {
			vec[idx] = value;
			stamp += 1;
			tfStamp_ += 1;
		}

		inline void setStamped3(
				std::vector<Vec4f> &vec, uint32_t &stamp, uint32_t idx, const Vec3f &value) {
			vec[idx].xyz_() = value;
			stamp += 1;
			tfStamp_ += 1;
		}
	};
} // namespace

#endif /* REGEN_MODEL_TRANSFORMATION_H_ */

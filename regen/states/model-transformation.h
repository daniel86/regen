/*
 * model-transformation.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef MODEL_TRANSFORMATION_H_
#define MODEL_TRANSFORMATION_H_

#include <regen/av/audio.h>
#include <regen/math/quaternion.h>
#include <regen/gl-types/input-container.h>
#include <regen/states/state.h>
#include "buffer-container.h"

namespace regen {
	struct PositionReader;
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
		explicit ModelTransformation(int tfMode = TF_MATRIX);

		/**
		 * @brief Constructor with model offset.
		 *
		 * Creates a model transformation with TF_OFFSET mode.
		 */
		explicit ModelTransformation(const ref_ptr<ShaderInput4f> &modelOffset);

		/**
		 * @brief Constructor with model matrix.
		 *
		 * Creates a model transformation with TF_MATRIX mode.
		 */
		explicit ModelTransformation(const ref_ptr<ShaderInputMat4> &modelMat);

		/**
		 * @return true if the model transformation has a model matrix.
		 */
		bool hasModelMat() const { return (tfMode_ & TF_MATRIX) != 0; }

		/**
		 * @return true if the model transformation has a model offset.
		 */
		bool hasModelOffset() const { return (tfMode_ & TF_OFFSET) != 0; }

		/**
		 * @return the number of instances of this model transformation.
		 */
		uint32_t numInstances() const;

		/**
		 * @return The stamp of the model transformation.
		 */
		uint32_t stamp() const;

		/**
		 * @return the model transformation matrix.
		 */
		auto &modelMat() const { return modelMat_; }

		/**
		 * @return the model offset.
		 */
		const ref_ptr<ShaderInput4f> &modelOffset() const { return modelOffset_; }

		/**
		 * @return the model velocity.
		 */
		auto &velocity() const { return velocity_; }

		/**
		 * @return The buffer object that contains the model transformation matrix.
		 */
		auto& bufferContainer() const { return bufferContainer_; }

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

		/**
		 * Maps position from the model matrix and/or model offset
		 * @param idx the index of the vertex to read
		 * @return a PositionReader that can be used to read the position
		 */
		PositionReader position(uint32_t idx) const;

		static ref_ptr<ModelTransformation> load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<State> &state);

	protected:
		int tfMode_;
		ref_ptr<ShaderInputMat4> modelMat_;
		ref_ptr<ShaderInput4f> modelOffset_;
		ref_ptr<ShaderInput3f> velocity_;
		ref_ptr<BufferContainer> bufferContainer_;

		ref_ptr<AudioSource> audioSource_;

		Vec3f lastPosition_ = Vec3f(0.0f, 0.0f, 0.0f);
		boost::posix_time::ptime lastTime_ = boost::posix_time::microsec_clock::local_time();

		void initBufferContainer();

		friend struct PositionReader;
		mutable Vec3f tmpPos_ = Vec3f::zero();
	};

	/**
	 * \brief A helper class to read positions from a ModelTransformation.
	 *
	 * This class provides a way to read the position of a vertex
	 * from the model matrix and model offset while avoiding a copy.
	 */
	struct PositionReader {
		PositionReader(const ModelTransformation *tf, unsigned int vertexIndex)
				: rawData_mat(getModelMat(tf), ShaderData::READ | ShaderData::INDEX),
				  rawData_offset(getModelOffset(tf), ShaderData::READ | ShaderData::INDEX),
				  r(getPositionReference(tf, vertexIndex)) {
		}
		PositionReader() :
				rawData_mat(nullptr, ShaderData::READ | ShaderData::INDEX),
				rawData_offset(nullptr, ShaderData::READ | ShaderData::INDEX),
				r(Vec3f::zero()) {
		}

		// do not allow copying
		PositionReader(const PositionReader &) = delete;

		static ShaderInput* getModelMat(const ModelTransformation *tf);

		static ShaderInput* getModelOffset(const ModelTransformation *tf);

		const Vec3f& getPositionReference(const ModelTransformation *tf, unsigned int vertexIndex) const;

		/**
		 * Unmap the data. Do not read after calling this method.
		 */
		void unmap() {
			rawData_mat.unmap();
			rawData_offset.unmap();
		}

	private:
		ShaderDataRaw_ro rawData_mat;
		ShaderDataRaw_ro rawData_offset;
	public:
		/**
		 * The mapped data for reading.
		 */
		const Vec3f &r;
	};
} // namespace

#endif /* MODEL_TRANSFORMATION_H_ */

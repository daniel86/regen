#ifndef REGEN_POSITION_READER_H_
#define REGEN_POSITION_READER_H_

#include <regen/buffer/client-buffer.h>
#include <regen/shader/shader-input.h>

namespace regen {
	/**
	 * \brief A helper class to read positions from a ModelTransformation.
	 *
	 * This class provides a way to read the position of a vertex
	 * from the model matrix and model offset while avoiding a copy.
	 */
	struct PositionReader {
		static ClientBuffer* getClientBuffer(ShaderInput *input) {
			return input ? input->clientBuffer().get() : nullptr;
		}

		PositionReader(ShaderInput *modelMat, ShaderInput *modelOffset, unsigned int vertexIndex)
				: rawData_mat(getClientBuffer(modelMat), BUFFER_GPU_READ),
				  rawData_offset(getClientBuffer(modelOffset), BUFFER_GPU_READ),
				  r(getPositionReference(modelMat, modelOffset, vertexIndex)) {
		}
		PositionReader() :
				rawData_mat(nullptr, BUFFER_GPU_READ),
				rawData_offset(nullptr, BUFFER_GPU_READ),
				r(Vec3f::zero()) {
		}

		// do not allow copying
		PositionReader(const PositionReader &) = delete;

		const Vec3f& getPositionReference(
				const ShaderInput *modelMatrix,
				const ShaderInput *modelOffset,
				unsigned int vertexIndex) const {
			if (modelOffset && modelMatrix) {
				tmpPos_ =
					((const Mat4f *) rawData_mat.r)[vertexIndex].position() +
					((const Vec4f *) rawData_offset.r)[vertexIndex].xyz();
				return tmpPos_;
			}
			if (modelOffset) {
				return ((const Vec4f *) rawData_offset.r)[vertexIndex].xyz();
			}
			if (modelMatrix) {
				return ((const Mat4f *) rawData_mat.r)[vertexIndex].position();
			}
			return Vec3f::zero();
		}

		/**
		 * Unmap the data. Do not read after calling this method.
		 */
		void unmap() {
			rawData_mat.unmap();
			rawData_offset.unmap();
		}

	private:
		ClientDataRaw_ro rawData_mat;
		ClientDataRaw_ro rawData_offset;
		mutable Vec3f tmpPos_;
	public:
		/**
		 * The mapped data for reading.
		 */
		const Vec3f &r;
	};
} // namespace

#endif /* REGEN_POSITION_READER_H_ */

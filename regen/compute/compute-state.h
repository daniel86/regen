#ifndef REGEN_COMPUTATION_STATE_H_
#define REGEN_COMPUTATION_STATE_H_

#include <regen/scene/state.h>

namespace regen {
	/**
	 * \brief Compute shader state.
	 */
	class ComputeState : public State {
	public:
		enum BarrierBit {
			SSBO_BIT = GL_SHADER_STORAGE_BARRIER_BIT,
			TBO_BIT = GL_TEXTURE_FETCH_BARRIER_BIT,
			UBO_BIT = GL_UNIFORM_BARRIER_BIT,
			ATOMIC_COUNTER_BIT = GL_ATOMIC_COUNTER_BARRIER_BIT,
			ALL_BARRIER_BITS = GL_ALL_BARRIER_BITS
		};

		ComputeState();

		ref_ptr<ComputeState> load(LoadingContext &ctx, scene::SceneInputNode &input);

		void setNumWorkUnits(uint32_t x, uint32_t y, uint32_t z);

		void setGroupSize(uint32_t x, uint32_t y, uint32_t z);

		void setBarrierBits(int bits) { barrierBits_ = bits; }

		void addBarrierBit(BarrierBit bit) { barrierBits_ |= bit; }

		Vec3ui numWorkGroups() const {
			return Vec3ui(
				static_cast<uint32_t>(numWorkGroups_.x),
				static_cast<uint32_t>(numWorkGroups_.y),
				static_cast<uint32_t>(numWorkGroups_.z));
		}

		Vec3ui workGroupSize() const {
			return Vec3ui(
				static_cast<uint32_t>(localSize_.x),
				static_cast<uint32_t>(localSize_.y),
				static_cast<uint32_t>(localSize_.z));
		}

		void dispatch();

	protected:
		// the number of invocations per work group
		struct {
			uint32_t x = 256u;
			uint32_t y = 1u;
			uint32_t z = 1u;
		} localSize_;
		struct {
			uint32_t x = 1u;
			uint32_t y = 1u;
			uint32_t z = 1u;
		} numWorkUnits_;
		// the number of work group invocations
		struct {
			uint32_t x = 0u;
			uint32_t y = 0u;
			uint32_t z = 0u;
		} numWorkGroups_;
		int barrierBits_ = SSBO_BIT;

		void updateNumWorkGroups();
	};
} // namespace

#endif /* REGEN_COMPUTATION_STATE_H_ */

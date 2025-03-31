#ifndef REGEN_COMPUTATION_STATE_H_
#define REGEN_COMPUTATION_STATE_H_

#include <regen/states/state.h>

namespace regen {
	/**
	 * \brief Compute shader state.
	 */
	class ComputeState : public State {
	public:
		ComputeState();

		ref_ptr<ComputeState> load(LoadingContext &ctx, scene::SceneInputNode &input);

		void setNumWorkUnits(int x, int y, int z);

		void setGroupSize(int x, int y, int z);

		// override
		void enable(RenderState *state) override;

	protected:
		// the number of invocations per work group
		struct {
			int x = 256;
			int y = 1;
			int z = 1;
		} localSize_;
		struct {
			int x = 1;
			int y = 1;
			int z = 1;
		} numWorkUnits_;
		// the number of work group invocations
		struct {
			int x = 0;
			int y = 0;
			int z = 0;
		} numWorkGroups_;

		void updateNumWorkGroups();
	};
} // namespace

#endif /* REGEN_COMPUTATION_STATE_H_ */

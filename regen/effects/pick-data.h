#ifndef REGE_PICK_DATA_H_
#define REGE_PICK_DATA_H_

namespace regen {
	/**
	 * The result of a pick operation.
	 */
	struct PickData {
		int objectID = 0;
		int instanceID = 0;
		float depth = 0.0f;
	};
} // namespace

#endif /* REGE_PICK_DATA_H_ */

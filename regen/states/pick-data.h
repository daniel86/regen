#ifndef REGE_PICK_DATA_H_
#define REGE_PICK_DATA_H_

namespace regen {
	/**
	 * The result of a pick operation.
	 */
	struct PickData {
		int objectID;
		int instanceID;
		float depth;
	};
} // namespace

#endif /* REGE_PICK_DATA_H_ */

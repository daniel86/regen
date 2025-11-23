#ifndef REGEN_DRAW_COMMAND_H_
#define REGEN_DRAW_COMMAND_H_

#include <cstdint>

namespace regen {
	/**
	 * This is used to unify the following two structures:
	 *     struct IndirectIndexedData {
	 *        uint32_t count;           // index count
	 *        uint32_t instanceCount;
	 *        uint32_t firstIndex;
	 *        int baseVertex;
	 *        uint32_t baseInstance;
	 *    };
	 *    struct IndirectArrayData {
	 *        uint32_t count;           // vertex count
	 *        uint32_t instanceCount;
	 *        uint32_t firstVertex;
	 *        uint32_t baseInstance;
	 *    };
	 **/
	struct DrawCommand {
		// if mode=1, then the data is:
		//   count, instanceCount, firstIndex, baseVertex, baseInstance
		// if mode=2, then the data is:
		//   count, instanceCount, firstVertex, baseInstance, _padding
		uint32_t data[5] = {0, 0, 0, 0, 0};
		// 0: skip, 1: element, 2: array
		uint32_t mode = 0u;
		uint32_t _pad[2] = {0, 0}; // pad to 32 bytes

		/**
		 * @return number of elements to draw.
		 */
		uint32_t count() const { return data[0]; }

		/**
		 * @param v wither number of elements to draw (mode=1) or number of vertices to draw (mode=2).
		 */
		void setCount(uint32_t v) { data[0] = v; }

		/**
		 * @return number of instances to draw.
		 */
		uint32_t instanceCount() const { return data[1]; }

		/**
		 * @param v number of instances to draw.
		 */
		void setInstanceCount(uint32_t v) { data[1] = v; }

		/**
		 * @return the first index (mode=1) or vertex (mode=2) to draw.
		 */
		uint32_t firstElement() const { return data[2]; }

		/**
		 * @param v the first index (mode=1) or vertex (mode=2) to draw.
		 */
		void setFirstElement(uint32_t v) { data[2] = v; }

		/**
		 * @return the base instance offset.
		 */
		uint32_t baseInstance() const { return data[mode==1 ? 4 : 3]; }

		/**
		 * @param v the base instance offset.
		 */
		void setBaseInstance(uint32_t v) { data[mode==1 ? 4 : 3] = v; }
	};
} // namespace

#endif /* REGEN_BOUNDING_BOX_BUFFER_H_ */

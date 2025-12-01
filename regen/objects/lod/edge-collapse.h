#ifndef REGEN_EDGE_COLLAPSE_H
#define REGEN_EDGE_COLLAPSE_H

#include <queue>
#include <vector>
#include <cstdint>
#include <regen/compute/vector.h>

namespace regen {
	/**
	 * \brief Edge collapse structure.
	 *
	 * This structure represents an edge collapse operation in a mesh.
	 * It contains the two vertices to be collapsed, the optimal position for the collapse,
	 * and the cost of the collapse.
	 */
	struct EdgeCollapse {
		uint32_t v1, v2;
		Vec3f optimalPos;
		double cost;

		bool operator>(const EdgeCollapse &other) const {
			return cost > other.cost;
		}
	};

	/**
	 * \brief Edge collapse priority queue.
	 *
	 * This is a priority queue that stores edge collapse operations.
	 * It uses a min-heap to ensure that the edge with the lowest cost is at the top.
	 */
	using EgdeCollapseQueue = std::priority_queue<
			EdgeCollapse,
			std::vector<EdgeCollapse>,
			std::greater<>>;
}

#endif //REGEN_EDGE_COLLAPSE_H

#ifndef REGEN_QUAD_TREE_H_
#define REGEN_QUAD_TREE_H_

#include <stack>
#include <vector>
#include <regen/shapes/spatial-index.h>
#include <regen/shapes/bounds.h>
#include <regen/shapes/orthogonal-projection.h>
#include "regen/utility/aligned-array.h"

namespace regen {
	/**
	 * A quad tree data structure for spatial indexing.
	 * It fits the scene into a quad which it subdivides at places where
	 * the number of shapes exceeds a certain threshold.
	 */
	class QuadTree : public SpatialIndex {
	public:
		// forward declaration of Node
		struct Node;
		/**
		 * An item in the quad tree, i.e. a shape with its orthogonal projection.
		 * Each item appears in the smallest node that fully contains it.
		 */
		struct Item {
			ref_ptr<BoundingShape> shape;
			OrthogonalProjection projection;
			Node *node = nullptr;
			// the position in the shape array of the node if any
			uint32_t idxInNode = 0;

			explicit Item(const ref_ptr<BoundingShape> &shape);
		};

		/**
		 * A node in the quad tree.
		 * It is either a leaf node or an internal node with 4 children.
		 * If it is a leaf node, it contains a (possibly empty) list of shapes.
		 */
		struct Node {
			Bounds<Vec2f> bounds;
			int32_t parentIdx = -1;
			int32_t childrenIdx[4];
			uint32_t nodeIdx = 0;
			std::vector<uint32_t> shapes;
			std::vector<uint32_t> shapesTmp;

			Node(const Vec2f &min, const Vec2f &max);

			~Node();

			inline bool isLeaf() const;

			inline bool isCollapsable() const;

			inline bool intersects(const OrthogonalProjection &projection) const;

			inline bool contains(const OrthogonalProjection &projection) const;
		};
		/**
		 * @brief Configuration for 3D intersection tests in the quad tree.
		 * QUAD_TREE_3D_TEST_NONE: No intersection test.
		 * QUAD_TREE_3D_TEST_CLOSEST: Only the closest intersection is returned.
		 * QUAD_TREE_3D_TEST_ALL: All intersections are returned.
		 */
		enum TestMode_3D {
			QUAD_TREE_3D_TEST_NONE = 0,
			QUAD_TREE_3D_TEST_CLOSEST,
			QUAD_TREE_3D_TEST_ALL
		};

		QuadTree();

		~QuadTree() override;

		/**
		 * @brief Get the root node of the quad tree
		 * @return The root node
		 */
		const Node *root() const { return root_; }

		/**
		 * @brief Get the number of nodes in the quad tree
		 * @return The number of nodes
		 */
		unsigned int numNodes() const { return numNodes_; }

		/**
		 * @brief Get the number of leaves in the quad tree
		 * @return The number of leaves
		 */
		unsigned int numLeaves() const { return numLeaves_; }

		/**
		 * @brief Set the minimum size of a node
		 * @param size The minimum size
		 */
		void setMinNodeSize(float size) { minNodeSize_ = size; }

		/**
		 * @brief Set the subdivision threshold, i.e. the maximum number of shapes
		 * a node can contain before it is subdivided.
		 * @param threshold The subdivision threshold
		 */
		void setSubdivisionThreshold(unsigned int threshold) {
			subdivisionThreshold_ = threshold;
		}

		/**
		 * Set the test mode for 3D intersection tests.
		 * @param mode The test mode to set
		 */
		void setTestMode3D(TestMode_3D mode) { testMode3D_ = mode; }

		/**
		 * Set the distance threshold for close distance tests.
		 * @param d The distance threshold to set
		 */
		void setCloseDistanceSquared(float d) { closeDistanceSquared_ = d * d; }

		/**
		 * Set the traversal bit used for intersection tests.
		 * @param bit The traversal bit to set
		 */
		void setTraversalBit(uint32_t bit) { traversalBit_ = bit; }

		// override SpatialIndex::insert
		void insert(const ref_ptr<BoundingShape> &shape) override;

		// override SpatialIndex::remove
		void remove(const ref_ptr<BoundingShape> &shape) override;

		// override SpatialIndex::update
		void update(float dt) override;

		// override SpatialIndex::hasIntersection
		bool hasIntersection(const BoundingShape &shape, uint32_t traversalBit) override;

		// override SpatialIndex::numIntersections
		int numIntersections(const BoundingShape &shape, uint32_t traversalBit) override;

		// override SpatialIndex::foreachIntersection
		void foreachIntersection(
				const BoundingShape &shape,
				void (*callback)(const BoundingShape&, void*),
				void *userData,
				uint32_t traversalBit) override;

		// override SpatialIndex
		void debugDraw(DebugInterface &debug) const override;

	protected:
		struct Private;
		Private *priv_;

		uint32_t subdivisionThreshold_ = 4;
		uint32_t traversalBit_ = BoundingShape::TRAVERSAL_BIT_DRAW;

		std::vector<Node*> nodes_;
		std::vector<Item *> items_;
		std::vector<Item *> newItems_;
		std::stack<Node *> nodePool_;
		std::stack<Item *> itemPool_;
		Node *root_ = nullptr;

		std::unordered_map<const BoundingShape*, uint32_t> shapeToItem_;
		float minNodeSize_ = 0.1f;
		uint32_t numNodes_ = 0;
		uint32_t numLeaves_ = 0;

		TestMode_3D testMode3D_ = QUAD_TREE_3D_TEST_CLOSEST;
		float closeDistanceSquared_ = 20.0f * 20.0f; // heuristic threshold for distance to camera position

		Bounds<Vec2f> newBounds_;
		std::vector<uint32_t> changedItems_;

		Node *createNode(const Vec2f &min, const Vec2f &max);

		void freeNode(Node *node);

		Item *createItem(const ref_ptr<BoundingShape> &shape);

		void freeItem(Item *item);

		bool reinsert(uint32_t itemIdx, bool allowSubdivision);

		bool insert(Node *node, uint32_t shapeIdx, bool allowSubdivision);

		bool insert1(Node *node, uint32_t newShapeIdx, bool allowSubdivision);

		void removeFromNode(Node *node, uint32_t itemIdx);

		Node* collapse(Node *node);

		void subdivide(Node *node);

		unsigned int numShapes() const;

		friend class QuadTreeTest;
	};
} // namespace

#endif /* REGEN_QUAD_TREE_H_ */

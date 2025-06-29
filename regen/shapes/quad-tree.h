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
		 * Eah item may appear in multiple nodes.
		 */
		struct Item {
			ref_ptr<BoundingShape> shape;
			OrthogonalProjection projection;
			std::vector<Node *> nodes;
			bool visited = false; // used during intersection tests

			explicit Item(const ref_ptr<BoundingShape> &shape);

			void removeNode(Node *node);
		};

		/**
		 * A node in the quad tree.
		 * It is either a leaf node or an internal node with 4 children.
		 * If it is a leaf node, it contains a (possibly empty) list of shapes.
		 */
		struct Node {
			Bounds<Vec2f> bounds;
			Node *parent;
			Node *children[4];
			std::vector<Item *> shapes;

			Node(const Vec2f &min, const Vec2f &max);

			~Node();

			inline bool isLeaf() const;

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
		 * Set the test mode for 3D intersection tests.
		 * @param mode The test mode to set
		 */
		void setTestMode3D(TestMode_3D mode) { testMode3D_ = mode; }

		/**
		 * Set the distance threshold for close distance tests.
		 * @param distance The distance threshold to set
		 */
		void setCloseDistanceSquared(float d) { closeDistanceSquared_ = d * d; }

		// override SpatialIndex::insert
		void insert(const ref_ptr<BoundingShape> &shape) override;

		// override SpatialIndex::remove
		void remove(const ref_ptr<BoundingShape> &shape) override;

		// override SpatialIndex::update
		void update(float dt) override;

		// override SpatialIndex::hasIntersection
		bool hasIntersection(const BoundingShape &shape) override;

		// override SpatialIndex::numIntersections
		int numIntersections(const BoundingShape &shape) override;

		// override SpatialIndex::foreachIntersection
		void foreachIntersection(
				const BoundingShape &shape,
				void (*callback)(const BoundingShape&, void*),
				void *userData) override;

		// override SpatialIndex
		void debugDraw(DebugInterface &debug) const override;

	protected:
		struct Private;
		Private *priv_;

		Node *root_ = nullptr;
		std::unordered_map<BoundingShape*, Item*> shapeToItem_;
		std::vector<Item *> items_;
		std::vector<Item *> newItems_;
		std::stack<Node *> nodePool_;
		std::stack<Item *> itemPool_;
		float minNodeSize_ = 0.1f;
		uint32_t numNodes_ = 0;
		uint32_t numLeaves_ = 0;

		TestMode_3D testMode3D_ = QUAD_TREE_3D_TEST_CLOSEST;
		float closeDistanceSquared_ = 20.0f * 20.0f; // heuristic threshold for distance to camera position

		Bounds<Vec2f> newBounds_;
		std::vector<Item *> changedItems_;

		QuadTree::Item* getItem(const ref_ptr<BoundingShape> &shape);

		Node *createNode(const Vec2f &min, const Vec2f &max);

		void freeNode(Node *node);

		Item *createItem(const ref_ptr<BoundingShape> &shape);

		void freeItem(Item *item);

		bool insert(Node *node, Item *shape, bool allowSubdivision);

		bool insert1(Node *node, Item *shape, bool allowSubdivision);

		void removeFromNodes(Item *shape);

		void removeFromNode(Node *node, Item *shape);

		void collapse(Node *node);

		void subdivide(Node *node);

		unsigned int numShapes() const;

		friend class QuadTreeTest;
	};
} // namespace

#endif /* REGEN_QUAD_TREE_H_ */

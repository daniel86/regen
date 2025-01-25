#ifndef REGEN_QUAD_TREE_H_
#define REGEN_QUAD_TREE_H_

#include <stack>
#include <vector>
#include <regen/shapes/spatial-index.h>
#include <regen/shapes/bounds.h>
#include <regen/shapes/orthogonal-projection.h>

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

			bool isLeaf() const;

			bool intersects(const OrthogonalProjection &projection) const;
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
		unsigned int numNodes() const;

		/**
		 * @brief Set the minimum size of a node
		 * @param size The minimum size
		 */
		void setMinNodeSize(float size) { minNodeSize_ = size; }

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
				const std::function<void(const BoundingShape &)> &callback) override;

		// override SpatialIndex
		void debugDraw(DebugInterface &debug) const override;

	protected:
		Node *root_ = nullptr;
		std::map<BoundingShape*, Item*> items_;
		std::vector<Item *> newItems_;
		std::stack<Node *> nodePool_;
		float minNodeSize_ = 0.1f;

		QuadTree::Item* getItem(const ref_ptr<BoundingShape> &shape);

		Node *createNode(const Vec2f &min, const Vec2f &max);

		void freeNode(Node *node);

		bool insert(Node *node, Item *shape, bool allowSubdivision);

		bool insert1(Node *node, Item *shape, bool allowSubdivision);

		void reinsertShapes(Node *node);

		void remove(Item *shape);

		void remove(Node *node, Item *shape);

		void collapse(Node *node);

		void subdivide(Node *node);

		void intersect2D(
			const BoundingShape &shape,
			const OrthogonalProjection &projection,
			const QuadTree::Node *node,
			std::atomic<unsigned int> &jobCounter,
			std::set<const Item *> &visited,
			std::mutex &mutex,
			const std::function<void(const BoundingShape &)> &callback);

		friend class QuadTreeTest;
	};
} // namespace

#endif /* REGEN_QUAD_TREE_H_ */

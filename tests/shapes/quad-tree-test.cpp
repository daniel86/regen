
#include "gtest/gtest.h"
#include "regen/shapes/quad-tree.h"
#include "regen/shapes/bounding-sphere.h"
#include "regen/shapes/aabb.h"

using namespace regen;

// fixture class for testing
class QuadTreeTest : public ::testing::Test {

};

/*
static inline auto testSphere(const Vec3f &center, float radius) {
	auto sphere = ref_ptr<BoundingSphere>::alloc(radius);
	auto centerInput = ref_ptr<ShaderInput3f>::alloc("center");
	centerInput->setUniformData(center);
	sphere->setCenter(centerInput);
	return sphere;
}

static inline auto testAABB(const Vec3f &center, const Vec3f &halfSize) {
	auto aabb = ref_ptr<AABB>::alloc(halfSize);
	auto centerInput = ref_ptr<ShaderInput3f>::alloc("center");
	centerInput->setUniformData(center);
	aabb->setCenter(centerInput);
	return aabb;
}
*/

TEST(QuadTreeTest, EmptyTree) {
	QuadTree tree;
	EXPECT_EQ(tree.root(), nullptr);
}

/*
TEST(QuadTreeTest, OneSphere_bounds) {
	QuadTree tree;
	tree.insert(testSphere(Vec3f(0, 0, 0), 0.5f));
	EXPECT_NE(tree.root(), nullptr);
	// root node is a leaf node
	EXPECT_TRUE(tree.root()->isLeaf());
	// test the bounds of the root node
	EXPECT_EQ(tree.root()->bounds.min, Vec2f(-0.5f, -0.5f));
	EXPECT_EQ(tree.root()->bounds.max, Vec2f(0.5f, 0.5f));
}

TEST(QuadTreeTest, TwoSpheres_bounds) {
	QuadTree tree;
	tree.insert(testSphere(Vec3f(0, 0, 0), 0.5f));
	tree.insert(testSphere(Vec3f(1, 0, 1), 0.5f));
	// root node is a leaf node
	EXPECT_TRUE(tree.root()->isLeaf());
	// test the bounds of the root node
	EXPECT_EQ(tree.root()->bounds.min, Vec2f(-0.5f, -0.5f));
	EXPECT_EQ(tree.root()->bounds.max, Vec2f(1.5f, 1.5f));
}

TEST(QuadTreeTest, Spheres_subdivision) {
	QuadTree tree;
	tree.insert(testSphere(Vec3f(1, 0, 1), 0.5f));
	tree.insert(testSphere(Vec3f(2, 0, 3), 0.5f));
	tree.insert(testSphere(Vec3f(2, 0, 2), 0.5f));
	tree.insert(testSphere(Vec3f(3, 0, 1), 0.5f));
	EXPECT_TRUE(tree.root()->isLeaf());
	tree.insert(testSphere(Vec3f(3, 0, 4), 0.5f));
	EXPECT_FALSE(tree.root()->isLeaf());
	// test the bounds of the root node
	EXPECT_EQ(tree.root()->bounds.min, Vec2f(0.5f, 0.5f));
	EXPECT_EQ(tree.root()->bounds.max, Vec2f(3.5f, 4.5f));
	// test bottom-left sub-node
	auto &bottomLeft = tree.root()->children[static_cast<int>(QuadSegment::BOTTOM_LEFT)];
	EXPECT_EQ(bottomLeft->bounds.min, Vec2f(0.5f, 0.5f));
	EXPECT_EQ(bottomLeft->bounds.max, Vec2f(2.0f, 2.5f));
	EXPECT_EQ(bottomLeft->shapes.size(), 2);
	// test other sub-nodes
	EXPECT_EQ(tree.root()->children[static_cast<int>(QuadSegment::BOTTOM_RIGHT)]->shapes.size(), 2);
	EXPECT_EQ(tree.root()->children[static_cast<int>(QuadSegment::TOP_RIGHT)]->shapes.size(), 2);
	EXPECT_EQ(tree.root()->children[static_cast<int>(QuadSegment::TOP_LEFT)]->shapes.size(), 1);
}

TEST(QuadTreeTest, SphereSphere_intersection) {
	QuadTree tree;
	tree.insert(testSphere(Vec3f(0, 0, 0), 0.5f));
	// test intersection with some spheres
	EXPECT_EQ(tree.numIntersections(testSphere(Vec3f(0, 0, 0), 0.5f)), 1);
	EXPECT_EQ(tree.numIntersections(testSphere(Vec3f(0, 0, 0), 0.25f)), 1);
	EXPECT_EQ(tree.numIntersections(testSphere(Vec3f(0, 0, 0), 1.0f)), 1);
	EXPECT_EQ(tree.numIntersections(testSphere(Vec3f(0.5, 0.5, 0.5), 0.5f)), 1);
	EXPECT_EQ(tree.numIntersections(testSphere(Vec3f(0.25, 0.25, 0.25), 0.25f)), 1);
	// some negative tests
	EXPECT_EQ(tree.numIntersections(testSphere(Vec3f(1, 1, 1), 0.5f)), 0);
	EXPECT_EQ(tree.numIntersections(testSphere(Vec3f(1, 1, 1), 0.25f)), 0);
}

TEST(QuadTreeTest, SphereSphere_multi_intersection) {
	QuadTree tree;
	tree.insert(testSphere(Vec3f(1, 0, 1), 0.5f));
	tree.insert(testSphere(Vec3f(2, 0, 3), 0.5f));
	tree.insert(testSphere(Vec3f(2, 0, 2), 0.5f));
	tree.insert(testSphere(Vec3f(3, 0, 1), 0.5f));
	tree.insert(testSphere(Vec3f(3, 0, 4), 0.5f));
	EXPECT_EQ(tree.numIntersections(testSphere(Vec3f(1, 0, 2.5), 0.5f)), 0);
	EXPECT_EQ(tree.numIntersections(testSphere(Vec3f(2, 0, 2.5), 0.5f)), 2);
	EXPECT_EQ(tree.numIntersections(testSphere(Vec3f(3, 0, 2.5), 0.5f)), 0);
}

TEST(QuadTreeTest, SphereAABB_intersection) {
	QuadTree tree;
	tree.insert(testSphere(Vec3f(1, 0, 1), 1.0f));
	// test intersection with some AABBs
	EXPECT_EQ(tree.hasIntersection(testAABB(Vec3f(0.5f, 0, 0.5f), Vec3f(0.5))), 1);
	EXPECT_EQ(tree.hasIntersection(testAABB(Vec3f(1.0f, 0, 0.5f), Vec3f(0.5))), 1);
	EXPECT_EQ(tree.hasIntersection(testAABB(Vec3f(1.5f, 0, 0.5f), Vec3f(0.5))), 1);
	// some negative tests
	EXPECT_EQ(tree.hasIntersection(testAABB(Vec3f(2.5f, 0, 0.5f), Vec3f(0.45))), 0);
	EXPECT_EQ(tree.hasIntersection(testAABB(Vec3f(-2.5f, 0, 0.5f), Vec3f(0.45))), 0);
	EXPECT_EQ(tree.hasIntersection(testAABB(Vec3f(1.0f, 0, 2.5f), Vec3f(0.45))), 0);
	EXPECT_EQ(tree.hasIntersection(testAABB(Vec3f(1.0f, 0, -2.5f), Vec3f(0.45))), 0);
}
*/

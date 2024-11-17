#include "regen/meshes/proc-tree.h"

using namespace regen;

ProcTree::ProcTree() {
	trunk.mesh = ref_ptr<Mesh>::alloc(GL_TRIANGLES, regen::VBO::USAGE_STATIC);
	trunk.indices = ref_ptr<ShaderInput1ui>::alloc("i");
	trunk.pos = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	trunk.nor = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	trunk.tan = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	trunk.texco = ref_ptr<ShaderInput2f>::alloc("texco0");

	twig.mesh = ref_ptr<Mesh>::alloc(GL_TRIANGLES, regen::VBO::USAGE_STATIC);
	twig.indices = ref_ptr<ShaderInput1ui>::alloc("i");
	twig.pos = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	twig.nor = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	twig.tan = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	twig.texco = ref_ptr<ShaderInput2f>::alloc("texco0");
}

void ProcTree::updateAttributes(TreeMesh &treeMesh,
			int numVertices, int numFaces,
			Proctree::fvec3 *vertices,
			Proctree::fvec3 *normals,
			Proctree::fvec2 *uvs,
			Proctree::ivec3 *faces) {
	auto numIndices = numFaces * 3;
	treeMesh.indices->setVertexData(numIndices, reinterpret_cast<const unsigned char *>(&faces[0].x));
	treeMesh.pos->setVertexData(numVertices, reinterpret_cast<const unsigned char *>(&vertices[0].x));
	treeMesh.nor->setVertexData(numVertices, reinterpret_cast<const unsigned char *>(&normals[0].x));
	treeMesh.texco->setVertexData(numVertices, reinterpret_cast<const unsigned char *>(&uvs[0].u));
	treeMesh.tan->setVertexData(numVertices);

	for (int i = 0; i < numFaces; i++) {
		auto &v0 = *((Vec3f *) &vertices[faces[i].x]);
		auto &v1 = *((Vec3f *) &vertices[faces[i].y]);
		auto &v2 = *((Vec3f *) &vertices[faces[i].z]);
		auto &uv0 = *((Vec2f *) &uvs[faces[i].x]);
		auto &uv1 = *((Vec2f *) &uvs[faces[i].y]);
		auto &uv2 = *((Vec2f *) &uvs[faces[i].z]);
		auto e1 = v1 - v0;
		auto e2 = v2 - v0;
		auto deltaUV1 = uv1 - uv0;
		auto deltaUV2 = uv2 - uv0;
		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
		Vec3f tangent3 = (e1 * deltaUV2.y - e2 * deltaUV1.y) * f;
		Vec4f tangent(tangent3.x, tangent3.y, tangent3.z, 1.0f);
		treeMesh.tan->setVertex(faces[i].x, tangent);
		treeMesh.tan->setVertex(faces[i].y, tangent);
		treeMesh.tan->setVertex(faces[i].z, tangent);
	}

	treeMesh.mesh->begin(ShaderInputContainer::INTERLEAVED);
	treeMesh.mesh->setIndices(treeMesh.indices, numVertices);
	treeMesh.mesh->setInput(treeMesh.pos);
	treeMesh.mesh->setInput(treeMesh.nor);
	treeMesh.mesh->setInput(treeMesh.tan);
	treeMesh.mesh->setInput(treeMesh.texco);
	treeMesh.mesh->end();
}

void ProcTree::updateTrunkAttributes() {
	updateAttributes(trunk, handle.mVertCount, handle.mFaceCount,
					 handle.mVert, handle.mNormal, handle.mUV, handle.mFace);
}

void ProcTree::updateTwigAttributes() {
	updateAttributes(twig, handle.mTwigVertCount, handle.mTwigFaceCount,
					 handle.mTwigVert, handle.mTwigNormal, handle.mTwigUV, handle.mTwigFace);
}

void ProcTree::update() {
	handle.generate();
	updateTrunkAttributes();
	updateTwigAttributes();
}

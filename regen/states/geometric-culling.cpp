/*
 * geometric-culling.cpp
 *
 *  Created on: Oct 17, 2014
 *      Author: daniel
 */

#include <regen/states/state-node.h>
#include "geometric-culling.h"
#include "regen/camera/sorting.h"

using namespace regen;

GeometricCulling::GeometricCulling(
		const ref_ptr<Camera> &camera,
		const ref_ptr<SpatialIndex> &spatialIndex,
		std::string_view shapeName)
		: StateNode(),
		  camera_(camera),
		  spatialIndex_(spatialIndex) {
	shapeIndex_ = spatialIndex_->getIndexedShape(camera, shapeName);
	if (shapeIndex_.get()) {
		numInstances_ = shapeIndex_->shape()->numInstances();
		mesh_ = shapeIndex_->shape()->mesh();
	}
	if (!mesh_.get()) {
		numInstances_ = 1;
	}
	if (numInstances_ > 1) {
		// create array with numInstances_ elements
		instanceIDMap_ = ref_ptr<ShaderInput1ui>::alloc("instanceIDMap", numInstances_);
		instanceIDMap_->setInstanceData(1, 1, nullptr);
		auto instanceData = instanceIDMap_->mapClientData<GLuint>(ShaderData::WRITE);
		for (GLuint i = 0; i < numInstances_; ++i) {
			instanceData.w[i] = i;
		}
		instanceData.unmap();
		state()->joinShaderInput(instanceIDMap_);
		lodGroups_.resize(mesh_->numLODs());
	}
}

void GeometricCulling::updateMeshLOD() {
	if (!mesh_.get() || mesh_->numLODs() <= 1) {
		return;
	}
	auto &shape = shapeIndex_->shape();
	// set LOD level based on distance
	auto camPos = camera_->position()->getVertex(0);
	auto distance = (shape->getCenterPosition() - camPos.r).length();
	camPos.unmap();
	mesh_->updateLOD(distance);
	for (auto &part : shape->parts()) {
		if (part->numLODs()>1) {
			part->updateLOD(distance);
		}
	}
}

void computeLODGroups_(
		const ref_ptr<Mesh> &mesh,
		const ref_ptr<BoundingShape> &shape,
		const ref_ptr<Camera> &camera,
		std::vector<std::vector<GLuint>> &lodGroups,
		const unsigned int *mappedData,
		int begin,
		int end,
		int increment) {
	auto &transform = shape->transform();
	auto &modelOffset = shape->modelOffset();
	auto camPos = camera->position()->getVertex(0);

	if (lodGroups.size() == 1) {
		for (int i = begin; i != end; i += increment) {
			lodGroups[0].push_back(mappedData[i]);
		}
	}
	else if (transform.get() && modelOffset.get()) {
		auto modelOffsetData = modelOffset->mapClientData<Vec3f>(ShaderData::READ);
		auto tfData = transform->get()->mapClientData<Mat4f>(ShaderData::READ);
		for (int i = begin; i != end; i += increment) {
			auto lodLevel = mesh->getLODLevel((
				tfData.r[mappedData[i]].position() +
				modelOffsetData.r[mappedData[i]] - camPos.r).length());
			lodGroups[lodLevel].push_back(mappedData[i]);
		}
	}
	else if (modelOffset.get()) {
		auto modelOffsetData = modelOffset->mapClientData<Vec3f>(ShaderData::READ);
		for (int i = begin; i != end; i += increment) {
			auto lodLevel = mesh->getLODLevel((
				modelOffsetData.r[mappedData[i]] - camPos.r).length());
			lodGroups[lodLevel].push_back(mappedData[i]);
		}
	}
	else if (transform.get()) {
		auto tfData = transform->get()->mapClientData<Mat4f>(ShaderData::READ);
		for (int i = begin; i != end; i += increment) {
			auto lodLevel = mesh->getLODLevel((
				tfData.r[mappedData[i]].position() - camPos.r).length());
			lodGroups[lodLevel].push_back(mappedData[i]);
		}
	}
	else {
		for (int i = begin; i != end; i += increment) {
			lodGroups[0].push_back(mappedData[i]);
		}
	}
}

void GeometricCulling::computeLODGroups() {
	for (auto &lodGroup : lodGroups_) {
		lodGroup.clear();
	}
	auto visible_ids = shapeIndex_->mapInstanceIDs(ShaderData::READ);
	auto numVisible = visible_ids.r[0];
	if (numVisible == 0) { return; }

	if (instanceSortMode_ == SortMode::BACK_TO_FRONT) {
		computeLODGroups_(
			mesh_,
			shapeIndex_->shape(),
			camera_,
			lodGroups_,
			visible_ids.r+1,
			static_cast<int>(numVisible) - 1,
			-1,
			-1);
	} else {
		computeLODGroups_(
			mesh_,
			shapeIndex_->shape(),
			camera_,
			lodGroups_,
			visible_ids.r+1,
			0,
			static_cast<int>(numVisible),
			1);
	}
}

void GeometricCulling::traverseInstanced_(RenderState *rs, unsigned int numVisible) {
	// set number of visible instances
	mesh_->inputContainer()->set_numVisibleInstances(numVisible);
	for (auto &part : shapeIndex_->shape()->parts()) {
		part->inputContainer()->set_numVisibleInstances(numVisible);
	}
	StateNode::traverse(rs);
	// reset number of visible instances
	mesh_->inputContainer()->set_numVisibleInstances(numInstances_);
	for (auto &part : shapeIndex_->shape()->parts()) {
		part->inputContainer()->set_numVisibleInstances(numInstances_);
	}
}

void GeometricCulling::traverseInstanced1(RenderState *rs) {
	// Shape does not have LOD levels, thus shapeIndex_ instance array can be used directly
	// TODO: Avoid the copy here and below. Exchanging the data pointers should be enough.
	//       But shader input does not have an interface for this yet. It manages two data slots.
	//       So the new operation would need to write-lock one slot, but then exchange the data pointers, plus reversing
	//       after traversal.
	auto instance_ids = instanceIDMap_->mapClientData<unsigned int>(ShaderData::WRITE);
	auto visible_ids = shapeIndex_->mapInstanceIDs(ShaderData::READ);
	auto numVisible = visible_ids.r[0];
	std::memcpy(instance_ids.w, visible_ids.r+1, numVisible * sizeof(unsigned int));
	visible_ids.unmap();
	instance_ids.unmap();
	traverseInstanced_(rs, numVisible);
}

void GeometricCulling::traverseInstanced2(RenderState *rs, const unsigned int *visibleInstances, unsigned int numVisible) {
	// update instanceIDMap_ based on visibility
	auto instance_ids = instanceIDMap_->mapClientData<unsigned int>(ShaderData::WRITE);
	std::memcpy(instance_ids.w, visibleInstances, numVisible * sizeof(unsigned int));
	instance_ids.unmap();
	traverseInstanced_(rs, numVisible);
}

void GeometricCulling::traverse(RenderState *rs) {
	if (!spatialIndex_->hasCamera(*camera_.get()) || !shapeIndex_.get()) {
		updateMeshLOD();
		StateNode::traverse(rs);
	}
	else if (numInstances_ <= 1) {
		if (shapeIndex_->isVisible()) {
			updateMeshLOD();
			StateNode::traverse(rs);
		}
	} else if (!mesh_.get()) {
		REGEN_WARN("No mesh set for shape " << shapeIndex_->shape()->name());
		numInstances_ = 0;
	}
	else {
		if (!shapeIndex_->hasVisibleInstances()) {
			// no visible instances
			return;
		}

		if (lodGroups_.size() < 2) {
			traverseInstanced1(rs);
		}
		else {
			// build LOD groups, then traverse each group
			computeLODGroups();

			for (unsigned int lodLevel=0; lodLevel<mesh_->numLODs(); ++lodLevel) {
				auto &lodGroup = lodGroups_[lodLevel];
				if (lodGroup.empty()) { continue; }
				// set the LOD level
				if (lodGroups_.size() > 1) {
					mesh_->activateLOD(lodLevel);
				}
				// set number of visible instances
				for (auto &part : shapeIndex_->shape()->parts()) {
					if (mesh_->numLODs() == part->numLODs() && part->numLODs() > 1) {
						part->activateLOD(lodLevel);
					}
					else if (part->numLODs()>1) {
						// could be part has different number of LODs, need to compute an adjusted
						// LOD level for each part
						part->activateLOD(static_cast<unsigned int>(std::round(static_cast<float>(lodLevel) *
							static_cast<float>(part->numLODs()) / static_cast<float>(mesh_->numLODs()))));
					}
				}
				traverseInstanced2(rs, lodGroup.data(), lodGroup.size());
			}
			// reset LOD level
			if (lodGroups_.size() > 1) {
				mesh_->activateLOD(0);
			}
		}
	}
}

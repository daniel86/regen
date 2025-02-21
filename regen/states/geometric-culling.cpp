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
		  spatialIndex_(spatialIndex),
		  shapeName_(shapeName) {
	numInstances_ = spatialIndex_->numInstances(shapeName);
	mesh_ = spatialIndex_->getMeshOfShape(shapeName);
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
	auto shape = spatialIndex_->getShape(shapeName_);
	if (!shape.get()) {
		REGEN_WARN("No shape found in index for " << shapeName_);
		return;
	}
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

template<typename T>
void computeLODGroups_(
		const ref_ptr<Mesh> &mesh,
		const ref_ptr<BoundingShape> &shape,
		const ref_ptr<Camera> &camera,
		std::vector<std::vector<GLuint>> &lodGroups,
		const T &begin,
		const T &end) {
	auto &transform = shape->transform();
	auto &modelOffset = shape->modelOffset();
	auto camPos = camera->position()->getVertex(0);

	if (lodGroups.size() == 1) {
		for (auto visibleInstance = begin; visibleInstance != end; ++visibleInstance) {
			lodGroups[0].push_back(*visibleInstance);
		}
	}
	else if (transform.get() && modelOffset.get()) {
		auto modelOffsetData = modelOffset->mapClientData<Vec3f>(ShaderData::READ);
		auto tfData = transform->get()->mapClientData<Mat4f>(ShaderData::READ);
		for (auto visibleInstance = begin; visibleInstance != end; ++visibleInstance) {
			auto lodLevel = mesh->getLODLevel((
				tfData.r[*visibleInstance].position() +
				modelOffsetData.r[*visibleInstance] - camPos.r).length());
			lodGroups[lodLevel].push_back(*visibleInstance);
		}
	}
	else if (modelOffset.get()) {
		auto modelOffsetData = modelOffset->mapClientData<Vec3f>(ShaderData::READ);
		for (auto visibleInstance = begin; visibleInstance != end; ++visibleInstance) {
			auto lodLevel = mesh->getLODLevel((
				modelOffsetData.r[*visibleInstance] - camPos.r).length());
			lodGroups[lodLevel].push_back(*visibleInstance);
		}
	}
	else if (transform.get()) {
		auto tfData = transform->get()->mapClientData<Mat4f>(ShaderData::READ);
		for (auto visibleInstance = begin; visibleInstance != end; ++visibleInstance) {
			auto lodLevel = mesh->getLODLevel((
				tfData.r[*visibleInstance].position() - camPos.r).length());
			lodGroups[lodLevel].push_back(*visibleInstance);
		}
	}
	else {
		for (auto visibleInstance = begin; visibleInstance != end; ++visibleInstance) {
			lodGroups[0].push_back(*visibleInstance);
		}
	}
}

void GeometricCulling::computeLODGroups(
		const std::vector<GLuint> &visibleInstances,
		const ref_ptr<BoundingShape> &shape,
		std::vector<std::vector<GLuint>> &lodGroups) {
	for (auto &lodGroup : lodGroups) {
		lodGroup.clear();
	}
	if (instanceSortMode_ == SortMode::BACK_TO_FRONT) {
		computeLODGroups_(
			mesh_,
			shape,
			camera_,
			lodGroups,
			visibleInstances.rbegin(),
			visibleInstances.rend());
	} else {
		computeLODGroups_(
			mesh_,
			shape,
			camera_,
			lodGroups,
			visibleInstances.begin(),
			visibleInstances.end());
	}
}

void GeometricCulling::traverse(RenderState *rs) {
	if (!spatialIndex_->hasCamera(*camera_.get())) {
		updateMeshLOD();
		StateNode::traverse(rs);
	}
	else if (numInstances_ <= 1) {
		if (spatialIndex_->isVisible(*camera_.get(), shapeName_)) {
			updateMeshLOD();
			StateNode::traverse(rs);
		}
	} else if (!mesh_.get()) {
		REGEN_WARN("No mesh set for shape " << shapeName_);
		numInstances_ = 0;
	} else {
		auto visibleInstances = spatialIndex_->getVisibleInstances(*camera_.get(), shapeName_);
		auto shape = spatialIndex_->getShape(shapeName_);
		//REGEN_INFO("shape " << shapeName_ << " has " << visibleInstances.size() << " visible instances");

		// build LOD groups, then traverse each group
		computeLODGroups(visibleInstances, shape, lodGroups_);

		for (unsigned int lodLevel=0; lodLevel<mesh_->numLODs(); ++lodLevel) {
			auto &lodGroup = lodGroups_[lodLevel];
			if (lodGroup.empty()) {
				continue;
			}
			// update instanceIDMap_ based on visibility
			auto instance_ids = instanceIDMap_->mapClientData<GLuint>(ShaderData::WRITE);
			for (GLuint i = 0; i < lodGroup.size(); ++i) {
				instance_ids.w[i] = lodGroup[i];
			}
			instance_ids.unmap();
			// set the LOD level
			if (lodGroups_.size() > 1) {
				mesh_->activateLOD(lodLevel);
			}
			// set number of visible instances
			mesh_->inputContainer()->set_numVisibleInstances(lodGroup.size());
			for (auto &part : shape->parts()) {
				if (mesh_->numLODs() == part->numLODs() && part->numLODs() > 1) {
					part->activateLOD(lodLevel);
				}
				else if (part->numLODs()>1) {
					// could be part has different number of LODs, need to compute an adusted
					// LOD level for each part
					auto adjustedLODLevel = static_cast<unsigned int>(std::round(
						static_cast<float>(lodLevel) *
						static_cast<float>(part->numLODs()) /
						static_cast<float>(mesh_->numLODs())));
					part->activateLOD(adjustedLODLevel);
				}
				part->inputContainer()->set_numVisibleInstances(lodGroup.size());
			}
			StateNode::traverse(rs);
			// reset number of visible instances
			mesh_->inputContainer()->set_numVisibleInstances(numInstances_);
			for (auto &part : shape->parts()) {
				part->inputContainer()->set_numVisibleInstances(numInstances_);
			}
		}
		// reset LOD level
		if (lodGroups_.size() > 1) {
			mesh_->activateLOD(0);
		}
	}
}

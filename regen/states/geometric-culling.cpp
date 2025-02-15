/*
 * geometric-culling.cpp
 *
 *  Created on: Oct 17, 2014
 *      Author: daniel
 */

#include <regen/states/state-node.h>
#include "geometric-culling.h"

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
	auto shapePos = shape->getCenterPosition();
	auto distance = (shapePos - camPos.r).length();
	mesh_->updateLOD(distance);
	for (auto &part : shape->parts()) {
		if (part->numLODs()>1) {
			part->updateLOD(distance);
		}
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
		auto &visibleInstances = spatialIndex_->getVisibleInstances(*camera_.get(), shapeName_);
		auto shape = spatialIndex_->getShape(shapeName_);
		auto &transform = shape->transform();

		//REGEN_INFO("shape " << shapeName_ << " has " << visibleInstances.size() << " visible instances");

		// build LOD groups, then traverse each group
		std::vector<std::vector<GLuint>> lodGroups(mesh_->numLODs());
		if (transform.get() && mesh_->numLODs() > 1) {
			// FIXME: need to consider offset here too! tf may not be instanced! BUG
			auto camPos = camera_->position()->getVertex(0);
			auto tfData = transform->get()->mapClientData<Mat4f>(ShaderData::READ);
			for (auto visibleInstance : visibleInstances) {
				auto &shapePos = tfData.r[visibleInstance].position();
				auto distance = (shapePos - camPos.r).length();
				auto lodLevel = mesh_->getLODLevel(distance);
				lodGroups[lodLevel].push_back(visibleInstance);
			}
		}
		else {
			auto &lodGroup = lodGroups[0];
			for (auto visibleInstance : visibleInstances) {
				lodGroup.push_back(visibleInstance);
			}
		}

		for (unsigned int lodLevel=0; lodLevel<mesh_->numLODs(); ++lodLevel) {
			auto &lodGroup = lodGroups[lodLevel];
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
			if (lodGroups.size() > 1) {
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
		if (lodGroups.size() > 1) {
			mesh_->activateLOD(0);
		}
	}
}

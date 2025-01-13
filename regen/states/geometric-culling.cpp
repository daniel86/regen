/*
 * geometric-culling.cpp
 *
 *  Created on: Oct 17, 2014
 *      Author: daniel
 */

#include <regen/states/state-node.h>
#include <regen/camera/camera.h>
#include <regen/meshes/mesh-state.h>

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
	if (numInstances_ > 1) {
		instanceIDMap_ = ref_ptr<ShaderInput1ui>::alloc("instanceIDMap", numInstances_);
		instanceIDMap_->setArrayData(numInstances_);
		for (GLuint i = 0; i < numInstances_; ++i) {
			instanceIDMap_->setVertex(i, i);
		}
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
	auto &camPos = camera_->position()->getVertex(0);
	auto shapePos = shape->getCenterPosition();
	auto distance = (shapePos - camPos).length();
	mesh_->updateLOD(distance);
	for (auto &part : shape->parts()) {
		part->updateLOD(distance);
	}
}

void GeometricCulling::traverse(RenderState *rs) {
	if (numInstances_ <= 1) {
		if (spatialIndex_->isVisible(*camera_.get(), shapeName_)) {
			updateMeshLOD();
			StateNode::traverse(rs);
		}
	} else if (!mesh_.get()) {
		REGEN_WARN("No mesh set for shape " << shapeName_);
		numInstances_ = 0;
	} else {
		auto &visibleInstances = spatialIndex_->getVisibleInstances(*camera_.get(), shapeName_);
		auto &camPos = camera_->position()->getVertex(0);
		auto shape = spatialIndex_->getShape(shapeName_);
		auto &transform = shape->transform();

		// build LOD groups, then traverse each group
		std::vector<std::vector<GLuint>> lodGroups(mesh_->numLODs());
		if (transform.get() && mesh_->numLODs() > 1) {
			auto &tf = transform->get();
			for (GLuint i = 0; i < visibleInstances.size(); ++i) {
				auto shapePos = tf->getVertex(visibleInstances[i]).position();
				auto distance = (shapePos - camPos).length();
				lodGroups[mesh_->getLODLevel(distance)].push_back(visibleInstances[i]);
			}
		}
		else {
			auto &lodGroup = lodGroups[0];
			for (GLuint i = 0; i < visibleInstances.size(); ++i) {
				lodGroup.push_back(visibleInstances[i]);
			}
		}

		for (unsigned int lodLevel=0; lodLevel<mesh_->numLODs(); ++lodLevel) {
			auto &lodGroup = lodGroups[lodLevel];
			if (lodGroup.empty()) {
				continue;
			}
			// update instanceIDMap_ based on visibility
			for (GLuint i = 0; i < lodGroup.size(); ++i) {
				instanceIDMap_->setVertex(i, lodGroup[i]);
			}
			// set the LOD level
			mesh_->activateLOD(lodLevel);
			// set number of visible instances
			mesh_->inputContainer()->set_numVisibleInstances(lodGroup.size());
			StateNode::traverse(rs);
			// reset number of visible instances
			mesh_->inputContainer()->set_numVisibleInstances(numInstances_);
			// reset LOD level
			mesh_->activateLOD(0);
		}
	}
}

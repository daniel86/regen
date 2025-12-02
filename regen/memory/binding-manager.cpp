#include <GL/glew.h>
#include "binding-manager.h"
#include <regen/gl/gl-param.h>
#include <regen/utility/logging.h>

using namespace regen;

BindingManager::BindingManager() {
	maxBindings_[UBO] = glParam<int32_t>(GL_MAX_UNIFORM_BUFFER_BINDINGS);
	maxBindings_[SSBO] = glParam<int32_t>(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS);
	nextBindingPoint_[UBO] = 0;
	nextBindingPoint_[SSBO] = 0;
	bindingPointCounter_[UBO].assign(maxBindings_[UBO], 0);
	bindingPointCounter_[SSBO].assign(maxBindings_[SSBO], 0);
}

void BindingManager::clear() {
	auto &instance = BindingManager::instance();
	instance.nextBindingPoint_[UBO] = 0;
	instance.nextBindingPoint_[SSBO] = 0;
	instance.bufferBindings_.clear();
	instance.bindingPointCounter_[UBO].assign(instance.maxBindings_[UBO], 0);
	instance.bindingPointCounter_[SSBO].assign(instance.maxBindings_[SSBO], 0);
}

int32_t BindingManager::request(
		BlockType blockType,
		std::uintptr_t bufferID,
		std::string_view bufferName,
		const std::set<int32_t> &avoidBindingPoints) {
	auto &instance = BindingManager::instance();
	return instance.request_(blockType, bufferID, bufferName, avoidBindingPoints);
}

int32_t BindingManager::request_(
		BlockType blockType,
		std::uintptr_t bufferID,
		std::string_view bufferName,
		const std::set<int32_t> &avoidBindingPoints) {
	auto &maxBindingPoints = maxBindings_[blockType];
	auto &nextBindingPoint = nextBindingPoint_[blockType];

	// check if the same buffer range was used before, and if so try to reuse the binding point.
	if (bufferID != 0) {
		auto it1 = bufferBindings_.find(bufferID);
		if (it1 != bufferBindings_.end()) {
			auto &bindingPoint = it1->second;
			if (avoidBindingPoints.find(bindingPoint) == avoidBindingPoints.end()) {
				// binding point is not in the list of avoided binding points
				bindingPointCounter_[blockType][bindingPoint]++;
				return bindingPoint;
			}
		}
	}

	int32_t bindingPoint;
	// no binding point was found, create a new one if possible
	if (nextBindingPoint < maxBindingPoints &&
			avoidBindingPoints.find(nextBindingPoint) == avoidBindingPoints.end()) {
		// raise the global binding point counter
		bindingPoint = nextBindingPoint++;
	} else {
		// all global binding points are used, try to reuse one
		auto minIt = std::min_element(
				bindingPointCounter_[blockType].begin(),
				bindingPointCounter_[blockType].end());
		bindingPoint = static_cast<int32_t>(std::distance(
			bindingPointCounter_[blockType].begin(), minIt));
		while (avoidBindingPoints.find(bindingPoint) != avoidBindingPoints.end()) {
			bindingPoint++;
			if (bindingPoint >= maxBindingPoints) {
				bindingPoint = 0;
			}
		}
	}

	bindingPointCounter_[blockType][bindingPoint]++;
	if (bufferID == 0) {
		// the block input was not added to the scene when the shader was created,
		// so we cannot ensure that once resolve the BO can be used across different shader programs
		// with same binding point.
		// This is usually only the case if there is some ping-ponging of the buffer objects involved,
		// i.e. if BO used by the program changes e.g. per frame.
		REGEN_DEBUG("Allocating unresolved "
			<< (blockType == UBO ? "UBO" : "SSBO") << " \""
			<< bufferName << "\" to binding point "
			<< bindingPoint << " (next=" << nextBindingPoint << ").");
	} else {
		bufferBindings_[bufferID] = bindingPoint;
		REGEN_DEBUG("Allocating "
			<< (blockType == UBO ? "UBO" : "SSBO") << " \""
			<< bufferName << "\" (" << bufferID << ") to binding point "
			<< bindingPoint << " (next=" << nextBindingPoint << ").");
	}
	return bindingPoint;
}


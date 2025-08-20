#include "name-registry.h"

using namespace regen;

NameRegistry& NameRegistry::instance() {
	static NameRegistry instance_;
	return instance_;
}

std::string_view NameRegistry::registerName(std::string_view name) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto [it, inserted] = names_.emplace(name);
	return *it;
}
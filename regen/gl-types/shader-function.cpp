#include "shader-function.h"

using namespace regen;

static std::string_view extractFunctor(std::string_view &importKey) {
	std::list<std::string_view> path;
	boost::split(path, importKey, boost::is_any_of("."));
	return *path.rbegin();
}

std::map<std::string_view, ref_ptr<ShaderFunction>> ShaderFunction::shaderFunctions_;

ref_ptr<ShaderFunction> ShaderFunction::createImport(std::string_view importKey) {
	return createImport(extractFunctor(importKey), importKey);
}

ref_ptr<ShaderFunction> ShaderFunction::createImport(std::string_view functor, std::string_view importKey) {
	auto needle = shaderFunctions_.find(importKey);
	if (needle != shaderFunctions_.end()) {
		return needle->second;
	}
	auto fun = ref_ptr<ShaderFunction>::alloc(functor, importKey, "");
	shaderFunctions_.insert({fun->importKey(), fun});
	return fun;
}

ref_ptr<ShaderFunction> ShaderFunction::createInline(std::string_view functor, std::string_view inlineCode) {
	// create hash of the inline code
	auto codeHash = std::to_string(std::hash<std::string_view>{}(inlineCode));
	auto needle = shaderFunctions_.find(codeHash);
	if (needle != shaderFunctions_.end()) {
		return needle->second;
	}
	auto fun = ref_ptr<ShaderFunction>::alloc(functor, "", inlineCode);
	shaderFunctions_.insert({codeHash, fun});
	return fun;
}

ref_ptr<ShaderFunction> ShaderFunction::load(scene::SceneInputNode &input, const std::string &keyPrefix) {
	auto nameAttr = REGEN_STRING(keyPrefix << "-name");
	auto keyAttr = REGEN_STRING(keyPrefix << "-key");
	auto codeAttr = REGEN_STRING(keyPrefix << "-code");
	auto funName = input.getValue(nameAttr);
	if (input.hasAttribute(codeAttr)) {
		if (funName.empty()) {
			REGEN_WARN("Ignoring " << input.getDescription() << ", no function name specified.");
		} else {
			return createInline(funName, input.getValue(codeAttr));
		}
	}
	if (input.hasAttribute(keyAttr)) {
		if (funName.empty()) {
			return createImport(input.getValue(keyAttr));
		} else {
			return createImport(funName, input.getValue(keyAttr));
		}
	}
	else if (input.hasAttribute(keyPrefix)) {
		if (funName.empty()) {
			return createImport(input.getValue(keyPrefix));
		} else {
			return createImport(funName, input.getValue(keyPrefix));
		}
	}
	return {};
}

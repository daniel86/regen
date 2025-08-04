#ifndef REGEN_SHADER_FUNCTION_H_
#define REGEN_SHADER_FUNCTION_H_

#include <string>
#include <set>
#include <map>
#include <GL/glew.h>
#include "regen/utility/ref-ptr.h"
#include "regen/scene/scene-input.h"

namespace regen {
	/**
	 * \brief A function that is used in a shader.
	 *
	 * This class is used to import functions from files on the import path
	 * or to define inline functions.
	 */
	class ShaderFunction {
	public:
		/**
		 * Constructor for importing a function.
		 * @param functor the function name.
		 * @param importKey the import key.
		 */
		static ref_ptr<ShaderFunction> createImport(std::string_view functor, std::string_view importKey);

		/**
		 * Constructor for importing a function.
		 * The functor name of the function must match last part of the import key.
		 * @param importKey the import key.
		 */
		static ref_ptr<ShaderFunction> createImport(std::string_view importKey);

		/**
		 * Constructor for inline code.
		 * @param functor the function name.
		 * @param inlineCode the inline code.
		 */
		static ref_ptr<ShaderFunction> createInline(std::string_view functor, std::string_view importKey);

		ShaderFunction(std::string_view functor, std::string_view importKey, std::string_view inlineCode)
				: functor_(functor), importKey_(importKey), inlineCode_(inlineCode) {}

		/**
		 * @return the function name.
		 */
		const std::string &functor() const { return functor_; }

		/**
		 * @return the import key.
		 */
		const std::string &importKey() const { return importKey_; }

		/**
		 * @return the inline code.
		 */
		const std::string &inlineCode() const { return inlineCode_; }

		/**
		 * @return true if the function is an inline function.
		 */
		bool isInlineFunction() const { return !inlineCode_.empty(); }

		/**
		 * @return true if the function is an imported function.
		 */
		bool isImportedFunction() const {
			return !isInlineFunction();
		}

		/**
		 * Load the function from a scene input node.
		 */
		static ref_ptr<ShaderFunction> load(scene::SceneInputNode &input, const std::string &keyPrefix);

	protected:
		static std::map<std::string_view, ref_ptr<ShaderFunction>> shaderFunctions_;
		const std::string functor_;
		const std::string importKey_;
		const std::string inlineCode_;
	};
} // namespace

#endif /* REGEN_ATOMIC_COUNTER_H_ */

/*
 * state-configurer.h
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#ifndef STATE_CONFIGURER_H_
#define STATE_CONFIGURER_H_

#include <regen/states/state-node.h>
#include <regen/states/state.h>
#include <regen/states/texture-state.h>
#include <regen/states/shader-state.h>
#include <regen/gl-types/shader-input.h>

namespace regen {
	/**
	 * \brief Shader configuration based on State's.
	 */
	class StateConfigurer {
	public:
		/**
		 * Load shader configuration based on a given node (and parent nodes).
		 */
		static StateConfig configure(const StateNode *node);

		/**
		 * Load shader configuration based on a given state (and joined states).
		 */
		static StateConfig configure(const State *state);

		StateConfigurer();

		/**
		 * @param cfg the shader configuration.
		 */
		StateConfigurer(const StateConfig &cfg);

		/**
		 * @param version the minimum GLSL version.
		 */
		void setVersion(GLuint version);

		/**
		 * Load shader configuration based on a given node (and parent nodes).
		 */
		void addNode(const StateNode *node);

		/**
		 * Load shader configuration based on a given state (and joined states).
		 */
		void addState(const State *state);

		/**
		 * Adds ShaderInput instance to StateConfig.
		 */
		void addInput(const std::string &name, const ref_ptr<ShaderInput> &in, const std::string &type = "");

		/**
		 * Add each key-value pair from given map to shader defines.
		 */
		void addDefines(const std::map<std::string, std::string> &defines);

		/**
		 * Add function declarations from given map.
		 */
		void addFunctions(const std::map<std::string, std::string> &functions);

		/**
		 * Add key-value pair to shader defines.
		 */
		void define(const std::string &name, const std::string &value);

		/**
		 * Add a function declaration.
		 */
		void defineFunction(const std::string &name, const std::string &value);

		/**
		 * @return the shader configuration.
		 */
		StateConfig &cfg();

	protected:
		StateConfig cfg_;
		std::map<std::string, ShaderInputList::iterator> inputNames_;
		GLuint numLights_;
	};
} // namespace

#endif /* STATE_CONFIGURER_H_ */

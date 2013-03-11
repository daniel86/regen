/*
 * shader-configurer.h
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#ifndef SHADER_CONFIGURER_H_
#define SHADER_CONFIGURER_H_

#include <ogle/states/state-node.h>
#include <ogle/states/state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/gl-types/shader-input.h>

namespace ogle {

/**
 * Let states and nodes configure generated shaders.
 */
class ShaderConfigurer
{
public:
  /**
   * Load shader configuration based on a given node (and parent nodes).
   */
  static ShaderState::Config configure(const StateNode *node);
  /**
   * Load shader configuration based on a given state (and joined states).
   */
  static ShaderState::Config configure(const State *state);

  ShaderConfigurer();
  ShaderConfigurer(const ShaderState::Config &cfg);

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
   * Add each key-value pair from given map to shader defines.
   */
  void addDefines(const map<string,string> &defines);
  /**
   * Add function declarations from given map.
   */
  void addFunctions(const map<string,string> &functions);

  /**
   * Add key-value pair to shader defines.
   */
  void define(const string &name, const string &value);
  /**
   * Add a function declaration.
   */
  void defineFunction(const string &name, const string &value);

  ShaderState::Config& cfg();

protected:
  ShaderState::Config cfg_;
  GLuint numLights_;
};

} // end ogle namespace

#endif /* SHADER_CONFIGURER_H_ */

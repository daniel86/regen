/*
 * shader-configurer.h
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#ifndef SHADER_CONFIGURER_H_
#define SHADER_CONFIGURER_H_

#include <ogle/render-tree/state-node.h>
#include <ogle/states/state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/gl-types/shader-input.h>

class ShaderConfigurer
{
public:
  /**
   * Load shader configuration based on a given node (and parent nodes).
   */
  static ShaderConfig configure(const StateNode *node);
  /**
   * Load shader configuration based on a given state (and joined states).
   */
  static ShaderConfig configure(const State *state);

  ShaderConfigurer();

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

  void define(const string &name, const string &value);
  void defineFunction(const string &name, const string &value);

  ShaderConfig& cfg();

protected:
  ShaderConfig cfg_;
};

#endif /* SHADER_CONFIGURER_H_ */

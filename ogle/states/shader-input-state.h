/*
 * shader-input-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef SHADER_INPUT_STATE_H_
#define SHADER_INPUT_STATE_H_

#include <ogle/states/state.h>
#include <ogle/gl-types/shader-input.h>
#include <ogle/gl-types/vbo.h>

namespace ogle {

/**
 * ShaderInput plus optional name overwrite.
 */
struct ShaderInputNamed {
  ShaderInputNamed(ref_ptr<ShaderInput> in, const string &name)
  : in_(in), name_(name) {}
  ref_ptr<ShaderInput> in_;
  string name_;
};
typedef list<ShaderInputNamed> ShaderInputContainer;
typedef ShaderInputContainer::const_iterator ShaderInputItConst;
typedef ShaderInputContainer::iterator ShaderInputIt;

/**
 * Provides vertex attributes.
 */
class ShaderInputState : public State
{
public:
  ShaderInputState();
  ShaderInputState(const ref_ptr<ShaderInput> &in, const string &name="");
  ~ShaderInputState();

  /**
   * Auto add to VBO when setInput() is called ?
   */
  void set_useVBOManager(GLboolean v);

  /**
   * vertex attributes.
   */
  ShaderInputContainer* inputsPtr();
  /**
   * vertex attributes.
   */
  const ShaderInputContainer& inputs() const;

  /**
   * Returns true if an attribute with given name was added.
   */
  GLboolean hasInput(const string &name) const;

  /**
   * Get attribute with specified name.
   */
  ShaderInputItConst getInput(const string &name) const;

  ref_ptr<ShaderInput> getInputPtr(const string &name);

  /**
   * Set a vertex attribute.
   */
  virtual ShaderInputItConst setInput(const ref_ptr<ShaderInput> &in, const string &name="");

protected:
  ShaderInputContainer inputs_;
  set<string> inputMap_;
  GLboolean useVBOManager_;

  void removeInput( const string &name );
  virtual void removeInput(const ref_ptr<ShaderInput> &att);
};

} // end ogle namespace

#endif /* ATTRIBUTE_STATE_H_ */

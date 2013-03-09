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
 * \brief Container State for shader input data.
 */
class ShaderInputState : public State
{
public:
  /**
   * ShaderInput plus optional name overwrite.
   */
  struct Named {
    Named(ref_ptr<ShaderInput> in, const string &name)
    : in_(in), name_(name) {}
    ref_ptr<ShaderInput> in_;
    string name_;
  };
  typedef list<Named> InputContainer;
  typedef InputContainer::const_iterator InputItConst;
  typedef InputContainer::iterator InputIt;

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
  InputContainer* inputsPtr();
  /**
   * vertex attributes.
   */
  const InputContainer& inputs() const;

  /**
   * Returns true if an attribute with given name was added.
   */
  GLboolean hasInput(const string &name) const;

  /**
   * Get attribute with specified name.
   */
  ref_ptr<ShaderInput> getInput(const string &name);

  /**
   * Set a vertex attribute.
   */
  virtual InputItConst setInput(const ref_ptr<ShaderInput> &in, const string &name="");

protected:
  InputContainer inputs_;
  set<string> inputMap_;
  GLboolean useVBOManager_;

  void removeInput( const string &name );
  virtual void removeInput(const ref_ptr<ShaderInput> &att);
};

} // end ogle namespace

#endif /* ATTRIBUTE_STATE_H_ */

/*
 * uniform.h
 *
 *  Created on: 02.04.2011
 *      Author: daniel
 */

#ifndef UNIFORM_H_
#define UNIFORM_H_

#include <string>
using namespace std;

#include <ogle/gl-types/vertex-attribute.h>
#include <ogle/algebra/vector.h>
#include <ogle/algebra/matrix.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/utility/stack.h>

/**
 * A uniform is a global GLSL variable declared with the "uniform"
 * storage qualifier. These act as parameters that the user
 * of a shader program can pass to that program.
 * They are stored in a program object.
 *
 * Uniforms are so named because they do not change from
 * one execution of a shader program to the next within
 * a particular rendering call.
 * This makes them unlike shader stage inputs and outputs,
 * which are often different for each invocation of a program stage.
 *
 * Intern the Uniform class uses a VertexAttribute,
 * but here it is used as instance attribute.
 * For each instance you can define a different uniform
 * value. The VertexAttribute can then be used with
 * GL_ARB_instanced_arrays.
 */
class Uniform
{
public:
  Uniform(const string &name,
      GLenum dataType=GL_FLOAT,
      GLuint dataTypeBytes=sizeof(GLfloat),
      GLuint valsPerElement=3,
      GLboolean normalize=false,
      GLuint elementCount=1,
      GLboolean isIntegerType=false);
  ~Uniform();

  /**
   * Name of the uniform used in shader programs.
   */
  const string& name() const { return name_; }
  /**
   * Name of the uniform used in shader programs.
   */
  void set_name(const string &s);

  /**
   * Number of uniform instance.
   * Each uniform can have different values
   * for instances.
   */
  GLuint numInstances() const;

  /**
   * Element count for uniform arrays.
   * If count==1 then no array is used.
   */
  GLuint count() const;

  /**
   * The current uniform value as VertexAttribute.
   */
  ref_ptr<VertexAttribute>& attribute();

  /**
   * Pops last value from stack.
   */
  void pop() {
    valueStack_.pop();
  }

  /**
   * Pushes a new value on the stack.
   */
  void pushNewAttribute();

  /**
   * Specify the value of a uniform variable
   * for the current program object.
   */
  virtual void apply(GLint loc) const = 0;

protected:
  typedef Stack< ref_ptr<VertexAttribute> > UniformStack;
  UniformStack valueStack_;
  GLuint numInstances_;
  GLuint divisor_;
  GLuint count_;
  GLenum dataType_;
  GLuint dataTypeBytes_;
  GLuint valsPerElement_;
  GLboolean normalize_;
  GLboolean isIntegerType_;
  string name_;

private:
  Uniform(const Uniform&);
  Uniform& operator=(const Uniform &other);
};

/**
 * Typed Uniform.
 */
template<class T>
class ValueUniform : public Uniform
{
public:
  ValueUniform(
      const string &name,
      GLenum dataType,
      GLuint dataTypeBytes,
      GLuint valsPerElement,
      GLboolean normalize,
      GLuint elementCount,
      GLboolean isIntegerType)
  : Uniform(name,dataType,dataTypeBytes,valsPerElement,
      normalize,elementCount,isIntegerType)
  {
  }

  /**
   * Get the first value of the first value array.
   */
  T &valuePtr() {
    byte *bytes = attribute()->dataPtr();
    return ((T*)bytes)[0];
  }
  /**
   * Get the first value of the first value array.
   */
  const T &value() const {
    const byte *bytes = valueStack_.top()->dataPtr();
    return ((T*)bytes)[0];
  }
  /**
   * Get the first value array.
   */
  T *valuePtrArray() {
    return (T*)attribute()->dataPtr();
  }
  /**
   * Get the first value array.
   */
  const T* valueArray() const
  {
    return (T*)valueStack_.top()->dataPtr();
  }

  /**
   * Pushes a single value.
   */
  void push(const T &value)
  {
    pushNewAttribute();
    set_value(value);
  }
  /**
   * Pushes a value array.
   */
  void push(const T *value)
  {
    pushNewAttribute();
    set_value(value);
  }
  /**
   * Pushes instanced value.
   */
  void pushInstanced(GLuint numInstances, GLuint divisor, const T *vals)
  {
    pushNewAttribute();
    set_value(numInstances, divisor, vals);
  }

  /**
   * Sets a single value.
   */
  void set_value(const T &value)
  {
    GLuint lastNumInstances = numInstances_;
    numInstances_ = 1;
    divisor_ = 1;
    const T vals[1] = {value};
    attribute()->setInstanceData(numInstances_, divisor_, (byte*) &vals[0]);
  }
  /**
   * Sets a value array.
   */
  void set_value(const T *vals)
  {
    GLuint lastNumInstances = numInstances_;
    numInstances_ = 1;
    divisor_ = 1;
    attribute()->setInstanceData(numInstances_, divisor_, (byte*) vals);
  }
  /**
   * Sets a instanced value.
   */
  void set_valuesInstanced(GLuint numInstances, GLuint divisor, const T *vals)
  {
    GLuint lastNumInstances = numInstances_;
    numInstances_ = max(1u,numInstances);
    divisor_ = max(1u,divisor);
    attribute()->setInstanceData(numInstances_, divisor_, (byte*) &vals[0]);
  }
};

/**
 * Integer Uniform.
 */
class UniformInt : public ValueUniform<GLint> {
  public:
    UniformInt(const string &name, const GLuint count=1);
    UniformInt(const string &name, const GLuint count, const GLint &val);
    virtual void apply(GLint loc) const;
};
/**
 * Integer[2] Uniform.
 */
class UniformIntV2 : public ValueUniform<Vec2i> {
  public:
    UniformIntV2(const string &name, const GLuint count=1);
    UniformIntV2(const string &name, const GLuint count, const Vec2i &val);
    virtual void apply(GLint loc) const;
};
/**
 * Integer[3] Uniform.
 */
class UniformIntV3 : public ValueUniform<Vec3i> {
  public:
    UniformIntV3(const string &name, const GLuint count=1);
    UniformIntV3(const string &name, const GLuint count, const Vec3i &val);
    virtual void apply(GLint loc) const;
};
/**
 * Integer[4] Uniform.
 */
class UniformIntV4 : public ValueUniform<Vec4i> {
  public:
    UniformIntV4(const string &name, const GLuint count=1);
    UniformIntV4(const string &name, const GLuint count, const Vec4i &val);
    virtual void apply(GLint loc) const;
};

/**
 * Float Uniform.
 */
class UniformFloat : public ValueUniform<GLfloat> {
  public:
    UniformFloat(const string &name, const GLuint count=1);
    UniformFloat(const string &name, const GLuint count, const GLfloat &val);
    virtual void apply(GLint loc) const;
};
/**
 * Float[2] Uniform.
 */
class UniformVec2 : public ValueUniform<Vec2f> {
  public:
    UniformVec2(const string &name, const GLuint count=1);
    UniformVec2(const string &name, const GLuint count, const Vec2f &val);
    virtual void apply(GLint loc) const;
};
/**
 * Float[3] Uniform.
 */
class UniformVec3 : public ValueUniform<Vec3f> {
  public:
    UniformVec3(const string &name, const GLuint count=1);
    UniformVec3(const string &name, const GLuint count, const Vec3f &val);
    virtual void apply(GLint loc) const;
};
/**
 * Float[4] Uniform.
 */
class UniformVec4 : public ValueUniform<Vec4f> {
  public:
    UniformVec4(const string &name, const GLuint count=1);
    UniformVec4(const string &name, const GLuint count, const Vec4f &val);
    virtual void apply(GLint loc) const;
};

/**
 * 3x3 Matrix Uniform.
 */
class UniformMat3 : public ValueUniform<Mat3f> {
  public:
    UniformMat3(const string &name, const GLuint count=1);
    UniformMat3(const string &name, const GLuint count, const Mat3f &val);
    virtual void apply(GLint loc) const;
};
/**
 * 4x4 Matrix Uniform.
 */
class UniformMat4 : public ValueUniform<Mat4f> {
  public:
    UniformMat4(const string &name, const GLuint count=1);
    UniformMat4(const string &name, const GLuint count, const Mat4f &val);
    virtual void apply(GLint loc) const;
};

#endif /* UNIFORM_H_ */

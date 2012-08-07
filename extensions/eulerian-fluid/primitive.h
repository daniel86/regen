/*
 * primitive.h
 *
 *  Created on: 24.02.2012
 *      Author: daniel
 */

#ifndef EULERIAN_PRIMITIVE_H_
#define EULERIAN_PRIMITIVE_H_

#include <shader.h>
#include <mesh.h>

class EulerianPrimitive {
public:
  EulerianPrimitive(
      GLuint width, GLuint height, GLuint depth,
      Mesh *updatePrimitive,
      UniformMat4 *unitOrthoMat,
      UniformFloat *deltaT,
      bool isLiquid, bool useHalfFloats=true);

  ref_ptr<Uniform> inverseSize() {
    return inverseSize_;
  }
  UniformFloat* timeStep() {
    return timeStep_;
  }

  bool is2D() const {
    return depth_<2;
  }
  bool isLiquid() const {
    return isLiquid_;
  }
  bool useHalfFloats() const {
    return useHalfFloats_;
  }

  int width() const {
    return width_;
  }
  int height() const {
    return height_;
  }
  int depth() const {
    return depth_;
  }

  ref_ptr<Shader> makeShader(
      const ShaderFunctions &func,
      const string &dimensionVec);
  void enableShader(ref_ptr<Shader> shader);

  void bind();
  void draw();

protected:
  GLuint width_, height_, depth_;
  Mesh *updatePrimitive_;
  UniformMat4 *unitOrthoMat_;
  UniformFloat *timeStep_;
  ref_ptr<Uniform> inverseSize_;
  bool isLiquid_;
  // use half floats for texture formats ?
  bool useHalfFloats_;
};

#endif /* EULERIAN_PRIMITIVE_H_ */

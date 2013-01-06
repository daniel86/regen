/*
 * picker.h
 *
 *  Created on: 03.09.2012
 *      Author: daniel
 */

#ifndef PICKER_H_
#define PICKER_H_

#include <ogle/animations/animation.h>
#include <ogle/render-tree/state-node.h>

/**
 * Implements geometry picking.
 * In the geometry shader faces are collected that intersect
 * with current mouse position. Using transform feedback
 * objects and instances can be distinguished.
 */
class Picker : public Animation
{
public:
  /**
   * GLUT mouse button event.
   */
  static GLuint PICK_EVENT;
  struct PickEvent {
    // associated MeshState
    MeshState *state;
    // identifies mesh
    GLint objectId;
    // identifies mesh instance
    GLint instanceId;
  };

  Picker(const ref_ptr<StateNode> &node, GLuint maxPickedObjects=999);
  virtual ~Picker();

  /**
   * Interval for updating the currently hovered object.
   */
  void set_pickInterval(GLdouble interval);

  // overwrite
  virtual void glAnimate(GLdouble dt);
  virtual void animate(GLdouble dt);
  virtual GLboolean useGLAnimation() const;
  virtual GLboolean useAnimation() const;
protected:
  static GLboolean pickerInitialled;
  static string pickerCode[3];
  static GLuint pickerShader[3];
  static void initPicker();

  GLdouble dt_;
  GLdouble pickInterval_;

  ref_ptr<StateNode> node_;
  map< Shader*, ref_ptr<Shader> > *shaderMap_;
  map< Shader*, ref_ptr<Shader> > *nextShaderMap_;

  vector<MeshState*> meshes_;
  MeshState *pickedMesh_;
  GLint pickedInstance_;

  ref_ptr<VertexBufferObject> feedbackBuffer_;
  ref_ptr<ShaderInput1i> pickObjectID_;
  GLuint countQuery_;
  GLint feedbackCount_;
  GLint lastFeedbackOffset_;

  ref_ptr<Shader> createPickShader(Shader *shader, GLenum in);

  Shader* getPickShader(Shader *shader, GLenum in);
  void pushPickShader();
  void popPickShader();

  void updatePickedObject();

  friend class PickerRenderState;
};

#endif /* PICKER_H_ */

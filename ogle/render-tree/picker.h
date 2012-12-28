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
#include <ogle/gl-types/geometry-shader-config.h>

class Picker : public Animation
{
public:
  /**
   * GLUT mouse button event.
   */
  static GLuint PICK_EVENT;
  struct PickEvent {
    MeshState *state;
    GLint objectId;
    GLint instanceId;
  };

  Picker(
      ref_ptr<StateNode> &node,
      GLuint maxPickedObjects=999);
  virtual ~Picker();

  void set_pickInterval(GLdouble interval);

  // overwrite
  virtual void animate(GLdouble dt);
  virtual void updateGraphics(GLdouble dt);
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

  ref_ptr<Shader> createPickShader(Shader *shader, GeometryShaderInput in);

  Shader* getPickShader(Shader *shader, GeometryShaderInput in);
  void pushPickShader();
  void popPickShader();

  void updatePickedObject();

  friend class PickerRenderState;
};

#endif /* PICKER_H_ */

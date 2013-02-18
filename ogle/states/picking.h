/*
 * picking.h
 *
 *  Created on: 17.02.2013
 *      Author: daniel
 */

#ifndef PICKING_H_
#define PICKING_H_

#include <ogle/states/state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/render-state.h>
#include <ogle/meshes/mesh-state.h>

class Picking : public State
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
  struct PickData {
    GLint objectID;
    GLint instanceID;
    GLfloat depth;
  };

  Picking();

  const MeshState* pickedMesh() const;
  GLint pickedInstance() const;
  GLint pickedObject() const;

protected:
  // currently hovered object
  MeshState *pickedMesh_;
  GLint pickedInstance_;
  GLint pickedObject_;

  void emitPickEvent();
};

//////////////
//////////////

class PickingGeom : public Picking, public RenderState
{
public:
  PickingGeom(GLuint maxPickedObjects=999);
  ~PickingGeom();

  virtual void enable(RenderState *rs=NULL);
  virtual void disable(RenderState *rs=NULL);

  virtual void pushShader(Shader *shader);
  virtual void popShader();
  virtual void pushMesh(MeshState *mesh);
  virtual void popMesh();

protected:
  // output target for picking geometry shader
  ref_ptr<VertexBufferObject> feedbackBuffer_;
  // counts number of hovered faces
  GLuint feedbackCount_;
  GLuint countQuery_;
  GLint lastFeedbackOffset_;

  ref_ptr<ShaderInput1i> pickObjectID_;
  deque< MeshState* > meshes_;

  map< Shader*, ref_ptr<Shader> > *shaderMap_;
  map< Shader*, ref_ptr<Shader> > *nextShaderMap_;
  string pickerCode_;
  GLuint pickerShader_;

  void initPicker();
  void updatePickedObject();

  ref_ptr<Shader> createPickShader(Shader *shader);
  Shader* getPickShader(Shader *shader);
};

#endif /* PICKING_H_ */

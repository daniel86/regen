/*
 * picking.h
 *
 *  Created on: 17.02.2013
 *      Author: daniel
 */

#ifndef PICKING_H_
#define PICKING_H_

#include <regen/states/state.h>
#include <regen/states/shader-state.h>
#include <regen/states/state-node.h>
#include <regen/meshes/mesh-state.h>

namespace regen {
/**
 * \brief Implements geometry picking.
 *
 * Uses special geometry shader that checks if the mouse intersects
 * the processed face. If an intersection occurred the object id, instance id
 * and face depth are written to output variables. These outputs are captured
 * using transform feedback. Finally a CPU depth test is used to obtain the
 * front-most face intersection.
 */
class PickingGeom : public State, public Animation
{
public:
  /**
   * GLUT mouse button event.
   */
  static GLuint PICK_EVENT;
  /**
   * \brief Selection changed event.
   */
  class PickEvent : public EventData {
  public:
    /** associated MeshState */
    Mesh *state;
    /** identifies mesh */
    GLint objectId;
    /** identifies mesh instance */
    GLint instanceId;
  };

  /**
   * @param maxPickedObjects maximum number of pickable objects intersection each other on the screen.
   */
  PickingGeom(GLuint maxPickedObjects=999);
  ~PickingGeom();

  /**
   * @param interval defines the frequency the selection is updated
   */
  void set_pickInterval(GLdouble interval);

  /**
   * @return currently hovered mesh.
   */
  const Mesh* pickedMesh() const;
  /**
   * @return instance id of currently hovered mesh.
   */
  GLint pickedInstance() const;
  /**
   * @return object id of currently hovered mesh.
   */
  GLint pickedObject() const;

  /**
   * Adds a pickable mesh.
   * @param mesh the pickable mesh
   * @param meshNode the mesh node that will be traversed
   * @param meshShader the regular mesh shader
   * @return GL_TRUE on success
   */
  GLboolean add(
      const ref_ptr<Mesh> &mesh,
      const ref_ptr<StateNode> &meshNode,
      const ref_ptr<Shader> &meshShader);
  /**
   * Removes previously added mesh.
   */
  void remove(Mesh *mesh);

  /**
   * Update selection and emit PickEvent when selection changed.
   * @param rs the render state
   */
  void update(RenderState *rs);

  // override
  void glAnimate(RenderState *rs, GLdouble dt);

protected:
  struct PickData {
    GLint objectID;
    GLint instanceID;
    GLfloat depth;
  };
  // Maps meshes to traversed nodes, pick shader
  // and object id.
  struct PickMesh {
    ref_ptr<Mesh> mesh_;
    ref_ptr<StateNode> meshNode_;
    ref_ptr<Shader> pickShader_;
    GLint id_;
  };

  // currently hovered object
  Mesh *pickedMesh_;
  GLint pickedInstance_;
  GLint pickedObject_;

  // output target for picking geometry shader
  ref_ptr<VertexBufferObject> feedbackBuffer_;
  BufferRange bufferRange_;
  GLuint countQuery_;

  map<GLint,PickMesh> meshes_;
  map<Mesh*,GLint> meshToID_;
  // object id shader input
  ref_ptr<ShaderInput1i> pickObjectID_;
  // contains last associated id
  GLint pickMeshID_;

  GLdouble dt_;
  GLdouble pickInterval_;

  // GLSL picker code
  string pickerCode_;
  // picker geometry shader handle
  GLuint pickerShader_;

  void emitPickEvent();
  void updatePickedObject(RenderState *rs, GLuint feedbackCount);
  ref_ptr<Shader> createPickShader(Shader *shader);
};

} // namespace

#endif /* PICKING_H_ */

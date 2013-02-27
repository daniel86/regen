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
#include <ogle/states/state-node.h>
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

class PickingGeom : public Picking
{
public:
  PickingGeom(GLuint maxPickedObjects=999);
  ~PickingGeom();

  GLboolean add(
      const ref_ptr<MeshState> &mesh,
      const ref_ptr<StateNode> &meshNode,
      const ref_ptr<Shader> &meshShader);
  void remove(MeshState *mesh);

  void update(RenderState *rs);

protected:
  // Maps meshes to traversed nodes, pick shader
  // and object id.
  struct PickMesh {
    ref_ptr<MeshState> mesh_;
    ref_ptr<StateNode> meshNode_;
    ref_ptr<Shader> pickShader_;
    GLint id_;
  };

  // output target for picking geometry shader
  ref_ptr<VertexBufferObject> feedbackBuffer_;
  GLuint countQuery_;

  map<GLint,PickMesh> meshes_;
  map<MeshState*,GLint> meshToID_;
  // object id shader input
  ref_ptr<ShaderInput1i> pickObjectID_;
  // contains last associated id
  GLint pickMeshID_;

  // GLSL picker code
  string pickerCode_;
  // picker geometry shader handle
  GLuint pickerShader_;

  void updatePickedObject(GLuint feedbackCount);

  ref_ptr<Shader> createPickShader(Shader *shader);
};

#endif /* PICKING_H_ */

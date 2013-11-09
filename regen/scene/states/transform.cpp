/*
 * transform.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "transform.h"
using namespace regen::scene;
using namespace regen;

#include <regen/scene/value-generator.h>
#include <regen/scene/resource-manager.h>

#define REGEN_TRANSFORM_STATE_CATEGORY "transform"

static void transformMatrix(
    const string &target, Mat4f &mat, const Vec3f &value)
{
  if(target=="translate") {
    mat.x[12] += value.x;
    mat.x[13] += value.y;
    mat.x[14] += value.z;
  }
  else if(target=="scale") {
    mat.scale(value);
  }
  else if(target=="rotate") {
    Quaternion q(0.0,0.0,0.0,1.0);
    q.setEuler(value.x,value.y,value.z);
    mat *= q.calculateMatrix();
  }
  else if(target == "set") {}
  else {
    REGEN_WARN("Unknown distribute target '" << target << "'.");
  }
}

static void transformMatrix(
    SceneInputNode &input, Mat4f *matrices, GLuint numInstances)
{
  const list< ref_ptr<SceneInputNode> > &childs = input.getChildren();
  for(list< ref_ptr<SceneInputNode> >::const_iterator
      it=childs.begin(); it!=childs.end(); ++it)
  {
    ref_ptr<SceneInputNode> child = *it;
    list<GLuint> indices = child->getIndexSequence(numInstances);

    if(child->getCategory() == "set") {
      ValueGenerator<Vec3f> generator(child.get(),indices.size(),
          child->getValue<Vec3f>("value",Vec3f(0.0f)));
      const string target = child->getValue<string>("target","translate");

      for(list<GLuint>::iterator it=indices.begin(); it!=indices.end(); ++it) {
        transformMatrix(target, matrices[*it], generator.next());
      }
    }
    else {
      for(list<GLuint>::iterator it=indices.begin(); it!=indices.end(); ++it) {
        transformMatrix(child->getCategory(), matrices[*it],
            child->getValue<Vec3f>("value",Vec3f(0.0f)));
      }
    }
  }
}

TransformStateProvider::TransformStateProvider()
: StateProcessor(REGEN_TRANSFORM_STATE_CATEGORY)
{}

void TransformStateProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<State> &state)
{
  bool isInstanced    = input.getValue<bool>("is-instanced",false);
  GLuint numInstances = input.getValue<GLuint>("num-instances",1u);

  ref_ptr<ModelTransformation> transform = ref_ptr<ModelTransformation>::alloc();

  // Handle instanced model matrix
  if(isInstanced && numInstances>1) {
    transform->modelMat()->setInstanceData(numInstances,1,NULL);
    Mat4f* matrices = (Mat4f*)transform->modelMat()->clientDataPtr();
    for(GLuint i=0; i<numInstances; i+=1) matrices[i] = Mat4f::identity();
    transformMatrix(input,matrices,numInstances);
    // add data to vbo
    transform->setInput(transform->modelMat());
  }
  else {
    Mat4f* matrices = (Mat4f*)transform->modelMat()->clientDataPtr();
    transformMatrix(input,matrices,1u);
  }

  if(input.hasAttribute("shape")) {
    for(GLuint i=0; i<numInstances; ++i) {
      // Synchronize ModelTransformation and physics simulation.
      ref_ptr<ModelMatrixMotion> motion =
          ref_ptr<ModelMatrixMotion>::alloc(transform->modelMat(),i);
      ref_ptr<PhysicalProps> props =
          createPhysicalObject(parser,input,motion);
      if(props.get()) {
        parser->getPhysics()->addObject(
            ref_ptr<PhysicalObject>::alloc(props));
      }
    }
  }

  state->joinStates(transform);
}

static ref_ptr<ShaderInput> getMeshPositions(
    SceneParser *parser, SceneInputNode &input)
{
  ref_ptr<MeshVector> meshes = parser->getResources()->getMesh(parser, input.getValue("mesh"));
  ref_ptr<ShaderInput> out;

  if(meshes.get()==NULL || meshes->empty()) {
    REGEN_WARN("Unable to find Mesh for '" << input.getDescription() << "'.");
  }
  else if(meshes->size()>1) {
    REGEN_WARN("Unable to handle multiple Meshes for '" << input.getDescription() << "'.");
  }
  else {
    out = (*meshes->begin())->positions();
  }
  return out;
}

static ref_ptr<Mesh> getMesh(
    SceneParser *parser, SceneInputNode &input)
{
  ref_ptr<MeshVector> meshes = parser->getResources()->getMesh(parser, input.getValue("mesh"));
  ref_ptr<Mesh> out;

  if(meshes.get()==NULL || meshes->empty()) {
    REGEN_WARN("Unable to find Mesh for '" << input.getDescription() << "'.");
  }
  else if(meshes->size()>1) {
    REGEN_WARN("Unable to handle multiple Meshes for '" << input.getDescription() << "'.");
  }
  else {
    out = (*meshes->begin());
  }
  return out;
}

ref_ptr<PhysicalProps> TransformStateProvider::createPhysicalObject(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<btMotionState> &motion)
{
  const string shapeName(input.getValue("shape"));
  GLfloat mass = input.getValue<GLfloat>("mass",1.0f);

  ref_ptr<PhysicalProps> props;
  // Primitives
  if(shapeName=="sphere") {
    GLfloat radius = input.getValue<GLfloat>("radius",1.0f)*0.5;
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btSphereShape>::alloc(radius));
  }
  else if(shapeName=="wall") {
    Vec2f size = input.getValue<Vec2f>("size",Vec2f(1.0f));
    btVector3 halfExtend(size.x*0.5, 0.001, size.y*0.5);
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btBoxShape>::alloc(halfExtend));
    mass = 0.0;
  }
  else if(shapeName=="infinite-wall") {
    Vec3f planeNormal = input.getValue<Vec3f>("normal",Vec3f(0.0f,1.0f,0.0f));
    GLfloat planeConstant = input.getValue<GLfloat>("constant",GLfloat(0.0f));
    btVector3 planeNormal_(planeNormal.x,planeNormal.y,planeNormal.z);
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btStaticPlaneShape>::alloc(planeNormal_,planeConstant));
    mass = 0.0;
  }
  else if(shapeName=="box") {
    Vec3f size = input.getValue<Vec3f>("size",Vec3f(1.0f));
    btVector3 halfExtend(size.x*0.5, size.y*0.5, size.z*0.5);
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btBoxShape>::alloc(halfExtend));
  }
  else if(shapeName=="cylinder") {
    Vec3f size = input.getValue<Vec3f>("size",Vec3f(1.0f));
    btVector3 halfExtend(size.x*0.5, size.y*0.5, size.z*0.5);
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btCylinderShape>::alloc(halfExtend));
  }
  else if(shapeName=="capsule") {
    GLfloat radius = input.getValue<GLfloat>("radius",1.0f);
    GLfloat height = input.getValue<GLfloat>("height",1.0f);
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btCapsuleShape>::alloc(radius,height));
  }
  else if(shapeName=="cone") {
    GLfloat radius = input.getValue<GLfloat>("radius",1.0f);
    GLfloat height = input.getValue<GLfloat>("height",1.0f);
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btConeShape>::alloc(radius,height));
  }
  else if(shapeName=="convex-hull") {
    ref_ptr<ShaderInput> pos = getMeshPositions(parser,input);
    if(pos.get()!=NULL) {
      if(!pos->hasClientData() && !pos->hasServerData()) {
        REGEN_WARN("Mesh '" << input.getValue("mesh") <<
            "' has no position data available.");
      }
      else {
        bool loadServerData = !pos->hasClientData();
        if(loadServerData) pos->readServerData();

        const btScalar *points = (const btScalar*) pos->clientData();
        props = ref_ptr<PhysicalProps>::alloc(
            motion, ref_ptr<btConvexHullShape>::alloc(points,pos->numVertices()));

        if(loadServerData) pos->deallocateClientData();
      }
    }
  }
  else if(shapeName=="triangle-mesh") {
    ref_ptr<Mesh> mesh = getMesh(parser,input);
    ref_ptr<ShaderInput> pos     = (mesh.get()==NULL ?
        ref_ptr<ShaderInput>() : mesh->positions());
    ref_ptr<ShaderInput> indices = (mesh.get()==NULL ?
        ref_ptr<ShaderInput>() : mesh->inputContainer()->indices());

    if(pos.get()!=NULL && indices.get()!=NULL) {
      btIndexedMesh btMesh;
      btMesh.m_numVertices = pos->numVertices();
      btMesh.m_vertexStride = pos->elementSize();

      switch(mesh->primitive()) {
      case GL_TRIANGLES:
        btMesh.m_triangleIndexStride = 3*indices->elementSize();
        btMesh.m_numTriangles = indices->numVertices()/3;
        break;
      case GL_TRIANGLE_STRIP:
        btMesh.m_triangleIndexStride = 1*indices->elementSize();
        btMesh.m_numTriangles = indices->numVertices()-2;
        break;
      default:
        btMesh.m_numTriangles = -1;
        btMesh.m_triangleIndexStride = -1;
        break;
      }

      if(btMesh.m_numTriangles>0) {
        if(!pos->hasClientData()) pos->readServerData();
        if(!indices->hasClientData()) indices->readServerData();
        btMesh.m_vertexBase        = pos->clientData();
        btMesh.m_triangleIndexBase = indices->clientData();

        PHY_ScalarType indexType;
        switch(indices->dataType()) {
        case GL_FLOAT:
          indexType = PHY_FLOAT;
          break;
        case GL_DOUBLE:
          indexType = PHY_DOUBLE;
          break;
        case GL_SHORT:
          indexType = PHY_SHORT;
          break;
        case GL_INT:
        case GL_UNSIGNED_INT:
        default:
          indexType = PHY_INTEGER;
          break;
        }

        const bool useQuantizedAabbCompression = true;

        btTriangleIndexVertexArray *btMeshIface = new btTriangleIndexVertexArray;
        btMeshIface->addIndexedMesh(btMesh, indexType);
        ref_ptr<btBvhTriangleMeshShape> shape =
            ref_ptr<btBvhTriangleMeshShape>::alloc(btMeshIface,useQuantizedAabbCompression);
        props = ref_ptr<PhysicalProps>::alloc(motion, shape);
      }
      else {
        REGEN_WARN("Unsupported primitive for btTriangleIndexVertexArray "
            << "in input node " << input.getDescription() << "'.");
      }
    }
  }
  else {
    REGEN_WARN("Ignoring unknown physical shape '" << input.getDescription() << "'.");
    return ref_ptr<PhysicalProps>();
  }

  Vec3f inertia = input.getValue<Vec3f>("inertia",Vec3f(0.0f));
  props->setMassProps(mass, btVector3(inertia.x,inertia.x,inertia.z));

  props->setRestitution(
      input.getValue<GLfloat>("restitution",0.0f));

  props->setLinearSleepingThreshold(
      input.getValue<GLfloat>("linear-sleeping-threshold",0.8f));
  props->setAngularSleepingThreshold(
      input.getValue<GLfloat>("angular-sleeping-threshold",1.0f));

  props->setFriction(
      input.getValue<GLfloat>("friction",0.5f));
  props->setRollingFriction(
      input.getValue<GLfloat>("rolling-friction",0.0f));

  props->setAdditionalDamping(
      input.getValue<bool>("additional-damping",false));
  props->setAdditionalDampingFactor(
      input.getValue<GLfloat>("additional-damping-factor",0.005f));

  props->setLinearDamping(
      input.getValue<GLfloat>("linear-damping",0.0f));
  props->setAdditionalLinearDampingThresholdSqr(
      input.getValue<GLfloat>("additional-linear-damping-threshold",0.01f));

  props->setAngularDamping(
      input.getValue<GLfloat>("angular-damping",0.0f));
  props->setAdditionalAngularDampingFactor(
      input.getValue<GLfloat>("additional-angular-damping-factor",0.01f));
  props->setAdditionalAngularDampingThresholdSqr(
      input.getValue<GLfloat>("additional-angular-damping-threshold",0.01f));

  if(mass>0) props->calculateLocalInertia();

  return props;
}

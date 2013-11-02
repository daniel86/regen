/*
 * scene-xml.cpp
 *
 *  Created on: Oct 26, 2013
 *      Author: daniel
 */

#include <regen/config.h>

#include <regen/states/state-configurer.h>
#include <regen/states/fbo-state.h>
#include <regen/states/blend-state.h>
#include <regen/states/blit-state.h>
#include <regen/states/material-state.h>
#include <regen/states/texture-state.h>
#include <regen/states/shader-state.h>
#include <regen/states/depth-state.h>
#include <regen/states/blend-state.h>
#include <regen/states/atomic-states.h>
#include <regen/states/depth-of-field.h>
#include <regen/states/tonemap.h>
#include <regen/states/tesselation-state.h>
#include <regen/states/picking.h>

#include <regen/meshes/mesh-state.h>
#include <regen/meshes/rectangle.h>
#include <regen/meshes/sky.h>
#include <regen/meshes/box.h>
#include <regen/meshes/sphere.h>
#include <regen/meshes/texture-mapped-text.h>
#include <regen/meshes/particle-cloud.h>
#include <regen/meshes/assimp-importer.h>

#include <regen/shading/light-pass.h>
#include <regen/shading/shading-direct.h>

#include <regen/gl-types/gl-enum.h>
#include <regen/gl-types/texture.h>

#include <regen/textures/texture-loader.h>

#include <regen/utility/font.h>
#include <regen/utility/filesystem.h>

#include "scene-xml.h"

template<class T> class Distributor {
public:
  Distributor(rapidxml::xml_node<> *n,
      const GLuint numValues,
      const T &initialValue=T(0))
  : n_(n),
    numValues_(numValues),
    counter_(Vec4ui(0u)),
    value_(initialValue)
  {
    mode_ = xml::readAttribute<string>(n_,"mode","row");
  }

  T next() {
    if(mode_=="row") return nextRow();
    else if(mode_=="fade") return nextFade();
    else if(mode_=="random") return nextRandom();
    else if(mode_=="constant") return value_;
    REGEN_WARN("Unknown distribute mode '" << mode_ << "'.");
    return value_;
  }

protected:
  rapidxml::xml_node<> *n_;
  GLuint numValues_;
  Vec4ui counter_;
  T value_;
  string mode_;

  T nextRow() {
    const T stepX = xml::readAttribute<T>(n_,"x-step",T(1));
    const T stepY = xml::readAttribute<T>(n_,"y-step",T(1));
    const T stepZ = xml::readAttribute<T>(n_,"z-step",T(1));
    value_ = stepX*counter_.x + stepY*counter_.y + stepZ*counter_.z;

    GLuint xCount = xml::readAttribute<GLuint>(n_,"x-count",numValues_);
    GLuint yCount = xml::readAttribute<GLuint>(n_,"y-count",1);
    counter_.x += 1;
    if(counter_.x >= xCount) {
      counter_.x = 0;
      counter_.y += 1;
      if(counter_.y >= yCount) {
        counter_.y = 0;
        counter_.z += 1;
      }
    }
    return value_;
  }

  T nextRandom() {
    const T min = xml::readAttribute<T>(n_,"min",T(0));
    const T max = xml::readAttribute<T>(n_,"max",T(1));
    value_ = min + (max-min)*((rand()%10000)/10000.0);
    counter_.x += 1;
    return value_;
  }

  T nextFade() {
    const T start = xml::readAttribute<T>(n_,"start",T(1.0f));
    const T stop = xml::readAttribute<T>(n_,"stop",T(2.0f));
    const GLfloat progress = ((GLfloat)counter_.x)/((GLfloat)numValues_);
    value_ = start + (stop-start)*progress;
    counter_.x += 1;
    return value_;
  }
};

// Resizes Framebuffer texture when the window size changed
class FBOResizer : public EventHandler
{
public:
  FBOResizer(const ref_ptr<FBO> &fbo, GLfloat wScale, GLfloat hScale)
  : EventHandler(), fbo_(fbo), wScale_(wScale), hScale_(hScale) { }

  void call(EventObject *evObject, EventData*) {
    Application *app = (Application*)evObject;
    const Vec2i& winSize = app->windowViewport()->getVertex(0);
    fbo_->resize(winSize.x*wScale_, winSize.y*hScale_, 1);
  }

protected:
  ref_ptr<FBO> fbo_;
  GLfloat wScale_, hScale_;
};

// Updates Camera Projection when window size changes
class ProjectionUpdater : public EventHandler
{
public:
  ProjectionUpdater(const ref_ptr<Camera> &cam,
      GLfloat fov, GLfloat near, GLfloat far)
  : EventHandler(), cam_(cam), fov_(fov), near_(near), far_(far) { }

  void call(EventObject *evObject, EventData*) {
    Application *app = (Application*)evObject;
    const Vec2i& winSize = app->windowViewport()->getVertex(0);
    GLfloat aspect = winSize.x/(GLfloat)winSize.y;

    Mat4f &view = *(Mat4f*)cam_->view()->dataPtr();
    Mat4f &viewInv = *(Mat4f*)cam_->viewInverse()->dataPtr();
    Mat4f &proj = *(Mat4f*)cam_->projection()->dataPtr();
    Mat4f &projInv = *(Mat4f*)cam_->projectionInverse()->dataPtr();
    Mat4f &viewproj = *(Mat4f*)cam_->viewProjection()->dataPtr();
    Mat4f &viewprojInv = *(Mat4f*)cam_->viewProjectionInverse()->dataPtr();

    proj = Mat4f::projectionMatrix(fov_, aspect, near_, far_);
    projInv = proj.projectionInverse();
    viewproj = view * proj;
    viewprojInv = projInv * viewInv;
    cam_->projection()->nextStamp();
    cam_->projectionInverse()->nextStamp();
    cam_->viewProjection()->nextStamp();
    cam_->viewProjectionInverse()->nextStamp();
  }

protected:
  ref_ptr<Camera> cam_;
  GLfloat fov_, near_, far_;
};

class SortByModelMatrix : public State
{
public:
  SortByModelMatrix(const ref_ptr<StateNode> &n, const ref_ptr<Camera> &cam, GLboolean frontToBack)
  : State(), n_(n), comparator_(cam,frontToBack) {}

  virtual void enable(RenderState *state) {
    n_->childs().sort(comparator_);
  }
protected:
  ref_ptr<StateNode> n_;
  NodeEyeDepthComparator comparator_;
};

///////////////
///////////////

static ref_ptr<PhysicalProps> createPhysicalObject(
    rapidxml::xml_node<> *xmlNode, const ref_ptr<ModelTransformation> &transform)
{
  rapidxml::xml_attribute<> *shapeAtt = xmlNode->first_attribute("shape");
  const string shapeName(shapeAtt->value());

  Vec3f rot = xml::readAttribute<Vec3f>(xmlNode,"rotation",Vec3f(0.0f,0.0f,0.0f));
  Quaternion q(0.0,0.0,0.0,1.0);
  q.setEuler(rot.x,rot.y,rot.z);

  Vec3f pos = xml::readAttribute<Vec3f>(xmlNode,"position",Vec3f(0.0f,10.0f,0.0f));
  // Synchronize ModelTransformation and physics simulation.
  ref_ptr<ModelMatrixMotion> motion = ref_ptr<ModelMatrixMotion>::alloc(btTransform(
      btQuaternion(q.x,q.y,q.z,q.w),
      btVector3(pos.x,pos.y,pos.z)),
      transform);
  GLfloat mass = xml::readAttribute<GLfloat>(xmlNode,"mass",1.0f);

  ref_ptr<PhysicalProps> props;
  // Primitives
  if(shapeName=="sphere") {
    GLfloat radius = xml::readAttribute<GLfloat>(xmlNode,"radius",1.0f)*0.5;
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btSphereShape>::alloc(radius));
  }
  else if(shapeName=="wall") {
    Vec2f size = xml::readAttribute<Vec2f>(xmlNode,"size",Vec2f(1.0f));
    btVector3 halfExtend(size.x*0.5, 0.001, size.y*0.5);
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btBoxShape>::alloc(halfExtend));
    mass = 0.0;
  }
  else if(shapeName=="infinite-wall") {
    Vec3f planeNormal = xml::readAttribute<Vec3f>(xmlNode,"normal",Vec3f(0.0f,1.0f,0.0f));
    GLfloat planeConstant = xml::readAttribute<GLfloat>(xmlNode,"constant",GLfloat(0.0f));
    btVector3 planeNormal_(planeNormal.x,planeNormal.y,planeNormal.z);
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btStaticPlaneShape>::alloc(planeNormal_,planeConstant));
    mass = 0.0;
  }
  else if(shapeName=="box") {
    Vec3f size = xml::readAttribute<Vec3f>(xmlNode,"size",Vec3f(1.0f));
    btVector3 halfExtend(size.x*0.5, size.y*0.5, size.z*0.5);
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btBoxShape>::alloc(halfExtend));
  }
  else if(shapeName=="cylinder") {
    Vec3f size = xml::readAttribute<Vec3f>(xmlNode,"size",Vec3f(1.0f));
    btVector3 halfExtend(size.x*0.5, size.y*0.5, size.z*0.5);
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btCylinderShape>::alloc(halfExtend));
  }
  else if(shapeName=="capsule") {
    GLfloat radius = xml::readAttribute<GLfloat>(xmlNode,"radius",1.0f);
    GLfloat height = xml::readAttribute<GLfloat>(xmlNode,"height",1.0f);
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btCapsuleShape>::alloc(radius,height));
  }
  else if(shapeName=="cone") {
    GLfloat radius = xml::readAttribute<GLfloat>(xmlNode,"radius",1.0f);
    GLfloat height = xml::readAttribute<GLfloat>(xmlNode,"height",1.0f);
    props = ref_ptr<PhysicalProps>::alloc(
        motion, ref_ptr<btConeShape>::alloc(radius,height));
  }
  else {
    REGEN_WARN("Ignoring unknown physical shape '" << shapeName << "'.");
    return ref_ptr<PhysicalProps>();
  }
  // TODO: Handle Physical Meshes....
  // - btConvexHullShape
  // - btConvexTriangleMeshShape
  // - btBvhTriangleMeshShape
  // - btHeightfieldTerrainShape
  // - btStaticPlaneShape

  Vec3f inertia = xml::readAttribute<Vec3f>(xmlNode,"inertia",Vec3f(0.0f));
  props->setMassProps(mass, btVector3(inertia.x,inertia.x,inertia.z));

  props->setRestitution(
      xml::readAttribute<GLfloat>(xmlNode,"restitution",0.0f));

  props->setLinearSleepingThreshold(
      xml::readAttribute<GLfloat>(xmlNode,"linear-sleeping-threshold",0.8f));
  props->setAngularSleepingThreshold(
      xml::readAttribute<GLfloat>(xmlNode,"angular-sleeping-threshold",1.0f));

  props->setFriction(
      xml::readAttribute<GLfloat>(xmlNode,"friction",0.5f));
  props->setRollingFriction(
      xml::readAttribute<GLfloat>(xmlNode,"rolling-friction",0.0f));

  props->setAdditionalDamping(
      xml::readAttribute<bool>(xmlNode,"additional-damping",false));
  props->setAdditionalDampingFactor(
      xml::readAttribute<GLfloat>(xmlNode,"additional-damping-factor",0.005f));

  props->setLinearDamping(
      xml::readAttribute<GLfloat>(xmlNode,"linear-damping",0.0f));
  props->setAdditionalLinearDampingThresholdSqr(
      xml::readAttribute<GLfloat>(xmlNode,"additional-linear-damping-threshold",0.01f));

  props->setAngularDamping(
      xml::readAttribute<GLfloat>(xmlNode,"angular-damping",0.0f));
  props->setAdditionalAngularDampingFactor(
      xml::readAttribute<GLfloat>(xmlNode,"additional-angular-damping-factor",0.01f));
  props->setAdditionalAngularDampingThresholdSqr(
      xml::readAttribute<GLfloat>(xmlNode,"additional-angular-damping-threshold",0.01f));

  if(mass>0) props->calculateLocalInertia();

  return props;
}

static void pushIndexToSequence(
    GLuint numIndices, list<GLuint> &indices, GLuint index)
{
  if(index>=numIndices) {
    REGEN_WARN("Invalid index: " << index << ">=" << numIndices << ".");
    return;
  }
  indices.push_back(index);
}

static list<GLuint> getIndexSequence(
    rapidxml::xml_node<> *n, GLuint numIndices)
{
  list<GLuint> indices;
  if(n->first_attribute("index")!=NULL) {
    pushIndexToSequence(numIndices,indices,
        xml::readAttribute<GLint>(n,"index",0u));
  }
  else if(n->first_attribute("from-index")!=NULL ||
          n->first_attribute("to-index")!=NULL) {
    GLuint from = xml::readAttribute<GLuint>(n,"from-index",0u);
    GLuint to = xml::readAttribute<GLuint>(n,"to-index",numIndices-1);
    GLuint step = xml::readAttribute<GLuint>(n,"index-step",1u);
    for(GLuint i=from; i<=to; i+=step) {
      pushIndexToSequence(numIndices,indices,i);
    }
  }
  else if(n->first_attribute("indices")!=NULL) {
    const string indicesAtt = xml::readAttribute<string>(n,"indices","0");
    vector<string> indicesStr;
    boost::split(indicesStr,indicesAtt,boost::is_any_of(","));
    for(vector<string>::iterator
        it=indicesStr.begin(); it!=indicesStr.end(); ++it)
      pushIndexToSequence(numIndices,indices,atoi(it->c_str()));
  }
  else if(n->first_attribute("random-indices")!=NULL) {
    GLuint indexCount = xml::readAttribute<GLuint>(n,"random-indices",numIndices);
    while(indexCount>0) {
      --indexCount;
      pushIndexToSequence(numIndices,indices,rand()%numIndices);
    }
  }

  if(indices.empty()) {
    for(GLuint i=0; i<numIndices; i+=1) {
      pushIndexToSequence(numIndices,indices,i);
    }
  }
  return indices;
}

static void transformMatrix(
    const string &target, Mat4f &mat, const Vec3f &value)
{
  if(target=="translate") {
    mat.x[12] += value.x;
    mat.x[13] += value.y;
    mat.x[14] += value.z;
  }
  else if(target=="scale") {
    mat *= Mat4f::scaleMatrix(value);
  }
  else if(target=="rotate") {
    Quaternion q(0.0,0.0,0.0,1.0);
    q.setEuler(value.x,value.y,value.z);
    mat *= q.calculateMatrix();
  }
  else if(target == "distribute") {}
  else {
    REGEN_WARN("Unknown distribute target '" << target << "'.");
  }
}

static void transformMatrix(
    rapidxml::xml_node<> *n, Mat4f *matrices, GLuint numInstances)
{
  for(rapidxml::xml_node<> *child=n->first_node();
      child!=NULL; child=child->next_sibling())
  {
    list<GLuint> indices = getIndexSequence(child,numInstances);
    string nodeTag(child->name());

    if(nodeTag == "distribute") {
      Distributor<Vec3f> distributor(child,indices.size(),
          xml::readAttribute<Vec3f>(child,"value",Vec3f(0.0f)));
      const string target = xml::readAttribute<string>(child,"target","translate");

      for(list<GLuint>::iterator it=indices.begin(); it!=indices.end(); ++it) {
        transformMatrix(target, matrices[*it], distributor.next());
      }
    }
    else {
      for(list<GLuint>::iterator it=indices.begin(); it!=indices.end(); ++it) {
        transformMatrix(nodeTag, matrices[*it],
            xml::readAttribute<Vec3f>(child,"value",Vec3f(0.0f)));
      }
    }
  }
}

////////////////////////
////////////////////////
////////////////////////

static bool isShadowCaster(const string &id,
    const vector<string> &shadowMaps)
{
  const string invID = REGEN_STRING("!" << id);
  bool matches = false;
  for(vector<string>::const_iterator it=shadowMaps.begin();
      it!=shadowMaps.end(); ++it)
  {
    const string &x = *it;
    if(x == "*" || x == id) {
      matches = true;
    }
    else if(x == invID) {
      matches = false;
    }
  }
  return matches;
}

template<class U, class T>
static ref_ptr<U> createShaderInput(
    rapidxml::xml_node<> *n, const T &defaultValue)
{
  rapidxml::xml_attribute<> *nameAtt = n->first_attribute("name");
  if(nameAtt==NULL) {
    REGEN_WARN("No name specified for shader input.");
    return ref_ptr<U>();
  }
  ref_ptr<U> v = ref_ptr<U>::alloc(nameAtt->value());
  v->set_isConstant(xml::readAttribute<bool>(n, "is-constant", false));

  GLuint numInstances = xml::readAttribute<GLuint>(n,"num-instances",1u);
  GLuint numVertices = xml::readAttribute<GLuint>(n,"num-vertices",1u);
  bool isInstanced = xml::readAttribute<bool>(n,"is-instanced",false) && numInstances>1;
  bool isAttribute = xml::readAttribute<bool>(n,"is-attribute",false) && numVertices>1;
  GLuint count=0;

  if(isInstanced) {
    v->setInstanceData(numInstances,1,NULL);
    count = numInstances;
  }
  else if(isAttribute) {
    v->setVertexData(numVertices,NULL);
    count = numVertices;
  }
  else {
    v->setUniformData(xml::readAttribute<T>(n,"value",defaultValue));
  }

  // Handle instanced ShaderInput
  if(isInstanced || isAttribute) {
    T *values = (T*)v->dataPtr();
    for(GLuint i=0; i<count; i+=1) values[i] = defaultValue;

    for(rapidxml::xml_node<> *child=n->first_node();
        child!=NULL; child=child->next_sibling())
    {
      list<GLuint> indices = getIndexSequence(child,count);
      string nodeTag(child->name());

      if(nodeTag == "distribute") {
        const string target = xml::readAttribute<string>(child,"target","translate");

        Distributor<T> distributor(child,indices.size(),
            xml::readAttribute<T>(child,"value",T(0)));
        for(list<GLuint>::iterator it=indices.begin(); it!=indices.end(); ++it) {
          values[*it] += distributor.next();
        }
      }
      else {
        REGEN_WARN("Unknown input tag '" << nodeTag << "'.");
      }
    }
  }
  
  return v;
}

static ref_ptr<ShaderInput> createShaderInput(rapidxml::xml_node<> *xmlNode)
{
  const string type = xml::readAttribute<string>(xmlNode, "type", "");
  if(type == "int") {
    return createShaderInput<ShaderInput1i,GLint>(xmlNode,GLint(0));
  }
  else if(type == "ivec2") {
    return createShaderInput<ShaderInput2i,Vec2i>(xmlNode,Vec2i(0));
  }
  else if(type == "ivec3") {
    return createShaderInput<ShaderInput3i,Vec3i>(xmlNode,Vec3i(0));
  }
  else if(type == "ivec4") {
    return createShaderInput<ShaderInput4i,Vec4i>(xmlNode,Vec4i(0));
  }
  else if(type == "uint") {
    return createShaderInput<ShaderInput1ui,GLuint>(xmlNode,GLuint(0));
  }
  else if(type == "uvec2") {
    return createShaderInput<ShaderInput2ui,Vec2ui>(xmlNode,Vec2ui(0));
  }
  else if(type == "uvec3") {
    return createShaderInput<ShaderInput3ui,Vec3ui>(xmlNode,Vec3ui(0));
  }
  else if(type == "uvec4") {
    return createShaderInput<ShaderInput4ui,Vec4ui>(xmlNode,Vec4ui(0));
  }
  else if(type == "float") {
    return createShaderInput<ShaderInput1f,GLfloat>(xmlNode,GLfloat(0));
  }
  else if(type == "vec2") {
    return createShaderInput<ShaderInput2f,Vec2f>(xmlNode,Vec2f(0));
  }
  else if(type == "vec3") {
    return createShaderInput<ShaderInput3f,Vec3f>(xmlNode,Vec3f(0));
  }
  else if(type == "vec4") {
    return createShaderInput<ShaderInput4f,Vec4f>(xmlNode,Vec4f(0));
  }
  else {
    REGEN_WARN("Unknown input type '" << type << "'.");
    return ref_ptr<ShaderInput>();
  }
}

static string getID(rapidxml::xml_node<> *xmlNode) {
  rapidxml::xml_attribute<> *idAtt = xmlNode->first_attribute("id");
  return (idAtt==NULL ? REGEN_STRING(rand()) : string(idAtt->value()));
}
static rapidxml::xml_node<>* getRootNode(rapidxml::xml_node<> *xmlNode) {
  rapidxml::xml_node<> *root = xmlNode;
  while(root->parent()!=NULL) root=root->parent();
  return root;
}
static rapidxml::xml_node<>* findNode(
    rapidxml::xml_node<> *root,
    const string &nodeTag,
    const string &nodeId)
{
  for(rapidxml::xml_node<> *n=root->first_node(nodeTag.c_str());
      n!=NULL; n= n->next_sibling(nodeTag.c_str()))
  {
    rapidxml::xml_attribute<> *idAtt = xml::loadAttribute(n,"id");
    if(idAtt!=NULL && nodeId.compare(string(idAtt->value()))==0) return n;
  }
  return NULL;
}

static string getResPath(const string &relPath) {
  PathChoice texPaths;
  texPaths.choices_.push_back(filesystemPath(
      REGEN_SOURCE_DIR, relPath));
  texPaths.choices_.push_back(filesystemPath(filesystemPath(
      REGEN_SOURCE_DIR, "regen"), relPath));
  texPaths.choices_.push_back(filesystemPath(filesystemPath(
      REGEN_SOURCE_DIR, "applications"), relPath));
  texPaths.choices_.push_back(filesystemPath(filesystemPath(
      REGEN_INSTALL_PREFIX, "share"), relPath));
  return texPaths.firstValidPath();
}

static Vec3i getSize(
    const Application *app,
    const string &sizeMode, const Vec3f &size) {
  if(sizeMode == "abs") {
    return Vec3i(size.x,size.y,size.z);
  }
  else if(sizeMode=="rel") {
    Vec2i viewport = app->windowViewport()->getVertex(0);
    return Vec3i(
        (GLint)(size.x*viewport.x),
        (GLint)(size.y*viewport.y),1);
  }
  else {
    REGEN_WARN("Unknown size mode '" << sizeMode << "'.");
    return Vec3i(size.x,size.y,size.z);
  }
}

////////////////////////
////////////////////////

SceneXML::SceneXML(Application *app, const string &sceneFile)
: app_(app),
  xmlInput_(sceneFile.c_str()),
  buffer_((
      istreambuf_iterator<char>(xmlInput_)),
      istreambuf_iterator<char>())
{
  physics_ = ref_ptr<BulletPhysics>::alloc();
  buffer_.push_back('\0');
  doc_.parse<0>( &buffer_[0] );
}
SceneXML::~SceneXML()
{
  doc_.clear();
  buffer_.clear();
  xmlInput_.close();
}

void SceneXML::processDocument(
    const ref_ptr<StateNode> &parent, const string &nodeId)
{
  rapidxml::xml_node<> *sceneNode = findNode(&doc_, "node", nodeId);
  if(sceneNode==NULL) {
    REGEN_ERROR("Unable to find node with id '" << nodeId << "' in XML document.");
    return;
  }
  processNode(parent,sceneNode);
}

////////////////////////
///// XML State's below
////////////////////////

void SceneXML::processToggleNode(
    const ref_ptr<State> &state,
    rapidxml::xml_node<> *xmlNode)
{
  rapidxml::xml_attribute<> *toggleAtt = xmlNode->first_attribute("key");
  if(toggleAtt==NULL) {
    REGEN_WARN("Ignoring toggle without key attribute.");
    return;
  }
  RenderState::Toggle key = xml::readAttribute<RenderState::Toggle>(
      xmlNode,"key",RenderState::CULL_FACE);
  state->joinStates(ref_ptr<ToggleState>::alloc(key,
      xml::readAttribute<bool>(xmlNode,"value",true)));
}

void SceneXML::processCullNode(
    const ref_ptr<State> &state,
    rapidxml::xml_node<> *xmlNode)
{
  const string mode = xml::readAttribute<string>(xmlNode, "mode", "back");
  state->joinStates(ref_ptr<CullFaceState>::alloc(glenum::cullFace(mode)));
}

void SceneXML::processDepthNode(
    const ref_ptr<State> &state,
    rapidxml::xml_node<> *xmlNode)
{
  ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();

  depth->set_useDepthTest(
      xml::readAttribute<bool>(xmlNode,"test",true));
  depth->set_useDepthWrite(
      xml::readAttribute<bool>(xmlNode,"write",true));

  rapidxml::xml_attribute<> *rangeAtt = xmlNode->first_attribute("range");
  if(rangeAtt!=NULL) {
    Vec2f range;
    stringstream ss(rangeAtt->value());
    ss >> range;
    depth->set_depthRange(range.x, range.y);
  }

  rapidxml::xml_attribute<> *functionAtt = xmlNode->first_attribute("function");
  if(functionAtt!=NULL) {
    depth->set_depthFunc(glenum::depthFunction(functionAtt->value()));
  }

  state->joinStates(depth);
}

void SceneXML::processBlendNode(
    const ref_ptr<State> &state,
    rapidxml::xml_node<> *xmlNode)
{
  ref_ptr<BlendState> blend = ref_ptr<BlendState>::alloc(
      xml::readAttribute<BlendMode>(xmlNode, "mode", BLEND_MODE_SRC));
  if(xmlNode->first_attribute("color")) {
    blend->setBlendColor(xml::readAttribute<Vec4f>(xmlNode,"color",Vec4f(0.0f)));
  }
  state->joinStates(blend);
}

void SceneXML::processBlitNode(
    const ref_ptr<State> &state,
    rapidxml::xml_node<> *xmlNode)
{
  rapidxml::xml_attribute<> *fboAtt = xmlNode->first_attribute("fbo");
  if(fboAtt==NULL) {
    REGEN_WARN("Ignoring blit without fbo attribute.");
    return;
  }
  ref_ptr<FBO> fbo = getFBO(fboAtt->value());
  if(fbo.get()==NULL) {
    REGEN_WARN("Unable to find fbo with name '" << fboAtt->value() << "'.");
    return;
  }
  GLuint attachment = xml::readAttribute<GLuint>(xmlNode, "attachment", 0u);

  ref_ptr<BlitToScreen> blit = ref_ptr<BlitToScreen>::alloc(fbo,
      app_->windowViewport(), GL_COLOR_ATTACHMENT0+attachment);
  state->joinStates(blit);
}

void SceneXML::processDefineNode(
    const ref_ptr<State> &state,
    rapidxml::xml_node<> *xmlNode)
{
  rapidxml::xml_attribute<> *keyAtt = xmlNode->first_attribute("key");
  rapidxml::xml_attribute<> *valAtt = xmlNode->first_attribute("value");
  if(keyAtt==NULL) {
    REGEN_WARN("Ignoring define without key attribute.");
    return;
  }
  if(valAtt==NULL) {
    REGEN_WARN("Ignoring define without value attribute.");
    return;
  }
  ref_ptr<State> s = state;
  while(!s->joined().empty()) {
    s = *s->joined().rbegin();
  }
  s->shaderDefine(keyAtt->value(), valAtt->value());
}

void SceneXML::processInputNode(
    const ref_ptr<State> &state, rapidxml::xml_node<> *xmlNode)
{
  ref_ptr<State> s = state;
  while(!s->joined().empty()) {
    s = *s->joined().rbegin();
  }

  HasInput *x = dynamic_cast<HasInput*>(s.get());
  if(x==NULL) {
    ref_ptr<HasInputState> inputState = ref_ptr<HasInputState>::alloc();
    inputState->setInput(createShaderInput(xmlNode));
    state->joinStates(inputState);
  }
  else {
    x->setInput(createShaderInput(xmlNode));
  }
}

void SceneXML::processTransformNode(
    const ref_ptr<State> &state, rapidxml::xml_node<> *n)
{
  bool isInstanced = xml::readAttribute<bool>(n,"is-instanced",false);
  GLuint numInstances = xml::readAttribute<GLuint>(n,"num-instances",1u);

  ref_ptr<ModelTransformation> transform = ref_ptr<ModelTransformation>::alloc();

  // Handle instanced model matrix
  if(isInstanced && numInstances>1) {
    transform->modelMat()->setInstanceData(numInstances,1,NULL);
    Mat4f* matrices = (Mat4f*)transform->modelMat()->dataPtr();
    for(GLuint i=0; i<numInstances; i+=1) matrices[i] = Mat4f::identity();
    transformMatrix(n,matrices,numInstances);
    // add data to vbo
    transform->setInput(transform->modelMat());
  }
  else {
    Mat4f* matrices = (Mat4f*)transform->modelMat()->dataPtr();
    transformMatrix(n,matrices,1u);
  }

  if(n->first_attribute("shape")!=NULL) {
    // TODO: shape instancing....
    ref_ptr<PhysicalProps> props = createPhysicalObject(n,transform);
    if(props.get()) {
      physics_->addObject(ref_ptr<PhysicalObject>::alloc(props));
    }
  }

  state->joinStates(transform);
}

void SceneXML::processMaterialNode(
    const ref_ptr<State> &state,
    rapidxml::xml_node<> *xmlNode)
{
  ref_ptr<Material> mat = ref_ptr<Material>::alloc();

  rapidxml::xml_attribute<> *presetAtt = xmlNode->first_attribute("preset");
  rapidxml::xml_attribute<> *assetAtt = xmlNode->first_attribute("asset");
  if(assetAtt!=NULL) {
    ref_ptr<AssimpImporter> assetLoader = getAsset(assetAtt->value());
    if(assetLoader.get()==NULL) {
      REGEN_WARN("Unknown Asset id '" << assetAtt->value() << "'.");
    }
    else {
      const vector< ref_ptr<Material> > materials = assetLoader->materials();
      GLuint materialIndex =
          xml::readAttribute<GLuint>(xmlNode,"asset-index",0u);
      if(materialIndex >=materials.size()) {
        REGEN_WARN("Invalid Material index '" << materialIndex << "'.");
      } else {
        mat = materials[materialIndex];
      }
    }
  }
  else if(presetAtt!=NULL) {
    string presetVal(presetAtt->value());
    if(presetVal == "jade")        mat->set_jade();
    else if(presetVal == "ruby")   mat->set_ruby();
    else if(presetVal == "chrome") mat->set_chrome();
    else if(presetVal == "gold")   mat->set_gold();
    else if(presetVal == "copper") mat->set_copper();
    else if(presetVal == "silver") mat->set_silver();
    else if(presetVal == "pewter") mat->set_pewter();
    else {
      REGEN_WARN("Unknown Material preset '" << presetVal << "'.");
      mat->set_copper();
    }
  }
  if(xmlNode->first_attribute("ambient")!=NULL)
    mat->ambient()->setVertex(0,
        xml::readAttribute<Vec3f>(xmlNode, "ambient", Vec3f(0.0f)));
  if(xmlNode->first_attribute("diffuse")!=NULL)
    mat->diffuse()->setVertex(0,
        xml::readAttribute<Vec3f>(xmlNode, "diffuse", Vec3f(1.0f)));
  if(xmlNode->first_attribute("specular")!=NULL)
    mat->specular()->setVertex(0,
        xml::readAttribute<Vec3f>(xmlNode, "specular", Vec3f(0.0f)));
  if(xmlNode->first_attribute("shininess")!=NULL)
    mat->shininess()->setVertex(0,
        xml::readAttribute<GLfloat>(xmlNode, "shininess", 1.0f));

  mat->alpha()->setVertex(0,
      xml::readAttribute<GLfloat>(xmlNode, "alpha", 1.0f));
  mat->refractionIndex()->setVertex(0,
      xml::readAttribute<GLfloat>(xmlNode, "refractionIndex", 0.95f));
  mat->set_fillMode(glenum::fillMode(
      xml::readAttribute<string>(xmlNode, "fillMode", "FILL")));
  if(xml::readAttribute<bool>(xmlNode,"twoSided",false)) {
    // this conflicts with shadow mapping front face culling.
    mat->set_twoSided(true);
  }

  state->joinStates(mat);
}

static vector<string> getFBOAttachments(rapidxml::xml_node<> *n)
{
  vector<string> out;
  string attachments = xml::readAttribute<string>(n, "attachments", "");
  if(attachments.empty()) {
    REGEN_WARN("No attachments specified.");
  } else {
    boost::split(out,attachments,boost::is_any_of(","));
  }
  return out;
}

void SceneXML::processFBONode(
    const ref_ptr<State> &state,
    rapidxml::xml_node<> *xmlNode)
{
  ref_ptr<FBO> fbo = getFBO(getID(xmlNode));
  if(fbo.get()==NULL) {
    REGEN_WARN("Unable to find fbo with name '" << getID(xmlNode) << "'.");
    return;
  }
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::alloc(fbo);

  for(rapidxml::xml_node<> *n=xmlNode->first_node();
      n!=NULL; n=n->next_sibling())
  {
    string nodeTag(n->name());

    if(nodeTag == "clear-depth") {
      fboState->setClearDepth();
    }
    else if(nodeTag == "clear-buffer") {
      vector<string> idVec = getFBOAttachments(n);
      vector<GLenum> buffers(idVec.size());

      ClearColorState::Data data;
      data.clearColor = xml::readAttribute<Vec4f>(n, "clear-color", Vec4f(0.0));

      for(GLuint i=0u; i<idVec.size(); ++i) {
        buffers[i] = GL_COLOR_ATTACHMENT0 + atoi(idVec[i].c_str());
      }
      data.colorBuffers = DrawBuffers(buffers);

      fboState->setClearColor(data);
    }
    else if(nodeTag == "draw-buffer") {
      vector<string> idVec = getFBOAttachments(n);
      vector<GLenum> buffers(idVec.size());
      for(GLuint i=0u; i<idVec.size(); ++i) {
        fboState->addDrawBuffer(GL_COLOR_ATTACHMENT0 + atoi(idVec[i].c_str()));
      }
    }
    else {
      REGEN_WARN("Skipping unknown node tag '" << n->name() << "'.");
    }
  }

  state->joinStates(fboState);
}

void SceneXML::processTextureNode(
    const ref_ptr<State> &state,
    rapidxml::xml_node<> *xmlNode)
{
  const string texName = xml::readAttribute<string>(xmlNode, "name", "");
  rapidxml::xml_attribute<> *idAtt = xmlNode->first_attribute("id");
  rapidxml::xml_attribute<> *fboAtt = xmlNode->first_attribute("fbo");
  rapidxml::xml_attribute<> *filterAtt = xmlNode->first_attribute("filter");

  ref_ptr<Texture> tex;
  // Find the texture resource
  if(idAtt != NULL) {
    tex = getTexture(idAtt->value());
  }
  else if(fboAtt != NULL) {
    ref_ptr<FBO> fbo = getFBO(fboAtt->value());
    if(fbo.get()==NULL) {
      REGEN_WARN("Unable to find FBO with id '" << fboAtt->value() << "'.");
      return;
    }
    const string val = xml::readAttribute<string>(xmlNode, "attachment", "0");
    if(val == "depth") {
      tex = fbo->depthTexture();
    }
    else {
      vector< ref_ptr<Texture> > &textures = fbo->colorTextures();
      unsigned int attachment = atoi(val.c_str());
      if(attachment < textures.size()) {
        tex = textures[attachment];
      }
      else {
        REGEN_WARN("Invalid attachment '" << val << "' for FBO texture.");
        return;
      }
    }
  }
  else if(filterAtt != NULL) {
    ref_ptr<FilterSequence> filter = filter_[filterAtt->value()];
    if(filter.get()==NULL) {
      REGEN_WARN("No filter named '" << filterAtt->value() <<
          "' known to node '" << getID(xmlNode) << "'.");
      return;
    }
    else {
      tex = filter->output();
    }
  }
  if(tex.get()==NULL) {
    REGEN_WARN("Skipping unidentified texture node.");
    return;
  }

  // Set-Up the texture state
  ref_ptr<TextureState> texState = ref_ptr<TextureState>::alloc(tex, texName); {
    texState->set_ignoreAlpha(
        xml::readAttribute<bool>(xmlNode, "ignore-alpha", false));
    texState->set_mapTo(xml::readAttribute<TextureState::MapTo>(
        xmlNode, "map-to", TextureState::MAP_TO_CUSTOM));

    // Describes how a texture will be mixed with existing pixels.
    texState->set_blendMode(
        xml::readAttribute<BlendMode>(xmlNode, "blend-mode", BLEND_MODE_SRC));
    texState->set_blendFactor(
        xml::readAttribute<GLfloat>(xmlNode, "blend-factor", 1.0f));
    const string blendFunctionName =
        xml::readAttribute<string>(xmlNode, "blend-function-name", "");
    rapidxml::xml_attribute<> *blendFunctionAtt =
        xmlNode->first_attribute("blend-function");
    if(blendFunctionAtt!=NULL) {
      texState->set_blendFunction(blendFunctionAtt->value(),blendFunctionName);
    }

    // Defines how a texture should be mapped on geometry.
    texState->set_mapping(xml::readAttribute<TextureState::Mapping>(
        xmlNode, "mapping", TextureState::MAPPING_TEXCO));
    const string mappingFunctionName =
        xml::readAttribute<string>(xmlNode, "mapping-function-name", "");
    rapidxml::xml_attribute<> *mappingFunctionAtt =
        xmlNode->first_attribute("mapping-function");
    if(mappingFunctionAtt!=NULL) {
      texState->set_mappingFunction(mappingFunctionAtt->value(),mappingFunctionName);
    }

    // texel transfer wraps sampled texels before returning them.
    const string texelTransferName =
        xml::readAttribute<string>(xmlNode, "texel-transfer-name", "");
    rapidxml::xml_attribute<> *texelTransferKeyAtt =
        xmlNode->first_attribute("texel-transfer-key");
    rapidxml::xml_attribute<> *texelTransferFunctionAtt =
        xmlNode->first_attribute("texel-transfer-function");
    if(texelTransferKeyAtt!=NULL) {
      texState->set_texelTransferKey(texelTransferKeyAtt->value(),texelTransferName);
    }
    else if(texelTransferFunctionAtt!=NULL) {
      texState->set_texelTransferFunction(texelTransferFunctionAtt->value(),texelTransferName);
    }

    // texel transfer wraps computed texture coordinates before returning them.
    if(xmlNode->first_attribute("texco-transfer")!=NULL) {
      texState->set_texcoTransfer(xml::readAttribute<TextureState::TransferTexco>(
          xmlNode, "texco-transfer", TextureState::TRANSFER_TEXCO_RELIEF));
    }
    const string texcoTransferName =
        xml::readAttribute<string>(xmlNode, "texco-transfer-name", "");
    rapidxml::xml_attribute<> *texcoTransferKeyAtt =
        xmlNode->first_attribute("texco-transfer-key");
    rapidxml::xml_attribute<> *texcoTransferFunctionAtt =
        xmlNode->first_attribute("texco-transfer-function");
    if(texcoTransferKeyAtt!=NULL) {
      texState->set_texcoTransferKey(texcoTransferKeyAtt->value(),texcoTransferName);
    }
    else if(texcoTransferFunctionAtt!=NULL) {
      texState->set_texcoTransferFunction(texcoTransferFunctionAtt->value(),texcoTransferName);
    }
  }

  state->joinStates(texState);
}

void SceneXML::processCameraNode(
    const ref_ptr<State> &state,
    rapidxml::xml_node<> *xmlNode)
{
  rapidxml::xml_attribute<> *idAtt = xmlNode->first_attribute("id");
  if(idAtt==NULL) {
    REGEN_WARN("Unable to load camera, missing id.");
    return;
  }
  ref_ptr<Camera> cam = getCamera(idAtt->value());
  if(cam.get()==NULL) {
    REGEN_WARN("Unable to load camera, id '" << idAtt->value() << "' unknown.");
    return;
  }
  state->joinStates(cam);
}

////////////////////////
///// XML StateNode's below
////////////////////////

void SceneXML::processStateNode(
    const ref_ptr<State> &state,
    rapidxml::xml_node<> *n)
{
  string nodeTag(n->name());
  if(nodeTag.compare("fbo")==0) {
    processFBONode(state,n);
  }
  else if(nodeTag.compare("blit")==0) {
    processBlitNode(state,n);
  }
  else if(nodeTag.compare("blend")==0) {
    processBlendNode(state,n);
  }
  else if(nodeTag.compare("depth")==0) {
    processDepthNode(state,n);
  }
  else if(nodeTag.compare("transform")==0) {
    processTransformNode(state,n);
  }
  else if(nodeTag.compare("cull")==0) {
    processCullNode(state,n);
  }
  else if(nodeTag.compare("input")==0) {
    processInputNode(state,n);
  }
  else if(nodeTag.compare("define")==0) {
    processDefineNode(state,n);
  }
  else if(nodeTag.compare("toggle")==0) {
    processToggleNode(state,n);
  }
  else if(nodeTag.compare("texture")==0) {
    processTextureNode(state,n);
  }
  else if(nodeTag.compare("material")==0) {
    processMaterialNode(state,n);
  }
  else if(nodeTag.compare("camera")==0) {
    processCameraNode(state,n);
  }
  else {
    REGEN_WARN("Skipping unknown node tag '" << nodeTag << "'.");
  }
}

void SceneXML::processNode(
    const ref_ptr<StateNode> &parent,
    rapidxml::xml_node<> *xmlNode,
    const ref_ptr<State> &state)
{
  rapidxml::xml_attribute<> *importAtt = xmlNode->first_attribute("import");

  if(importAtt==NULL) {

    ref_ptr<StateNode> newNode = ref_ptr<StateNode>::alloc(state);
    parent->addChild(newNode);
    nodes_[getID(xmlNode)] = newNode;

    // Sort node children by model view matrix.
    GLuint sortMode = xml::readAttribute<GLuint>(xmlNode,"sort",0);
    if(sortMode!=0u) {
      ref_ptr<Camera> sortCam = getCamera(xml::readAttribute<string>(xmlNode,"sort-camera",""));
      if(sortCam.get()!=NULL) {
        state->joinStatesFront(
            ref_ptr<SortByModelMatrix>::alloc(newNode,sortCam,(sortMode==1)));
      }
    }

    for(rapidxml::xml_node<> *n=xmlNode->first_node();
        n!=NULL; n= n->next_sibling())
    {
      string nodeTag(n->name());

      if(nodeTag == "node") {
        processNode(newNode,n);
      }
      else if(nodeTag.compare("texture-box")==0) {
        processTextureBoxNode(newNode,n);
      }
      else if(nodeTag.compare("text-box")==0) {
        processTextBoxNode(newNode,n);
      }
      else if(nodeTag.compare("state-sequence")==0) {
        processStateSequenceNode(newNode,n);
      }
      else if(nodeTag.compare("filter-sequence")==0) {
        processFilterSequenceNode(newNode,n);
      }
      else if(nodeTag.compare("mesh")==0) {
        processMeshNode(newNode,n);
      }
      else if(nodeTag.compare("fullscreen-pass")==0) {
        processFullscreenPassNode(newNode,n);
      }
      else if(nodeTag.compare("light-pass")==0) {
        processLightPassNode(newNode,n);
      }
      else if(nodeTag.compare("direct-shading")==0) {
        processDirectShadingNode(newNode,n);
      }
      else {
        processStateNode(state,n);
      }
    }

    rapidxml::xml_attribute<> *shadowMapsAtt = xmlNode->first_attribute("shadow-maps");
    if(shadowMapsAtt!=NULL) {
      addShadowCaster(newNode,shadowMapsAtt->value());
    }
  }
  else {
    rapidxml::xml_node<> *importNode = findNode(
        getRootNode(xmlNode), "node", importAtt->value());
    if(importNode==NULL) {
      REGEN_WARN("Unable to import node '" << importAtt->value() << "'.");
    } else {
      processNode(parent,importNode);
    }
  }
}

void SceneXML::processStateSequenceNode(
    const ref_ptr<StateNode> &parent,
    rapidxml::xml_node<> *xmlNode)
{
  processNode(parent, xmlNode, ref_ptr<StateSequence>::alloc());
}

void SceneXML::processFilterSequenceNode(
    const ref_ptr<StateNode> &parent,
    rapidxml::xml_node<> *xmlNode)
{
  rapidxml::xml_attribute<> *filterID = xmlNode->first_attribute("id");
  rapidxml::xml_attribute<> *textureID = xmlNode->first_attribute("texture");
  rapidxml::xml_attribute<> *fboID = xmlNode->first_attribute("fbo");
  if(filterID==NULL) {
    REGEN_WARN("Ignoring filter without id.");
    return;
  }
  if(textureID==NULL && fboID==NULL) {
    REGEN_WARN("Ignoring filter without input texture.");
    return;
  }

  ref_ptr<Texture> input;
  if(textureID!=NULL) {
    input = getTexture(textureID->value());
    if(input.get()==NULL) {
      REGEN_WARN("Unable to find Texture with ID " << textureID->value() << ".");
      return;
    }
  }
  else {
    ref_ptr<FBO> fbo = getFBO(fboID->value());
    if(fbo.get()==NULL) {
      REGEN_WARN("Unable to find FBO with ID " << fboID->value() << ".");
      return;
    }
    GLuint attachment = xml::readAttribute<GLuint>(xmlNode,"attachment",0);
    vector< ref_ptr<Texture> > textures = fbo->colorTextures();
    if(attachment >= textures.size()) {
      REGEN_WARN("FBO has not " << attachment << " attachments.");
      return;
    }
    input = textures[attachment];
  }

  bool bindInput = xml::readAttribute<bool>(xmlNode,"bind-input",true);
  ref_ptr<FilterSequence> filterSeq = ref_ptr<FilterSequence>::alloc(input,bindInput);

  filterSeq->set_format(glenum::textureFormat(
      xml::readAttribute<string>(xmlNode, "format", "NONE")));
  filterSeq->set_internalFormat(glenum::textureInternalFormat(
      xml::readAttribute<string>(xmlNode, "internal-format", "NONE")));
  filterSeq->set_pixelType(glenum::pixelType(
      xml::readAttribute<string>(xmlNode, "pixel-type", "NONE")));

  for(rapidxml::xml_node<> *n=xmlNode->first_node("filter");
      n!=NULL; n=n->next_sibling("filter"))
  {
    rapidxml::xml_attribute<> *shaderAtt = n->first_attribute("shader");
    if(shaderAtt==NULL) {
      REGEN_WARN("Ignoring filter without shader.");
      continue;
    }
    GLfloat scaleFactor = xml::readAttribute<GLfloat>(xmlNode,"scale",1.0f);
    filterSeq->addFilter(ref_ptr<Filter>::alloc(shaderAtt->value(),scaleFactor));
  }

  parent->state()->joinStates(filterSeq);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(parent.get());
  filterSeq->createShader(shaderConfigurer.cfg());

  filter_[filterID->value()] = filterSeq;
}

void SceneXML::processFullscreenPassNode(
    const ref_ptr<StateNode> &parent,
    rapidxml::xml_node<> *xmlNode)
{
  rapidxml::xml_attribute<> *shaderAtt = xmlNode->first_attribute("shader");
  if(shaderAtt==NULL) {
    REGEN_WARN("Missing shader attribute for fullscreen-pass node.");
    return;
  }
  const string shaderKey = shaderAtt->value();

  ref_ptr<FullscreenPass> fs = ref_ptr<FullscreenPass>::alloc(shaderKey);
  ref_ptr<StateNode> node = ref_ptr<StateNode>::alloc(fs);
  parent->addChild(node);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  fs->createShader(shaderConfigurer.cfg());
}

void SceneXML::processLightPassNode(
    const ref_ptr<StateNode> &parent,
    rapidxml::xml_node<> *xmlNode)
{
  rapidxml::xml_attribute<> *shaderAtt = xmlNode->first_attribute("shader");
  if(shaderAtt==NULL) {
    REGEN_WARN("Missing shader attribute for light-pass node.");
    return;
  }
  const string shaderKey = shaderAtt->value();
  Light::Type lightType =
      xml::readAttribute<Light::Type>(xmlNode, "type", Light::SPOT);

  ref_ptr<LightPass> x = ref_ptr<LightPass>::alloc(lightType,shaderKey);

  bool useShadows =
      xml::readAttribute<bool>(xmlNode, "use-shadows", true);
  if(useShadows) {
    x->setShadowFiltering(xml::readAttribute<ShadowMap::FilterMode>(
        xmlNode, "shadow-filter", ShadowMap::FILTERING_NONE));
    x->setShadowLayer(xml::readAttribute<GLuint>(xmlNode,"shadow-layer",1));
  }

  for(rapidxml::xml_node<> *n=xmlNode->first_node("light");
      n!=NULL; n=n->next_sibling("light"))
  {
    list< ref_ptr<ShaderInput> > inputs;
    ref_ptr<ShadowMap> shadowMap;

    rapidxml::xml_attribute<> *idAtt = n->first_attribute("id");
    if(idAtt==NULL) {
      REGEN_WARN("Missing id attribute for light-pass light.");
      continue;
    }
    ref_ptr<Light> light = getLight(idAtt->value());
    if(light.get()==NULL) {
      REGEN_WARN("Unable to find light with id '" << idAtt->value() << "'.");
      continue;
    }

    rapidxml::xml_attribute<> *shadowAtt = n->first_attribute("shadow-map");
    if(shadowAtt!=NULL) {
      if(!useShadows) {
        REGEN_WARN("Light pass has no use-shadows attribute.");
      }
      else {
        const string shadowMapID(shadowAtt->value());
        shadowMap = getShadowMap(shadowMapID);
        if(shadowMap.get()==NULL) {
          REGEN_WARN("Unable to find shadow-map with id '" << shadowAtt->value() << "'.");
        }
      }
    }
    else if(useShadows) {
      REGEN_WARN("Light '" << idAtt->value() << "' has no associated shadow.");
      continue;
    }

    for(rapidxml::xml_node<> *m=n->first_node("input");
        m!=NULL; m=m->next_sibling("input"))
    {
      inputs.push_back(createShaderInput(m));
    }

    x->addLight(light,shadowMap,inputs);
  }

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(parent.get());
  x->createShader(shaderConfigurer.cfg());

  parent->state()->joinStates(x);
}

void SceneXML::processDirectShadingNode(
    const ref_ptr<StateNode> &parent,
    rapidxml::xml_node<> *xmlNode)
{
  rapidxml::xml_node<> *lightsNode = xmlNode->first_node("direct-lights");
  rapidxml::xml_node<> *passNode = xmlNode->first_node("direct-pass");
  if(lightsNode==NULL) {
    REGEN_WARN("Missing direct-lights node for direct-shading.");
    return;
  }
  if(passNode==NULL) {
    REGEN_WARN("Missing direct-pass node for direct-shading.");
    return;
  }

  ref_ptr<DirectShading> shadingState = ref_ptr<DirectShading>::alloc();
  ref_ptr<StateNode> shadingNode = ref_ptr<StateNode>::alloc(shadingState);
  parent->addChild(shadingNode);

  rapidxml::xml_attribute<> *ambientAtt = xmlNode->first_attribute("ambient");
  if(ambientAtt!=NULL) {
    shadingState->ambientLight()->setVertex(0,
        xml::readAttribute<Vec3f>(xmlNode,"ambient",Vec3f(0.1f)));
  }

  // load lights
  for(rapidxml::xml_node<> *n=lightsNode->first_node("light");
      n!=NULL; n=n->next_sibling("light"))
  {
    rapidxml::xml_attribute<> *idAtt = n->first_attribute("id");
    if(idAtt==NULL) {
      REGEN_WARN("Missing id attribute for direct-shading light.");
      continue;
    }
    ref_ptr<Light> light = getLight(idAtt->value());
    if(light.get()==NULL) {
      REGEN_WARN("Unable to find light with id '" << idAtt->value() << "'.");
      continue;
    }

    ShadowMap::FilterMode shadowFiltering =
        xml::readAttribute<ShadowMap::FilterMode>(n,"shadow-filter",ShadowMap::FILTERING_NONE);
    rapidxml::xml_attribute<> *shadowAtt = n->first_attribute("shadow-map");

    if(shadowAtt!=NULL) {
      const string shadowMapID(shadowAtt->value());
      ref_ptr<ShadowMap> shadowMap = getShadowMap(shadowMapID);
      if(shadowMap.get()==NULL) {
        shadingState->addLight(light);
      }
      else {
        shadingState->addLight(light,shadowMap,shadowFiltering);
      }
    }
    else {
      shadingState->addLight(light);
    }
  }

  // parse passNode
  processNode(shadingNode,passNode);
}

void SceneXML::processMeshNode(
    const ref_ptr<StateNode> &parent,
    rapidxml::xml_node<> *xmlNode)
{
  rapidxml::xml_attribute<> *idAtt = xmlNode->first_attribute("id");
  rapidxml::xml_attribute<> *shaderAtt = xmlNode->first_attribute("shader");
  if (idAtt==NULL) {
    REGEN_WARN("Unable to load mesh, missing id.");
    return;
  }
  vector< ref_ptr<Mesh> > meshes = getMesh(idAtt->value());
  if(meshes.empty()) {
    REGEN_WARN("Unable to load mesh, with id '" << idAtt->value() << "'.");
    return;
  }

  for(vector< ref_ptr<Mesh> >::iterator
      it=meshes.begin(); it!=meshes.end(); ++it)
  {
    ref_ptr<Mesh> meshResource = *it;
    if(meshResource.get()==NULL) {
      REGEN_WARN("null mesh");
      continue;
    }
    ref_ptr<Mesh> mesh;
    // XXX: i guess it's problematic when attributes are defined in the tree,
    //  because they might get added to the same input container.
    //  A mesh shallow copy should contain the original container
    //  for reading only, and another one for custom attributes.
    if(usedMeshes_.count(meshResource.get())==0) {
      // mesh not referenced yet. Take the reference we have to keep
      // reference on special mesh types like Sky.
      mesh = meshResource;
      usedMeshes_.insert(meshResource.get());
    }
    else {
      mesh = ref_ptr<Mesh>::alloc(meshResource);
    }

    ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(mesh);
    parent->addChild(meshNode);

    // Handle shader
    HasShader *hasShader = dynamic_cast<HasShader*>(mesh.get());
    if(hasShader!=NULL) {
      StateConfigurer shaderConfigurer;
      shaderConfigurer.addNode(meshNode.get());
      hasShader->createShader(shaderConfigurer.cfg());
    }
    else if(shaderAtt!=NULL) {
      const string shaderKey = shaderAtt->value();
      ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
      mesh->joinStates(shaderState);

      StateConfigurer shaderConfigurer;
      shaderConfigurer.addNode(meshNode.get());
      shaderState->createShader(shaderConfigurer.cfg(), shaderKey);
      // TODO: what does this do, how should it work with shader sharing....
      mesh->initializeResources(RenderState::get(),
          shaderConfigurer.cfg(), shaderState->shader());
    }
  }
}

////////////////////////////
////////////////////////////
// TODO: rethink GUI stuff

void SceneXML::processTextBoxNode(
    const ref_ptr<StateNode> &parent,
    rapidxml::xml_node<> *xmlNode)
{
  string hAlign = xml::readAttribute<string>(xmlNode, "h-alignment", "left");
  string vAlign = xml::readAttribute<string>(xmlNode, "v-alignment", "top");
  string fontID = xml::readAttribute<string>(xmlNode, "font", "");
  Vec4f textColor = xml::readAttribute<Vec4f>(
      xmlNode, "textColor", Vec4f(0.97f,0.86f,0.77f,0.95f));
  GLfloat height = xml::readAttribute<GLfloat>(
      xmlNode, "height", 16.0f);

  rapidxml::xml_node<> *fontNode = findNode(getRootNode(xmlNode), "font", fontID);
  if(fontNode==NULL) {
    REGEN_WARN("Unable to find font for '" << fontID << "'.");
    return;
  }
  rapidxml::xml_attribute<> *fileAtt = fontNode->first_attribute("file");
  if(fileAtt==NULL) {
    REGEN_WARN("Unable to find font file attribute for '" << fontID << "'.");
    return;
  }
  GLuint size = xml::readAttribute<GLuint>(fontNode, "size", 16u);
  GLuint dpi = xml::readAttribute<GLuint>(fontNode, "dpi", 96u);

  ref_ptr<regen::Font> font = regen::Font::get(
      getResPath(fileAtt->value()),size,dpi);
  if(font.get()==NULL) {
    REGEN_WARN("Unable to load font '" << fontID << "'.");
    return;
  }

  ref_ptr<TextureMappedText> widget = ref_ptr<TextureMappedText>::alloc(font, height);
  widget->set_color(textColor);

  rapidxml::xml_attribute<> *att = xml::findAttribute(xmlNode,"text");
  if(att!=NULL) {
    wstringstream ss;
    ss << att->value();
    widget->set_value(ss.str());
  }

  ref_ptr<StateNode> widgetNode = ref_ptr<StateNode>::alloc(widget);
  parent->addChild(widgetNode);

  StateConfigurer shaderConfigurer;
  if(vAlign == "bottom") {
    shaderConfigurer.define("INVERT_Y", "TRUE");
  }
  if(hAlign == "right") {
    shaderConfigurer.define("INVERT_X", "TRUE");
  }
  shaderConfigurer.addNode(widgetNode.get());
  widget->createShader(shaderConfigurer.cfg());
}

void SceneXML::processTextureBoxNode(
    const ref_ptr<StateNode> &parent,
    rapidxml::xml_node<> *xmlNode)
{
  string hAlign = xml::readAttribute<string>(xmlNode, "h-alignment", "left");
  string vAlign = xml::readAttribute<string>(xmlNode, "v-alignment", "top");
  string textureID = xml::readAttribute<string>(xmlNode, "texture", "");
  Vec2f sizeFactor = xml::readAttribute<Vec2f>(xmlNode, "size", Vec2f(1.0f));

  ref_ptr<Texture> tex = getTexture(textureID);
  if(tex.get()==NULL) {
    REGEN_WARN("Unable to load texture '" << textureID << "'.");
    return;
  }
  Vec2f size(tex->width()*sizeFactor.x, tex->height()*sizeFactor.y);

  Rectangle::Config cfg;
  cfg.levelOfDetail = 0;
  cfg.isTexcoRequired = GL_TRUE;
  cfg.isNormalRequired = GL_FALSE;
  cfg.isTangentRequired = GL_FALSE;
  cfg.centerAtOrigin = GL_FALSE;
  cfg.posScale = Vec3f(size.x, 1.0, size.y);
  cfg.rotation = Vec3f(0.5f*M_PI, 0.0f, 0.0f);
  cfg.texcoScale = Vec2f(-1.0,1.0);
  cfg.translation = Vec3f(0.0f,0.0f,0.0f);
  ref_ptr<Mesh> widget = ref_ptr<Rectangle>::alloc(cfg);

  ref_ptr<Material> material = ref_ptr<Material>::alloc();
  material->alpha()->setVertex(0,0.7f);
  // Join the material at front of node so that
  // States before can redefine material inputs.
  parent->state()->joinStatesFront(material);

  ref_ptr<TextureState> texState = ref_ptr<TextureState>::alloc(tex);
  texState->set_mapTo(TextureState::MAP_TO_COLOR);
  texState->set_blendMode(BLEND_MODE_SRC);
  material->joinStates(texState);

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
  widget->joinStates(shaderState);

  ref_ptr<StateNode> widgetNode = ref_ptr<StateNode>::alloc(widget);
  parent->addChild(widgetNode);

  StateConfigurer shaderConfigurer;
  if(vAlign == "bottom") {
    shaderConfigurer.define("INVERT_Y", "TRUE");
  }
  if(hAlign == "right") {
    shaderConfigurer.define("INVERT_X", "TRUE");
  }
  shaderConfigurer.addNode(widgetNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "regen.gui.widget");
  widget->initializeResources(RenderState::get(),
      shaderConfigurer.cfg(), shaderState->shader());

}

////////////////////////////
////////////////////////////

ref_ptr<FBO> SceneXML::createFBO(rapidxml::xml_node<> *fboNode)
{
  string sizeMode = xml::readAttribute<string>(fboNode, "size-mode", "abs");
  Vec3f sizef = xml::readAttribute<Vec3f>(fboNode, "size", Vec3f(256.0,256.0,1.0));
  Vec3i size = getSize(app_,sizeMode,sizef);

  ref_ptr<FBO> fbo = ref_ptr<FBO>::alloc(size.x,size.y);
  if(sizeMode == "rel") {
    // XXX: never removed
    app_->connect(Application::RESIZE_EVENT, ref_ptr<FBOResizer>::alloc(fbo,sizef.x,sizef.y));
  }

  for(rapidxml::xml_node<> *n=fboNode->first_node();
      n!=NULL; n=n->next_sibling())
  {
    string nodeTag(n->name());

    if(nodeTag == "texture") {
      fbo->addTexture(createTexture(n));
    }
    else if(nodeTag == "depth") {
      GLint depthSize = xml::readAttribute<GLint>(n, "pixel-size", 16);
      GLenum depthType = glenum::pixelType(
          xml::readAttribute<string>(n, "pixel-type", "UNSIGNED_BYTE"));
      GLenum depthFormat;
      if(depthSize<=16) depthFormat=GL_DEPTH_COMPONENT16;
      else if(depthSize<=24) depthFormat=GL_DEPTH_COMPONENT24;
      else depthFormat=GL_DEPTH_COMPONENT32;

      fbo->createDepthTexture(GL_TEXTURE_2D,depthFormat,depthType);
    }
    else {
      REGEN_WARN("Skipping unknown node tag '" << n->name() << "'.");
    }
  }

  return fbo;
}

ref_ptr<Texture> SceneXML::createTexture(rapidxml::xml_node<> *texNode)
{
  rapidxml::xml_attribute<> *fileAtt = texNode->first_attribute("file");
  rapidxml::xml_attribute<> *skyAtt = texNode->first_attribute("sky");
  ref_ptr<Texture> tex;

  if(fileAtt!=NULL) {
    GLenum mipmapFlag = GL_DONT_CARE;
    GLenum forcedType = glenum::pixelType(
        xml::readAttribute<string>(texNode,"forced-type","NONE"));
    GLenum forcedInternalFormat = glenum::textureInternalFormat(
        xml::readAttribute<string>(texNode,"forced-internal-format","NONE"));
    GLenum forcedFormat = glenum::textureFormat(
        xml::readAttribute<string>(texNode,"forced-format","NONE"));
    Vec3ui forcedSize =
        xml::readAttribute<Vec3ui>(texNode,"forced-size",Vec3ui(0u));

    if(xml::readAttribute<bool>(texNode,"is-cube",false)) {
      bool flipBackFace =
          xml::readAttribute<bool>(texNode,"cube-flip-back",false);
      tex = textures::loadCube(
          getResPath(fileAtt->value()),
          flipBackFace,
          mipmapFlag,
          forcedInternalFormat,
          forcedFormat,
          forcedType,
          forcedSize);
    }
    else if(xml::readAttribute<bool>(texNode,"is-array",false)) {
      // TODO: pattern ?
      const string namePattern =
          xml::readAttribute<string>(texNode,"name-pattern","*");
      tex = textures::loadArray(
          getResPath(fileAtt->value()),
          namePattern,
          mipmapFlag,
          forcedInternalFormat,
          forcedFormat,
          forcedType,
          forcedSize);
    }
    else if(xml::readAttribute<bool>(texNode,"is-raw",false)) {
      const Vec3ui size =
          xml::readAttribute<Vec3ui>(texNode,"raw-size",Vec3ui(256u));
      const GLuint numComponents =
          xml::readAttribute<GLuint>(texNode,"raw-components",3u);
      const GLuint bytesPerComponent =
          xml::readAttribute<GLuint>(texNode,"raw-bytes",4u);

      tex = textures::loadRAW(
          getResPath(fileAtt->value()),
          size, numComponents, bytesPerComponent);
    }
    else {
      tex = textures::load(
          getResPath(fileAtt->value()),
          mipmapFlag,
          forcedInternalFormat,
          forcedFormat,
          forcedType,
          forcedSize);
    }
  }
  else if(skyAtt!=NULL) {
    ref_ptr<SkyScattering> sky = getSky(skyAtt->value());
    if(sky.get()) tex = sky->cubeMap();
  }
  else {
    string sizeMode =
        xml::readAttribute<string>(texNode, "size-mode", "abs");
    Vec3f sizef =
        xml::readAttribute<Vec3f>(texNode, "size", Vec3f(256.0,256.0,1.0));
    Vec3i size = getSize(app_,sizeMode,sizef);
    if(sizeMode == "rel") {
      // TODO: auto-resize ??
    }

    GLuint texCount =
        xml::readAttribute<GLuint>(texNode, "count", 1);
    GLuint pixelSize =
        xml::readAttribute<GLuint>(texNode, "pixel-size", 24);
    GLuint pixelComponents =
        xml::readAttribute<GLuint>(texNode, "pixel-components", 4);
    GLenum pixelType = glenum::pixelType(
        xml::readAttribute<string>(texNode, "pixel-type", "UNSIGNED_BYTE"));

    tex = FBO::createTexture(
        size.x,size.y,size.z,
        texCount,
        size.z>1 ? GL_TEXTURE_3D : GL_TEXTURE_2D,
        glenum::textureFormat(pixelComponents),
        glenum::textureInternalFormat(pixelType,pixelComponents,pixelSize),
        pixelType);
  }

  if(tex.get()==NULL) {
    REGEN_WARN("Failed to create Texture.");
    return tex;
  }

  tex->begin(RenderState::get(), 0); {
    if(texNode->first_attribute("wrapping")!=NULL) {
      tex->wrapping().push(glenum::wrappingMode(
          xml::readAttribute<string>(texNode,"wrapping","CLAMP_TO_EDGE")));
    }
    if(texNode->first_attribute("aniso")!=NULL) {
      tex->aniso().push(xml::readAttribute<GLfloat>(texNode,"aniso",2.0f));
    }
    if(texNode->first_attribute("lod")!=NULL) {
      tex->lod().push(xml::readAttribute<Vec2f>(texNode,"lod",Vec2f(1.0f)));
    }
    // TODO: some more texture stuff...

    rapidxml::xml_attribute<> *minFilterAtt = texNode->first_attribute("min-filter");
    rapidxml::xml_attribute<> *magFilterAtt = texNode->first_attribute("mag-filter");
    if(minFilterAtt!=NULL && magFilterAtt!=NULL) {
      GLenum min = glenum::filterMode(minFilterAtt->value());
      GLenum mag = glenum::filterMode(magFilterAtt->value());
      tex->filter().push(TextureFilter(min,mag));
    }
    else if(minFilterAtt!=NULL || magFilterAtt!=NULL) {
      REGEN_WARN("Minifiacation and magnification filters must be specified both.");
    }
  } tex->end(RenderState::get(), 0);

  return tex;
}

ref_ptr<ShadowMap> SceneXML::createShadowMap(rapidxml::xml_node<> *n)
{
  rapidxml::xml_attribute<> *lightAtt = n->first_attribute("light");
  if(lightAtt==NULL) {
    REGEN_WARN("No light attribute defined for shadow map '" << getID(n) << "'.");
    return ref_ptr<ShadowMap>();
  }
  rapidxml::xml_attribute<> *camAtt = n->first_attribute("camera");
  if(camAtt==NULL) {
    REGEN_WARN("No camera attribute defined for shadow map '" << getID(n) << "'.");
    return ref_ptr<ShadowMap>();
  }

  ref_ptr<Light> light = getLight(lightAtt->value());
  if(light.get()==NULL) {
    REGEN_WARN("No light with id '" << lightAtt->value() << "' known.");
    return ref_ptr<ShadowMap>();
  }
  ref_ptr<Camera> cam = getCamera(camAtt->value());
  if(cam.get()==NULL) {
    REGEN_WARN("No camera with id '" << camAtt->value() << "' known.");
    return ref_ptr<ShadowMap>();
  }

  ShadowMap::Config shadowConfig;
  shadowConfig.size =
      xml::readAttribute<GLuint>(n, "size", 1024u);
  shadowConfig.numLayer =
      xml::readAttribute<GLuint>(n, "num-layer", 3);
  shadowConfig.splitWeight =
      xml::readAttribute<GLdouble>(n, "split-weight", 0.9);
  shadowConfig.depthType = glenum::pixelType(
      xml::readAttribute<string>(n, "pixel-type", "FLOAT"));

  GLint depthSize = xml::readAttribute<GLint>(n, "pixel-size", 16);
  if(depthSize<=16)
    shadowConfig.depthFormat = GL_DEPTH_COMPONENT16;
  else if(depthSize<=24)
    shadowConfig.depthFormat = GL_DEPTH_COMPONENT24;
  else
    shadowConfig.depthFormat = GL_DEPTH_COMPONENT32;

  ref_ptr<ShadowMap> sm = ref_ptr<ShadowMap>::alloc(light,cam,shadowConfig);

  // Hacks for shadow issues
  sm->setCullFrontFaces(
      xml::readAttribute<bool>(n,"cull-front-faces",true));
  if(n->first_attribute("polygon-offset")!=NULL) {
    Vec2f x = xml::readAttribute<Vec2f>(n,"polygon-offset",Vec2f(0.0f,0.0f));
    sm->setPolygonOffset(x.x,x.y);
  }
  // Hide cube shadow map faces.
  if(n->first_attribute("hide-faces")!=NULL) {
    const string val = xml::readAttribute<string>(n,"hide-faces","");
    vector<string> faces;
    boost::split(faces,val,boost::is_any_of(","));
    for(vector<string>::iterator it=faces.begin();
        it!=faces.end(); ++it) {
      int faceIndex = atoi(it->c_str());
      sm->set_isCubeFaceVisible(
          GL_TEXTURE_CUBE_MAP_POSITIVE_X+faceIndex, GL_FALSE);
    }
  }

  // Setup moment computation. Moments make it possible to
  // apply blur filter to shadow map.
  if(xml::readAttribute<bool>(n,"compute-moments",false)) {
    sm->setComputeMoments();

    if(xml::readAttribute<bool>(n,"blur",false)) {
      GLuint size =
          xml::readAttribute<GLuint>(n,"blur-size",GLuint(4));
      GLfloat sigma =
          xml::readAttribute<GLfloat>(n,"blur-sigma",GLfloat(2.0));
      GLboolean downsampleTwice =
          xml::readAttribute<GLboolean>(n,"blur-downsample-twice",GL_FALSE);
      sm->createBlurFilter(size, sigma, downsampleTwice);
    }
  }

  return sm;
}

ref_ptr<Camera> SceneXML::createCamera(rapidxml::xml_node<> *n)
{
  ref_ptr<Camera> cam = ref_ptr<Camera>::alloc();

  cam->set_sensitivity(
      xml::readAttribute<GLfloat>(n, "sensitivity", 0.000125f));
  cam->set_walkSpeed(
      xml::readAttribute<GLfloat>(n, "walkSpeed", 0.5f));
  cam->set_isAudioListener(
      xml::readAttribute<bool>(n, "isAudioListener", false));

  cam->position()->setVertex(0,
      xml::readAttribute<Vec3f>(n, "position", Vec3f(0.0f,2.0f,-2.0f)));

  Vec3f dir = xml::readAttribute<Vec3f>(n, "direction", Vec3f(0.0f,0.0f,1.0f));
  dir.normalize();
  cam->direction()->setVertex(0, dir);

  GLfloat fov =
      xml::readAttribute<GLfloat>(n, "fov", 45.0f);
  GLfloat near =
      xml::readAttribute<GLfloat>(n, "near", 0.1f);
  GLfloat far =
      xml::readAttribute<GLfloat>(n, "far", 200.0f);
  ref_ptr<ProjectionUpdater> projUpdater =
      ref_ptr<ProjectionUpdater>::alloc(cam, fov, near, far);

  // XXX: never removed
  app_->connect(Application::RESIZE_EVENT, projUpdater); {
    EventData evData;
    evData.eventID = Application::RESIZE_EVENT;
    projUpdater->call(app_, &evData);
  }

  return cam;
}

ref_ptr<Light> SceneXML::createLight(rapidxml::xml_node<> *n)
{
  rapidxml::xml_attribute<> *skyAtt = n->first_attribute("sky");
  if(skyAtt!=NULL) {
    ref_ptr<SkyScattering> sky = getSky(skyAtt->value());
    if(sky.get()) {
      return sky->sun();
    }
    else return ref_ptr<Light>();
  }
  else {
    Light::Type lightType =
        xml::readAttribute<Light::Type>(n, "type", Light::SPOT);
    ref_ptr<Light> light = ref_ptr<Light>::alloc(lightType);
    light->set_isAttenuated(
        xml::readAttribute<bool>(n, "isAttenuated", lightType!=Light::DIRECTIONAL));
    light->position()->setVertex(0,
        xml::readAttribute<Vec3f>(n, "position", Vec3f(0.0f)));
    light->direction()->setVertex(0,
        xml::readAttribute<Vec3f>(n, "direction", Vec3f(0.0f,0.0f,1.0f)));
    light->diffuse()->setVertex(0,
        xml::readAttribute<Vec3f>(n, "diffuse", Vec3f(1.0f)));
    light->specular()->setVertex(0,
        xml::readAttribute<Vec3f>(n, "specular", Vec3f(1.0f)));
    light->radius()->setVertex(0,
        xml::readAttribute<Vec2f>(n, "radius", Vec2f(999999.9f)));

    Vec2f angles = xml::readAttribute<Vec2f>(n, "cone-angles", Vec2f(50.0f,55.0f));
    light->set_innerConeAngle(angles.x);
    light->set_outerConeAngle(angles.y);

    return light;
  }
}

///////////////////////////
///////////////////////////
///////////////////////////

ref_ptr<SkyScattering> SceneXML::createSky(rapidxml::xml_node<> *n)
{
  GLuint size = xml::readAttribute<GLuint>(n, "size", 512);
  bool useFloat = xml::readAttribute<GLuint>(n, "use-float", false);
  ref_ptr<SkyScattering> sky = ref_ptr<SkyScattering>::alloc(size,useFloat);

  const string preset = xml::readAttribute<string>(n, "preset", "earth");
  if(preset == "earth")       sky->setEarth();
  else if(preset == "mars")   sky->setMars();
  else if(preset == "venus")  sky->setVenus();
  else if(preset == "uranus") sky->setUranus();
  else if(preset == "alien")  sky->setAlien();
  else if(preset == "custom") {
    const Vec3f absorbtion =
        xml::readAttribute<Vec3f>(n, "absorbtion", Vec3f(
            0.18867780436772762,
            0.4978442963618773,
            0.6616065586417131));
    const Vec3f rayleigh =
        xml::readAttribute<Vec3f>(n, "rayleigh", Vec3f(19.0,359.0,81.0));
    const Vec4f mie =
        xml::readAttribute<Vec4f>(n, "mie", Vec4f(44.0,308.0,39.0,74.0));
    const GLfloat spot =
        xml::readAttribute<GLfloat>(n, "spot", 373.0);
    const GLfloat strength =
        xml::readAttribute<GLfloat>(n, "strength", 54.0);

    sky->setRayleighBrightness(rayleigh.x);
    sky->setRayleighStrength(rayleigh.y);
    sky->setRayleighCollect(rayleigh.z);
    sky->setMieBrightness(mie.x);
    sky->setMieStrength(mie.y);
    sky->setMieCollect(mie.z);
    sky->setMieDistribution(mie.w);
    sky->setSpotBrightness(spot);
    sky->setScatterStrength(strength);
    sky->setAbsorbtion(absorbtion);
  }
  else REGEN_WARN("Ignoring unknown sky preset '" << preset << "'.");

  sky->setSunElevation(
      xml::readAttribute<GLdouble>(n, "dayLength", 0.8),
      xml::readAttribute<GLdouble>(n, "maxElevation", 30.0),
      xml::readAttribute<GLdouble>(n, "minElevation", -20.0));
  sky->set_dayTime(
      xml::readAttribute<GLdouble>(n, "dayTime", 0.5));
  sky->set_timeScale(
      xml::readAttribute<GLdouble>(n, "timeScale", 0.00000004));
  sky->set_updateInterval(
      xml::readAttribute<GLdouble>(n, "updateInterval", 4000.0));

  return sky;
}

vector< ref_ptr<Mesh> > SceneXML::createAssetMeshes(
    const ref_ptr<AssimpImporter> &importer, rapidxml::xml_node<> *n)
{
  const VBO::Usage vboUsage =
      xml::readAttribute<VBO::Usage>(n,"usage",VBO::USAGE_DYNAMIC);
  const Vec3f scaling =
      xml::readAttribute<Vec3f>(n,"scaling",Vec3f(1.0f));
  const Vec3f rotation =
      xml::readAttribute<Vec3f>(n,"rotation",Vec3f(0.0f));
  const Vec3f translation =
      xml::readAttribute<Vec3f>(n,"translation",Vec3f(0.0f));

  Mat4f transform = Mat4f::scaleMatrix(scaling);
  Quaternion q(0.0,0.0,0.0,1.0);
  q.setEuler(rotation.x,rotation.y,rotation.z);
  transform *= q.calculateMatrix();
  transform.translate(translation);

  const string assetIndices = xml::readAttribute<string>(n,"asset-indices","*");
  bool useAnimation = xml::readAttribute<bool>(n,"asset-animation",false);
  vector< ref_ptr<Mesh> > out;

  vector<string> indicesStr;
  boost::split(indicesStr,assetIndices,boost::is_any_of(","));
  vector<GLuint> indices(indicesStr.size());
  bool useAllIndices = false;
  for(GLuint i=0u; i<indices.size(); ++i) {
    if(indicesStr[i] == "*") {
      useAllIndices = true;
      break;
    }
    else {
      indices[i] = atoi(indicesStr[i].c_str());
    }
  }

  const vector< ref_ptr<NodeAnimation> > &nodeAnims = importer->getNodeAnimations();
  if(useAnimation && nodeAnims.empty()) {
    REGEN_WARN("Mesh has use-animation=1 but Asset '" <<
        xml::readAttribute<string>(n,"asset","") << "' has not.");
    useAnimation = false;
  }

  if(useAllIndices) {
    out = importer->loadAllMeshes(transform,vboUsage);
  }
  else {
    out = importer->loadMeshes(transform,vboUsage,indices);
  }
  for(GLuint i=0u; i<out.size(); ++i) {
    ref_ptr<Mesh> mesh = out[i];
    if(mesh.get()==NULL) continue;

    if(xml::readAttribute<bool>(n,"asset-material",true)) {
      ref_ptr<Material> material =
          importer->getMeshMaterial(mesh.get());
      if(material.get()!=NULL) {
        mesh->joinStates(material);
      }
    }

    if(useAnimation) {
      list< ref_ptr<AnimationNode> > meshBones;
      GLuint numBoneWeights = importer->numBoneWeights(mesh.get());
      GLuint numBones = 0u;

      for(vector< ref_ptr<NodeAnimation> >::const_iterator
          it=nodeAnims.begin(); it!=nodeAnims.end(); ++it) {
        list< ref_ptr<AnimationNode> > ibonNodes =
            importer->loadMeshBones(mesh.get(), it->get());
        meshBones.insert(meshBones.end(), ibonNodes.begin(), ibonNodes.end());
        numBones = ibonNodes.size();
      }

      if(!meshBones.empty()) {
        ref_ptr<Bones> bonesState = ref_ptr<Bones>::alloc(numBoneWeights,numBones);
        bonesState->setBones(meshBones);
        mesh->joinStates(bonesState);
      }
    }
  }

  return out;
}

ref_ptr<Particles> SceneXML::createParticleMesh(rapidxml::xml_node<> *n,
    const GLuint numParticles, const string &updateShader)
{
  ref_ptr<Particles> particles = ref_ptr<Particles>::alloc(numParticles,updateShader);

  particles->gravity()->setVertex(0,
      xml::readAttribute<Vec3f>(n,"gravity",Vec3f(0.0,-9.81,0.0)));
  particles->dampingFactor()->setVertex(0,
      xml::readAttribute<GLfloat>(n,"damping-factor",2.0));
  particles->noiseFactor()->setVertex(0,
      xml::readAttribute<GLfloat>(n,"noise-factor",100.0));

  for(rapidxml::xml_node<> *m=n->first_node(); m!=NULL; m=m->next_sibling())
  { processStateNode(particles,m); }

  // create VBO and Update shader.
  particles->createBuffer();

  return particles;
}


vector< ref_ptr<Mesh> > SceneXML::createMesh(rapidxml::xml_node<> *n)
{
  const string meshType =
      xml::readAttribute<string>(n,"type","");
  // TODO: struct for below stuff, meshes should use it.
  GLuint levelOfDetail =
      xml::readAttribute<GLuint>(n,"lod",4);
  Vec3f scaling =
      xml::readAttribute<Vec3f>(n,"scaling",Vec3f(1.0f));
  Vec2f texcoScaling =
      xml::readAttribute<Vec2f>(n,"texco-scaling",Vec2f(1.0f));
  Vec3f rotation =
      xml::readAttribute<Vec3f>(n,"rotation",Vec3f(0.0f));
  bool useNormal =
      xml::readAttribute<bool>(n,"use-normal",true);
  bool useTexco =
      xml::readAttribute<bool>(n,"use-texco",true);
  bool useTangent =
      xml::readAttribute<bool>(n,"use-tangent",false);
  VBO::Usage vboUsage =
      xml::readAttribute<VBO::Usage>(n,"usage",VBO::USAGE_DYNAMIC);
  vector< ref_ptr<Mesh> > out;

  // Primitives
  if(meshType == "sphere") {
    Sphere::Config meshCfg;
    meshCfg.texcoMode = xml::readAttribute<Sphere::TexcoMode>(
        n, "texco-mode", Sphere::TEXCO_MODE_UV);
    meshCfg.levelOfDetail = levelOfDetail;
    meshCfg.posScale = scaling;
    meshCfg.texcoScale = texcoScaling;
    meshCfg.isNormalRequired = useNormal;
    meshCfg.isTangentRequired = useTangent;
    meshCfg.usage = vboUsage;

    out = vector< ref_ptr<Mesh> >(1);
    out[0] = ref_ptr<Sphere>::alloc(meshCfg);
  }
  else if(meshType == "rectangle") {
    Rectangle::Config meshCfg;
    meshCfg.centerAtOrigin =
        xml::readAttribute<bool>(n,"center",true);
    meshCfg.levelOfDetail = levelOfDetail;
    meshCfg.posScale = scaling;
    meshCfg.rotation = rotation;
    meshCfg.texcoScale = texcoScaling;
    meshCfg.isNormalRequired = useNormal;
    meshCfg.isTangentRequired = useTangent;
    meshCfg.isTexcoRequired = useTexco;
    meshCfg.usage = vboUsage;

    out = vector< ref_ptr<Mesh> >(1);
    out[0] = ref_ptr<Rectangle>::alloc(meshCfg);
  }
  else if(meshType == "box") {
    Box::Config meshCfg;
    meshCfg.texcoMode = xml::readAttribute<Box::TexcoMode>(
        n, "texco-mode", Box::TEXCO_MODE_UV);
    meshCfg.posScale = scaling;
    meshCfg.rotation = rotation;
    meshCfg.texcoScale = texcoScaling;
    meshCfg.isNormalRequired = useNormal;
    meshCfg.isTangentRequired = useTangent;
    meshCfg.usage = vboUsage;

    out = vector< ref_ptr<Mesh> >(1);
    out[0] = ref_ptr<Box>::alloc(meshCfg);
  }
  else if(meshType == "cone" || meshType == "cone-closed") {
    ConeClosed::Config meshCfg;
    meshCfg.levelOfDetail = levelOfDetail;
    meshCfg.radius =
        xml::readAttribute<GLfloat>(n,"radius",1.0f);
    meshCfg.height =
        xml::readAttribute<GLfloat>(n,"height",1.0f);
    meshCfg.isBaseRequired =
        xml::readAttribute<bool>(n,"use-base",true);
    meshCfg.isNormalRequired = useNormal;
    meshCfg.usage = vboUsage;

    out = vector< ref_ptr<Mesh> >(1);
    out[0] = ref_ptr<ConeClosed>::alloc(meshCfg);
  }
  else if(meshType == "cone-opened") {
    ConeOpened::Config meshCfg;
    meshCfg.levelOfDetail = levelOfDetail;
    meshCfg.cosAngle =
        xml::readAttribute<GLfloat>(n,"angle",0.5f);
    meshCfg.height =
        xml::readAttribute<GLfloat>(n,"height",1.0f);
    meshCfg.isNormalRequired = useNormal;
    meshCfg.usage = vboUsage;

    out = vector< ref_ptr<Mesh> >(1);
    out[0] = ref_ptr<ConeOpened>::alloc(meshCfg);
  }
  // Special meshes
  else if(meshType == "particles") {
    const GLuint numParticles =
        xml::readAttribute<GLuint>(n,"num-vertices",0u);
    const string updateShader =
        xml::readAttribute<string>(n,"update-shader","");
    if(numParticles==0u) {
      REGEN_WARN("Ignoring particles with num-vertices=0.");
    }
    else if(updateShader.empty()) {
      REGEN_WARN("Ignoring particles without update-shader.");
    }
    else {
      out = vector< ref_ptr<Mesh> >(1);
      out[0] = createParticleMesh(n,numParticles,updateShader);
      return out;
    }
  }
  else if(meshType == "asset") {
    ref_ptr<AssimpImporter> importer = getAsset(xml::readAttribute<string>(n,"asset",""));
    if(importer.get()==NULL) {
      REGEN_WARN("Ignoring mesh with unknown asset '" <<
          xml::readAttribute<string>(n,"asset","") << "'.");
    }
    else {
      out = createAssetMeshes(importer,n);
    }
  }
  else if(meshType == "sky") {
    out = vector< ref_ptr<Mesh> >(1);
    out[0] = getSky(getID(n));
  }
  else if(meshType == "text") {
    // TODO: text
    REGEN_WARN("Ignoring mesh with unhandled type '" << meshType << "'.");
  }
  else {
    REGEN_WARN("Ignoring mesh with unknown type '" << meshType << "'.");
  }

  for(rapidxml::xml_node<> *m=n->first_node(); m!=NULL; m=m->next_sibling())
  {
    for(vector< ref_ptr<Mesh> >::iterator
        it=out.begin(); it!=out.end(); ++it) {
      processStateNode(*it,m);
    }
  }

  return out;
}

ref_ptr<AssimpImporter> SceneXML::createAsset(rapidxml::xml_node<> *n)
{
  rapidxml::xml_attribute<> *fileAtt = n->first_attribute("file");
  if(fileAtt==NULL) {
    REGEN_WARN("Ignoring Asset '" << getID(n) << "' without file.");
    return ref_ptr<AssimpImporter>();
  }
  ref_ptr<AssimpImporter> importer;
  const string texturePath = getResPath(
      xml::readAttribute<string>(n,"texture-path",""));
  GLint assimpFlags = xml::readAttribute<GLint>(n,"import-flags",-1);

  AssimpAnimationConfig animConfig;
  animConfig.numInstances =
      xml::readAttribute<GLuint>(n,"animation-instances",1u);
  animConfig.useAnimation = (animConfig.numInstances>0) &&
      xml::readAttribute<bool>(n,"use-animation",true);
  animConfig.forceStates =
      xml::readAttribute<bool>(n,"animation-force-states",true);
  animConfig.ticksPerSecond =
      xml::readAttribute<GLfloat>(n,"animation-tps",20.0);
  animConfig.postState = xml::readAttribute<NodeAnimation::Behavior>(
      n,"animation-post-state",NodeAnimation::BEHAVIOR_LINEAR);
  animConfig.preState = xml::readAttribute<NodeAnimation::Behavior>(
      n,"animation-pre-state",NodeAnimation::BEHAVIOR_LINEAR);

  try {
    importer = ref_ptr<AssimpImporter>::alloc(
        getResPath(fileAtt->value()),
        texturePath,
        animConfig,
        assimpFlags);
  }
  catch(AssimpImporter::Error &e) {
    REGEN_WARN("Unable to open asset file: " << e.what() << ".");
    return ref_ptr<AssimpImporter>();
  }

  return importer;
}

////////////////////////////
////////////////////////////

ref_ptr<FBO> SceneXML::getFBO(const string &id)
{
  if(fbos_.count(id)>0) return fbos_[id];

  rapidxml::xml_node<> *n = findNode(&doc_, "fbo", id);
  if(n==NULL) {
    REGEN_WARN("Unable to find FBO with id '" << id << "'.");
    return ref_ptr<FBO>();
  }
  else {
    ref_ptr<FBO> fbo = createFBO(n);
    fbos_[id] = fbo;
    return fbo;
  }
}

ref_ptr<Texture> SceneXML::getTexture(const string &id)
{
  if(textures_.count(id)>0) return textures_[id];

  rapidxml::xml_node<> *n = findNode(&doc_, "texture", id);
  if(n==NULL) {
    REGEN_WARN("Unable to find Texture with id '" << id << "'.");
    return ref_ptr<Texture>();
  }
  else {
    ref_ptr<Texture> tex = createTexture(n);
    textures_[id] = tex;
    return tex;
  }
}

ref_ptr<SkyScattering> SceneXML::getSky(const string &id)
{
  if(skys_.count(id)>0) return skys_[id];

  rapidxml::xml_node<> *n = findNode(&doc_, "mesh", id);
  if(n==NULL) {
    REGEN_WARN("Unable to find Sky with id '" << id << "'.");
    return ref_ptr<SkyScattering>();
  }
  else {
    ref_ptr<SkyScattering> sky = createSky(n);
    skys_[id] = sky;
    return sky;
  }
}

ref_ptr<Camera> SceneXML::getCamera(const string &id)
{
  if(cameras_.count(id)>0) return cameras_[id];

  rapidxml::xml_node<> *n = findNode(&doc_, "camera", id);
  if(n==NULL) {
    REGEN_WARN("Unable to find Camera with id '" << id << "'.");
    return ref_ptr<Camera>();
  }
  else {
    ref_ptr<Camera> cam = createCamera(n);
    cameras_[id] = cam;
    return cam;
  }
}

ref_ptr<Light> SceneXML::getLight(const string &id)
{
  if(lights_.count(id)>0) return lights_[id];

  rapidxml::xml_node<> *n = findNode(&doc_, "light", id);
  if(n==NULL) {
    REGEN_WARN("Unable to find Light with id '" << id << "'.");
    return ref_ptr<Light>();
  }
  else {
    ref_ptr<Light> light = createLight(n);
    lights_[id] = light;
    return light;
  }
}

ref_ptr<AssimpImporter> SceneXML::getAsset(const string &id)
{
  if(assets_.count(id)>0) return assets_[id];

  rapidxml::xml_node<> *n = findNode(&doc_, "asset", id);
  if(n==NULL) {
    REGEN_WARN("Unable to find Asset with id '" << id << "'.");
    return ref_ptr<AssimpImporter>();
  }
  else {
    ref_ptr<AssimpImporter> v = createAsset(n);
    assets_[id] = v;
    return v;
  }
}

vector< ref_ptr<Mesh> > SceneXML::getMesh(const string &id)
{
  if(meshes_.count(id)>0) return meshes_[id];

  rapidxml::xml_node<> *n = findNode(&doc_, "mesh", id);
  if(n==NULL) {
    REGEN_WARN("Unable to find Mesh with id '" << id << "'.");
    return vector< ref_ptr<Mesh> >();
  }
  vector< ref_ptr<Mesh> > v = createMesh(n);
  meshes_[id] = v;
  return v;
}

vector<BoneAnimRange> SceneXML::getAnimationRanges(const string &assetId)
{
  rapidxml::xml_node<> *n = findNode(&doc_, "asset", assetId);
  if(n==NULL) {
    REGEN_WARN("Unable to find Asset with id '" << assetId << "'.");
    return vector<BoneAnimRange>();
  }

  GLuint animRangeCount = 0u;
  for(rapidxml::xml_node<> *rangeNode=n->first_node("anim-range");
      rangeNode!=NULL; rangeNode=rangeNode->next_sibling("anim-range"))
  { animRangeCount += 1u; }

  vector<BoneAnimRange> out(animRangeCount);
  animRangeCount = 0u;
  for(rapidxml::xml_node<> *rangeNode=n->first_node("anim-range");
      rangeNode!=NULL; rangeNode=rangeNode->next_sibling("anim-range"))
  {
    out[animRangeCount] = BoneAnimRange(
        xml::readAttribute<string>(rangeNode,"name",""),
        xml::readAttribute<Vec2d>(rangeNode,"range",Vec2d(0.0)));
    animRangeCount += 1u;
  }

  return out;
}

ref_ptr<ShadowMap> SceneXML::getShadowMap(const string &id)
{
  if(shadowMaps_.count(id)>0) return shadowMaps_[id];

  rapidxml::xml_node<> *n = findNode(&doc_, "shadow-map", id);
  if(n==NULL) {
    REGEN_WARN("Unable to find ShadowMap with id '" << id << "'.");
    return ref_ptr<ShadowMap>();
  }
  ref_ptr<ShadowMap> shadowMap = createShadowMap(n);
  shadowMaps_[id] = shadowMap;

  // add caster to shadow map
  for(list< ref_ptr<ShadowCaster> >::iterator it=shadowCaster_.begin();
      it!=shadowCaster_.end(); ++it)
  {
    const ref_ptr<ShadowCaster> &caster = *it;
    if(isShadowCaster(id,caster->targets)) {
      // XXX never removed
      shadowMap->addCaster(caster->node);
    }
  }

  return shadowMap;
}

ref_ptr<BulletPhysics> SceneXML::getPhysics()
{
  return physics_;
}
ref_ptr<StateNode> SceneXML::getNode(const string &id)
{
  return nodes_[id];
}

void SceneXML::addShadowCaster(
    const ref_ptr<StateNode> &node,
    const string &shadowMapIds)
{
  ref_ptr<ShadowCaster> caster = ref_ptr<ShadowCaster>::alloc();
  caster->node = node;
  boost::split(caster->targets, shadowMapIds, boost::is_any_of(","));
  shadowCaster_.push_back(caster);

  // add the caster to loaded shadow maps
  for(map<string, ref_ptr<ShadowMap> >::iterator it=shadowMaps_.begin();
      it!=shadowMaps_.end(); ++it)
  {
    const string &shadowMapID = it->first;
    if(isShadowCaster(shadowMapID,caster->targets)) {
      const ref_ptr<ShadowMap> &shadowMap = it->second;
      // XXX never removed
      shadowMap->addCaster(node);
    }
  }

}

/*
 * render-tree.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <queue>

#include "render-tree.h"
#include <ogle/shader/shader-manager.h>
#include <ogle/utility/stack.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/utility/gl-error.h>

// TODO RENDER_TREE: implement tree optimizing.
//    * remove states that are enabled before
//    * join childs with identical states (for example same material)
//    * optimize VBO usage
//    * try to avoid enable/disable costly states

static inline bool isShaderInputState(State *s)
{
  return dynamic_cast<ShaderInputState*>(s)!=NULL;
}

static inline bool isVBOState(State *s)
{
  return dynamic_cast<VBOState*>(s)!=NULL;
}
static inline VBOState* getVBOState(State *s)
{
  if(isVBOState(s)) {
    return (VBOState*)s;
  }
  // first try joined states, they are enabled before s
  for(list< ref_ptr<State> >::reverse_iterator
      it=s->joined().rbegin(); it!=s->joined().rend(); ++it)
  {
    return getVBOState(it->get());
  }
  return NULL;
}

static GLboolean hasUnhandledGeometry(State *s)
{
  if(!isShaderInputState(s)) { return false; }
  ShaderInputState *attState = (ShaderInputState*)(s);
  return !attState->isBufferSet();
}

static void copyPath(
    ref_ptr<StateNode> pathStartNode,
    ref_ptr<StateNode> pathEndNode,
    ref_ptr<StateNode> fromNode,
    ref_ptr<StateNode> endNode)
{
  ref_ptr<StateNode> path = pathEndNode;
  for(ref_ptr<StateNode> n=fromNode;
      n->parent().get()!=endNode.get(); n=n->parent())
  {
    ref_ptr<StateNode> nodeCopy =
        ref_ptr<StateNode>::manage(new StateNode(n->state()));
    nodeCopy->addChild(path);
    path->set_parent(nodeCopy);
    path = nodeCopy;
  }
  pathStartNode->addChild(path);
  path->set_parent(pathStartNode);
}

RenderTree::RenderTree(ref_ptr<StateNode> &node)
{
  rootNode_ = ref_ptr<StateNode>::manage(new StateNode);
  // find the root node of node
  ref_ptr<StateNode> n;
  for(n=node; n->hasParent(); n=n->parent());
  addChild(rootNode_, n, true);
}

RenderTree::RenderTree()
{
  rootNode_ = ref_ptr<StateNode>::manage(new StateNode);
}

ref_ptr<StateNode>& RenderTree::rootNode()
{
  return rootNode_;
}

static GLboolean updateState(
    State *state,
    GLfloat dt,
    set< State* > &updatedStates)
{
  if(updatedStates.count(state)==0) {
    updatedStates.insert(state);
    state->update(dt);
  }
  GLboolean geomUpdated = hasUnhandledGeometry(state);

  for(list< ref_ptr<State> >::iterator
      it=state->joined().begin(); it!=state->joined().end(); ++it)
  {
    if(updateState(it->get(), dt, updatedStates)) {
      geomUpdated = GL_TRUE;
    }
  }
  return geomUpdated;
}
void RenderTree::updateStates(GLfloat dt)
{
  Stack< ref_ptr<StateNode> > nodes;
  set< ref_ptr<StateNode> > updatedGeometryNodes;
  set< State* > updatedStates;

  // wake up animation thread if it is waiting for the next frame
  // from rendering thread
#ifdef SYNCHRONIZE_ANIM_AND_RENDER
  AnimationManager::get().nextFrame();
#endif

  nodes.push(rootNode_);
  while(!nodes.isEmpty()) {
    ref_ptr<StateNode> n = nodes.top();
    nodes.pop();
    State *s = n->state().get();
    if(updateState(s, dt, updatedStates)) {
      updatedGeometryNodes.insert(n);
    }
    for(list< ref_ptr<StateNode> >::iterator
        it=n->childs().begin(); it!=n->childs().end(); ++it)
    {
      nodes.push(*it);
    }
  }

  // re-add nodes which have update geometry
  for(set< ref_ptr<StateNode> >::iterator it=updatedGeometryNodes.begin();
      it!=updatedGeometryNodes.end(); ++it)
  {
    addChild((*it)->parent(), *it, true);
  }

#ifdef SYNCHRONIZE_ANIM_AND_RENDER
  // wait for animation thread if it was slower then the rendering thread,
  // animations must do a step each frame
  AnimationManager::get().waitForStep();
#endif
  // some animations modify the vertex data,
  // updating the vbo needs a context so we do it here in the main thread..
  AnimationManager::get().updateGraphics(dt);
  // invoke event handler of queued events
  EventObject::emitQueued();
}

ref_ptr<StateNode> RenderTree::addVBONode(
    ref_ptr<StateNode> node, GLuint sizeMB)
{
  list< ShaderInputState* > attributeStates;
  findUnhandledGeomNodes(node, &attributeStates);

  ref_ptr<State> vboState = ref_ptr<State>::manage(
      new VBOState(attributeStates, sizeMB));
  ref_ptr<StateNode> vboNode = ref_ptr<StateNode>::manage(
      new StateNode(vboState));

  list< ref_ptr<StateNode> > children = node->childs();
  for(list< ref_ptr<StateNode> >::iterator
      it=children.begin(); it!=children.end(); ++it)
  {
    ref_ptr<StateNode> child = *it;
    vboNode->addChild(child);
    node->removeChild(child);
    child->set_parent(vboNode);
  }
  node->addChild(vboNode);
  vboNode->set_parent(node);

  return vboNode;
}

void RenderTree::addChild(
    ref_ptr<StateNode> parent,
    ref_ptr<StateNode> node,
    GLboolean generateVBONode)
{
  if(node->hasParent())
  {
    remove(node);
  }

  // if parent has VBO childs, use these as parent
  for(list< ref_ptr<StateNode> >::iterator
      it=parent->childs().begin();
      it!=parent->childs().end(); ++it)
  {
    VBOState *vboState = getVBOState(it->get()->state().get());
    if(vboState!=NULL) {
      parent = *it;
      break;
    }
  }

  // get geometry defined in the child tree
  // of the node that has no VBO parent yet
  list< ShaderInputState* > attributeStates;
  findUnhandledGeomNodes(node, &attributeStates);

  if(attributeStates.empty()) {
    // no unhandled geometry in child tree
    node->set_parent(parent);
    parent->addChild(node);
  } else {
    ref_ptr<StateNode> vboNode = getParentVBO(parent);

    if(vboNode.get() == NULL) {
      DEBUG_LOG("Mesh added without parent VBO. Creating a VBO node at the root node.");
      vboNode = addVBONode(rootNode_);
    }

    VBOState *vboState = getVBOState(vboNode->state().get());
    if(vboState->add(attributeStates))
    {
      node->set_parent(parent);
      parent->addChild(node);
      return;
    }
    DEBUG_LOG("VBO has not enough space left. Creating new one.");

    // not enough space for the data...
    // we have to create a new VBO. First check if
    // there is a vbo with enough space on the same level.
    for(list< ref_ptr<StateNode> >::iterator
        it=vboNode->parent()->childs().begin();
        it!=vboNode->parent()->childs().end(); ++it)
    {
      ref_ptr<StateNode> otherVboNode = *it;
      if(otherVboNode.get()==vboNode.get()) { continue; }

      VBOState *otherVboState = getVBOState(otherVboNode->state().get());
      if(otherVboState==NULL) { continue; }

      if(!otherVboState->add(attributeStates)) { continue; }

      // now we have found a VBO child but the node was attached in the
      // tree on a path with VBO parent. We have to copy the complete
      // path to the VBO to the new VBO node.
      // TODO: test
      copyPath(otherVboNode, node, parent, vboNode);
      return;
    }

    // there is no VBO with enough space on the same level
    // as the parent VBO. So we create a new one.
    // TODO: test
    ref_ptr<State> newVboState = ref_ptr<State>::manage(
        new VBOState(attributeStates));
    ref_ptr<StateNode> newVboNode = ref_ptr<StateNode>::manage(
        new StateNode(newVboState));
    vboNode->parent()->addChild(newVboNode);
    newVboNode->set_parent(vboNode->parent());
    copyPath(newVboNode, node, parent, vboNode);
  }
}

void RenderTree::remove(ref_ptr<StateNode> node)
{
  if(node->hasParent()) {
    // remove geometry data from parent VBO
    list< ShaderInputState* > geomNodes;
    findUnhandledGeomNodes(node, &geomNodes);
    removeFromVBO(node->parent(), geomNodes);
    // remove parent-node connection
    node->parent()->removeChild(node);
    node->set_parent(ref_ptr<StateNode>());
    for(list< ShaderInputState* >::iterator
        it=geomNodes.begin(); it!=geomNodes.end(); ++it)
    {
      (*it)->setBuffer(0);
    }
  }
}

void RenderTree::traverse(RenderState *state)
{
  handleGLError("before RenderTree::traverse");

  rootNode_->traverse(state);

  handleGLError("after RenderTree::traverse");
}

void RenderTree::traverse(RenderState *state, ref_ptr<StateNode> node)
{
  handleGLError("before RenderTree::traverse");

  node->traverse(state);

  handleGLError("after RenderTree::traverse");
}

ref_ptr<Shader> RenderTree::generateShader(
    StateNode &node,
    ShaderGenerator *shaderGen,
    ShaderConfiguration *cfg)
{
  ref_ptr<Shader> shader = ref_ptr<Shader>::manage(new Shader);
  static const GLenum knownStages[] =
  {
      GL_VERTEX_SHADER,
      GL_TESS_CONTROL_SHADER,
      GL_TESS_EVALUATION_SHADER,
      GL_GEOMETRY_SHADER,
      GL_FRAGMENT_SHADER
  };
  static GLuint numKnownStages = sizeof(knownStages)/sizeof(GLenum);

  // let parent nodes and state configure the shader
  node.configureShader(cfg);

  shaderGen->generate(cfg);

  map<GLenum, ShaderFunctions> stages = shaderGen->getShaderStages();
  // generate glsl source for stages
  map<GLenum, string> stagesStr;
  for(map<GLenum, ShaderFunctions>::iterator
      it=stages.begin(); it!=stages.end(); ++it)
  {
    // find next shader stage, need for
    // input output variable naming
    GLboolean stageFound = GL_FALSE;
    GLenum nextStage = GL_NONE;
    for(GLuint i=0; i<numKnownStages; ++i) {
      if(knownStages[i]==it->first) {
        stageFound = GL_TRUE;
      } else if(stageFound) {
        if(stages.count(knownStages[i])>0) {
          // found next stage
          nextStage = knownStages[i];
          break;
        }
      }
    }
    stagesStr[it->first] = ShaderManager::generateSource(
        it->second, it->first, nextStage);
  }

  // compile and link the shader
  shader->compile(stagesStr); {
    // call glTransformFeedbackVaryings
    list<string> tfNames = ShaderManager::getValidTransformFeedbackNames(
        stagesStr, cfg->transformFeedbackAttributes());
    shader->setupTransformFeedback(tfNames);
    // bind fragment outputs collected at parent nodes
    list<ShaderOutput> outputs;
    list<ShaderFragmentOutput*> fragmentOutputs = cfg->fragmentOutputs();
    for(list<ShaderFragmentOutput*>::const_iterator
        it=fragmentOutputs.begin(); it!=fragmentOutputs.end(); ++it)
    {
      ShaderFragmentOutput *x = *it;
      outputs.push_back(ShaderOutput(
          x->colorAttachment(), x->variableName()));
    }
    shader->setupOutputs(outputs);
  } shader->link();

  // load uniform and attribute locations
  set<string> attributeNames, uniformNames;
  map<GLenum, ShaderFunctions>::iterator
      vsIt = stages.find(GL_VERTEX_SHADER);
  if(vsIt != stages.end())
  {
    ShaderFunctions &stage = vsIt->second;
    for(list<GLSLTransfer>::const_iterator
        jt=stage.inputs().begin(); jt!=stage.inputs().end(); ++jt)
    {
      attributeNames.insert(jt->name);
    }
  }
  for(map<GLenum, ShaderFunctions>::iterator
      it=stages.begin(); it!=stages.end(); ++it)
  {
    ShaderFunctions &stage = it->second;
    for(list<GLSLUniform>::const_iterator
        jt=stage.uniforms().begin(); jt!=stage.uniforms().end(); ++jt)
    {
      uniformNames.insert(jt->name);
    }
  }
  shader->setupLocations(attributeNames, uniformNames);

  return shader;
}

ref_ptr<Shader> RenderTree::generateShader(StateNode &node)
{
  ShaderConfiguration cfg;
  ShaderGenerator shaderGen;
  return generateShader(node, &shaderGen, &cfg);
}

//////////////////////////

void RenderTree::removeFromVBO(
    ref_ptr<StateNode> node,
    list< ShaderInputState* > &geomNodes)
{
  VBOState *vboState = getVBOState(node->state().get());
  if(vboState!=NULL) {
    for(list< ShaderInputState* >::iterator
        it=geomNodes.begin(); it!=geomNodes.end(); ++it)
    {
      vboState->remove(*it);
    }
    return;
  } else if(node->hasParent()) {
    removeFromVBO(node->parent(), geomNodes);
  }
}

ref_ptr<StateNode> RenderTree::getParentVBO(ref_ptr<StateNode> node)
{
  VBOState *vboState = getVBOState(node->state().get());
  if(vboState!=NULL) {
    // there is a parent node that is a vbo node
    // try to add the geometry data to this vbo.
    // if it does not fit there is no possibility left to
    // use an existing vbo and a new one must be created
    // for the geometry data.
    // for this case parent also does not contain child nodes
    // with vbo's with enough space
    return node;
  } else if(node->hasParent()) {
    return getParentVBO(node->parent());
  } else {
    return ref_ptr<StateNode>();
  }
}

static GLboolean findUnhandledGeomStates(
    ref_ptr<State> state,
    list< ShaderInputState* > *ret)
{
  if(isShaderInputState(state.get())) {
    ShaderInputState *in = (ShaderInputState*)state.get();
    if(in->interleavedAttributes().size()>0 ||
        in->sequentialAttributes().size()>0)
    {
      ret->push_back( (ShaderInputState*)state.get() );
    }
  } else if(isVBOState(state.get())) {
    return GL_FALSE;
  }

  for(list< ref_ptr<State> >::iterator
      it=state->joined().begin(); it!=state->joined().end(); ++it)
  {
    if(!findUnhandledGeomStates(*it, ret)) {
      return GL_FALSE;
    }
  }
  return GL_TRUE;
}
void RenderTree::findUnhandledGeomNodes(
    ref_ptr<StateNode> node,
    list< ShaderInputState* > *ret)
{
  if(!findUnhandledGeomStates(node->state(),ret)) {
    return;
  }
  for(list< ref_ptr<StateNode> >::iterator
      it=node->childs().begin(); it!=node->childs().end(); ++it)
  {
    findUnhandledGeomNodes(*it, ret);
  }
}


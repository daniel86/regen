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

// TODO RENDER_TREE: implement tree optimizing.
//    * remove states that are enabled before
//    * join childs with identical states (for example same material)
//    * optimize VBO usage
//    * try to avoid enable/disable costly states

static inline bool isAttributeState(State *s)
{
  return typeid(s)==typeid(AttributeState);
}

static inline bool isVBOState(State *s)
{
  return typeid(s)==typeid(VBOState);
}
static inline VBOState* getVBOState(State *s)
{
  // first try joined states, they are enabled before s
  for(list< ref_ptr<State> >::reverse_iterator
      it=s->joined().rend(); it!=s->joined().rbegin(); --it)
  {
    ref_ptr<State> &joinned = *it;
    if(isVBOState(joinned.get())) {
      return (VBOState*)joinned.get();
    }
  }
  if(isVBOState(s)) {
    return (VBOState*)s;
  } else {
    return NULL;
  }
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

GLboolean RenderTree::hasUnhandledGeometry(State *s)
{
  if(!isAttributeState(s)) { return false; }
  AttributeState *attState = (AttributeState*)(s);
  return attState->isBufferSet();
}

void RenderTree::updateStates(GLfloat dt)
{
  Stack< ref_ptr<StateNode> > nodes;
  set< ref_ptr<StateNode> > updatedGeometryNodes;
  set< State* > updatedStates;

  // wake up animation thread if it is waiting for the next frame
  // from rendering thread
  AnimationManager::get().nextFrame();

  nodes.push(rootNode_);
  while(!nodes.isEmpty()) {
    ref_ptr<StateNode> n = nodes.top();
    nodes.pop();
    State *s = n->state().get();
    if(updatedStates.count(s)==0) {
      updatedStates.insert(s);
      s->update(dt);
    }
    if(hasUnhandledGeometry(s)) {
      updatedGeometryNodes.insert(n);
    }
    for(list< ref_ptr<State> >::iterator
        it=s->joined().begin(); it!=s->joined().end(); ++it)
    {
      s = it->get();
      if(updatedStates.count(s)==0) {
        updatedStates.insert(s);
        s->update(dt);
      }
      if(hasUnhandledGeometry(s)) {
        updatedGeometryNodes.insert(n);
      }
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
    ref_ptr<StateNode> updatedNode = *it;
    addChild(updatedNode->parent(), updatedNode, true);
  }

  // wait for animation thread if it was slower then the rendering thread,
  // animations must do a step each frame
  AnimationManager::get().waitForStep();
  // some animations modify the vertex data,
  // updating the vbo needs a context so we do it here in the main thread..
  AnimationManager::get().updateGraphics(dt);
  // invoke event handler of queued events
  EventObject::emitQueued();
}

void RenderTree::addChild(
    ref_ptr<StateNode> &parent,
    ref_ptr<StateNode> &child,
    GLboolean generateVBONode)
{
  // get geometry defined in the child tree
  // of the node that has no VBO parent yet
  set< AttributeState* > attributeStates;
  findUnhandledGeomNodes(child, attributeStates);

  if(child->hasParent()) {
    child->parent()->removeChild(child);
  }

  if(attributeStates.empty()) {
    // no unhandled geometry in child tree
    child->set_parent(parent);
    parent->addChild(child);
  } else {
    if(child->hasParent()) {
      // re setting parent requires removing the child
      // from the parent VBO
      removeFromParentVBO(child->parent(), child, attributeStates);
    }

    // first try to add to existing vbo node
    if(!addToParentVBO(parent, child, attributeStates))
    {
      if(generateVBONode) {
        // there is no vbo node the geometry data can be added to
        // so we need to create a new vbo node here as parent of the
        // geometry nodes
        ref_ptr<State> vboState = ref_ptr<State>::manage(
            new VBOState(attributeStates));
        ref_ptr<StateNode> vboNode = ref_ptr<StateNode>::manage(
            new StateNode(vboState));
        vboNode->set_parent(parent);
        parent->addChild(vboNode);
        child->set_parent(vboNode);
        vboNode->addChild(child);
      } else { // !generateVBONode
        child->set_parent(parent);
        parent->addChild(child);
      }
    }
  }
}

void RenderTree::remove(ref_ptr<StateNode> &node)
{
  if(node->hasParent()) {
    // remove geometry data from parent VBO
    set< AttributeState* > geomNodes;
    findUnhandledGeomNodes(node, geomNodes);
    removeFromParentVBO(node->parent(), node, geomNodes);
    // remove parent-node connection
    node->parent()->removeChild(node);
    ref_ptr<StateNode> noParent;
    node->set_parent(noParent);
    for(set< AttributeState* >::iterator
        it=geomNodes.begin(); it!=geomNodes.end(); ++it)
    {
      (*it)->setBuffer(0);
    }
  }
}

void RenderTree::traverse(RenderState *state)
{
  rootNode_->traverse(state);
}

void RenderTree::traverse(RenderState *state, ref_ptr<StateNode> &node)
{
  node->traverse(state);
}

ref_ptr<Shader> RenderTree::generateShader(
    StateNode &node,
    ShaderGenerator *shaderGen,
    ShaderConfiguration *cfg)
{
  ref_ptr<Shader> shader = ref_ptr<Shader>::manage(new Shader);

  // let parent nodes and state configure the shader
  node.configureShader(cfg);
  shaderGen->generate(cfg);

  map<GLenum, ShaderFunctions> stages = shaderGen->getShaderStages();
  // generate glsl source for stages
  map<GLenum, string> stagesStr;
  for(map<GLenum, ShaderFunctions>::iterator
      it=stages.begin(); it!=stages.end(); ++it)
  {
    stagesStr[it->first] = ShaderManager::generateSource(it->second, it->first);
  }

  // compile and link the shader
  shader->compile(stagesStr); {
    // call glTransformFeedbackVaryings
    list<string> tfNames = ShaderManager::getValidTransformFeedbackNames(
        stages, cfg->transformFeedbackAttributes);
    shader->setupTransformFeedback(tfNames);
    // bind fragment outputs collected at parent nodes
    list<ShaderOutput> outputs;
    for(list<ShaderFragmentOutput*>::const_iterator
        it=cfg->fragmentOutputs.begin(); it!=cfg->fragmentOutputs.end(); ++it)
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
    for(set<GLSLTransfer>::const_iterator
        jt=stage.inputs().begin(); jt!=stage.inputs().end(); ++jt)
    {
      attributeNames.insert(jt->name);
    }
  }
  for(map<GLenum, ShaderFunctions>::iterator
      it=stages.begin(); it!=stages.end(); ++it)
  {
    ShaderFunctions &stage = it->second;
    for(set<GLSLUniform>::const_iterator
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

void RenderTree::removeFromParentVBO(
    ref_ptr<StateNode> &parent,
    ref_ptr<StateNode> &child,
    const set< AttributeState* > &geomNodes)
{
  VBOState *vboState = getVBOState(parent->state().get());
  if(vboState!=NULL) {
    for(set< AttributeState* >::const_iterator
        it=geomNodes.begin(); it!=geomNodes.end(); ++it)
    {
      vboState->remove(*it);
    }
  }

  if(parent->hasParent()) {
    return removeFromParentVBO(parent->parent(), parent, geomNodes);
  }
}

bool RenderTree::addToParentVBO(
    ref_ptr<StateNode> &parent,
    ref_ptr<StateNode> &child,
    set< AttributeState* > &geomNodes)
{
  VBOState *vboState = getVBOState(parent->state().get());
  if(vboState!=NULL) {
    // there is a parent node that is a vbo node
    // try to add the geometry data to this vbo.
    // if it does not fit there is no possibility left to
    // use an existing vbo and a new one must be created
    // for the geometry data.
    // for this case parent also does not contain child nodes
    // with vbo's with enough space
    return vboState->add(geomNodes);
  }

  // find vbo children of node breadth-first
  queue< ref_ptr<StateNode> > nodes;
  StateNode *x = parent.get();
  while(true)
  {
    // add all children to the queue,
    // nodes on the same level will be processed before
    // children
    if(x!=NULL)
    {
      for(list< ref_ptr<StateNode> >::iterator
          it=x->childs().begin(); it!=x->childs().end(); ++it)
      {
        nodes.push(*it);
      }
    }
    // no childs left in queue
    if(nodes.empty()) { break; }

    // pop first node and check if it is a vbo node
    ref_ptr<StateNode> first = nodes.front();
    nodes.pop();
    if(first.get() == child.get()) {
      // skip the child that called addToParentVBO
      continue;
    }
    vboState = getVBOState(first->state().get());
    if(vboState == NULL) {
      // try to find vbo nodes in children list
      // of first
      x = first.get();
      continue;
    }

    // do not add child of vbo node
    x = NULL;
    // add to the node
    if(vboState->add(geomNodes)) {
      // adding successful
      // set vboNode to be the only child of parent,
      // add old children of parent as children of vbo node
      for(list< ref_ptr<StateNode> >::iterator
          it=parent->childs().begin(); it!=parent->childs().end(); ++it)
      {
        ref_ptr<StateNode> &n = *it;
        parent->removeChild(n);
        first->addChild(n);
      }
      parent->addChild(first);
      return true;
    }
  }

  if(parent->hasParent()) {
    return addToParentVBO(parent->parent(), parent, geomNodes);
  }
}

void RenderTree::findUnhandledGeomNodes(
    ref_ptr<StateNode> &node,
    set< AttributeState* > &ret)
{
  ref_ptr<State> &nodeState = node->state();
  if(isAttributeState(nodeState.get())) {
    ret.insert( (AttributeState*)nodeState.get() );
  } else if(isVBOState(nodeState.get())) {
    return;
  }

  for(list< ref_ptr<State> >::iterator
      it=nodeState->joined().begin(); it!=nodeState->joined().end(); ++it)
  {
    ref_ptr<State> &state = node->state();
    if(isAttributeState(state.get())) {
      ret.insert( (AttributeState*)state.get() );
    } else if(isVBOState(state.get())) {
      return;
    }
  }

  for(list< ref_ptr<StateNode> >::iterator
      it=node->childs().begin(); it!=node->childs().end(); ++it)
  {
    findUnhandledGeomNodes(*it, ret);
  }
}


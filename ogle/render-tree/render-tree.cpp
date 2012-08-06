/*
 * render-tree.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <queue>

#include "render-tree.h"
#include <ogle/shader/shader-manager.h>

static inline bool isAttributeState(StateNode *n)
{
  // TODO: joinned states ?
  return typeid(n->state().get())==typeid(AttributeState);
}
static inline bool isVBOState(StateNode *n)
{
  return typeid(n->state().get())==typeid(VBOState);
}

RenderTree::RenderTree(ref_ptr<StateNode> &node)
{
  for(rootNode_=node;
      rootNode_->hasParent();
      rootNode_=rootNode_->parent());
}

RenderTree::RenderTree()
{
  rootNode_ = ref_ptr<StateNode>::manage(new StateNode);
}

ref_ptr<StateNode>& RenderTree::rootNode()
{
  return rootNode_;
}

void RenderTree::addChild(
    ref_ptr<StateNode> &parent,
    ref_ptr<StateNode> &child,
    bool generateVBONode)
{
  // get geometry defined in the child tree
  // of the node that has no VBO parent yet
  list< ref_ptr<AttributeState> > attributeStates;
  findUnhandledGeomNodes(child, attributeStates);

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
    list< ref_ptr<AttributeState> > geomNodes;
    findUnhandledGeomNodes(node, geomNodes);
    removeFromParentVBO(node->parent(), node, geomNodes);
    // remove parent-node connection
    node->parent()->removeChild(node);
    ref_ptr<StateNode> noParent;
    node->set_parent(noParent);
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
    ShaderConfiguration *cfg)
{
  ref_ptr<Shader> shader = ref_ptr<Shader>::manage(new Shader);
  // generate shader code
  ShaderGenerator shaderGen;

  // let parent nodes and state configure the shader
  node.configureShader(cfg);
  shaderGen.generate(cfg);

  map<GLenum, ShaderFunctions> stages = shaderGen.getShaderStages();
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
  return generateShader(node, &cfg);
}

void RenderTree::optimize()
{
  // * join nodes with only a single child
  // * do not duplicate nodes with draw calls
  // * do something special with vbo ?
  // * some priority for states would make sense
  //    * it is for some states more costly to call enable/disable then for others
}

//////////////////////////

//////////////////////////

void RenderTree::removeFromParentVBO(
    ref_ptr<StateNode> &parent,
    ref_ptr<StateNode> &child,
    const list< ref_ptr<AttributeState> > &geomNodes)
{
  if(isVBOState(parent.get())) {
    VBOState *vboNode = (VBOState*)(parent.get());
    for(list< ref_ptr<AttributeState> >::const_iterator
        it=geomNodes.begin(); it!=geomNodes.end(); ++it)
    {
      vboNode->remove(it->get());
    }
  }

  if(parent->hasParent()) {
    return removeFromParentVBO(parent->parent(), parent, geomNodes);
  }
}

bool RenderTree::addToParentVBO(
    ref_ptr<StateNode> &parent,
    ref_ptr<StateNode> &child,
    list< ref_ptr<AttributeState> > &geomNodes)
{
  if(isVBOState(parent.get())) {
    // there is a parent node that is a vbo node
    // try to add the geometry data to this vbo.
    // if it does not fit there is no possibility left to
    // use an existing vbo and a new one must be created
    // for the geometry data.
    // for this case parent also does not contain child nodes
    // with vbo's with enough space
    VBOState *vboState = (VBOState*)(parent.get());
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
    if(!isVBOState(first.get())) {
      // try to find vbo nodes in children list
      // of first
      x = first.get();
      continue;
    }

    // do not add child of vbo node
    x = NULL;
    // add to the node
    VBOState *vboState = (VBOState*)(first->state().get());
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
    list< ref_ptr<AttributeState> > &ret)
{
  if(isAttributeState(node.get())) {
    ret.push_back(ref_ptr<AttributeState>::cast(node));
  }
  if(isVBOState(node.get())) {
    // children are expected to be handled by VBO already
    return;
  }
  for(list< ref_ptr<StateNode> >::iterator
      it=node->childs().begin(); it!=node->childs().end(); ++it)
  {
    findUnhandledGeomNodes(*it, ret);
  }
}


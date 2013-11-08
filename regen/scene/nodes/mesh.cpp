/*
 * mesh.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "mesh.h"
using namespace regen::scene;
using namespace regen;

#include <regen/states/state-configurer.h>
#include <regen/scene/resource-manager.h>

#define REGEN_MESH_NODE_CATEGORY "mesh"

MeshNodeProvider::MeshNodeProvider()
: NodeProcessor(REGEN_MESH_NODE_CATEGORY)
{}

static ref_ptr<Shader> findShader(State *s)
{
  for(list< ref_ptr<State> >::const_reverse_iterator
      it=s->joined().rbegin(); it!=s->joined().rend(); ++it)
  {
    ref_ptr<Shader> out = findShader((*it).get());
    if(out.get()!=NULL) return out;
  }

  ShaderState *shaderState = dynamic_cast<ShaderState*>(s);
  if(shaderState!=NULL) return shaderState->shader();

  HasShader *hasShader = dynamic_cast<HasShader*>(s);
  if(hasShader!=NULL) return hasShader->shaderState()->shader();

  return ref_ptr<Shader>();
}

static ref_ptr<Shader> findShader(StateNode *n)
{
  ref_ptr<Shader> out = findShader(n->state().get());
  if(out.get()==NULL && n->hasParent()) {
    return findShader(n->parent());
  }
  else {
    return out;
  }
}

void MeshNodeProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<StateNode> &parent)
{
  ref_ptr<MeshVector> meshes =
      parser->getResources()->getMesh(parser,input.getName());
  if(meshes.get()==NULL) {
    REGEN_WARN("Unable to load Mesh for '" << input.getDescription() << "'.");
    return;
  }

  for(MeshVector::iterator it=meshes->begin(); it!=meshes->end(); ++it)
  {
    ref_ptr<Mesh> meshResource = *it;
    if(meshResource.get()==NULL) {
      REGEN_WARN("null mesh");
      continue;
    }
    ref_ptr<Mesh> mesh;
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

    StateConfigurer stateConfigurer;
    stateConfigurer.addNode(meshNode.get());
    ref_ptr<Shader> meshShader;

    // Handle shader
    HasShader *hasShader = dynamic_cast<HasShader*>(mesh.get());
    if(hasShader!=NULL) {
      hasShader->createShader(stateConfigurer.cfg());
      meshShader = hasShader->shaderState()->shader();
    }
    else if(input.hasAttribute("shader")) {
      const string shaderKey = input.getValue("shader");
      ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
      mesh->joinStates(shaderState);

      shaderState->createShader(stateConfigurer.cfg(), shaderKey);
      meshShader = shaderState->shader();
    }
    if(meshShader.get()==NULL) {
      // Try to find parent shader.
      meshShader = findShader(parent.get());
    }

    if(meshShader.get()==NULL) {
      REGEN_WARN("Unable to find shader for " << input.getDescription() << ".");
    }
    else {
      // Update VAO
      mesh->updateVAO(RenderState::get(),
          stateConfigurer.cfg(),meshShader);
    }
  }
}

/*
 * view-node.h
 *
 *  Created on: Oct 19, 2014
 *      Author: daniel
 */

#ifndef SCENE_DISPLAY_VIEW_NODE_H_
#define SCENE_DISPLAY_VIEW_NODE_H_

class ViewNodeProcessor : public NodeProcessor {
public:
	ViewNodeProcessor(ViewNodeList *viewNodes)
			: NodeProcessor("view"),
			  viewNodes_(viewNodes) {}

	// Override
	void processInput(
			SceneParser *parser,
			SceneInputNode &input,
			const ref_ptr<StateNode> &parent) {
		ref_ptr<State> state = ref_ptr<State>::alloc();
		ref_ptr<StateNode> newNode = ref_ptr<StateNode>::alloc(state);
		newNode->set_name(input.getName());
		newNode->set_isHidden(GL_TRUE);
		parent->addChild(newNode);
		parser->putNode(input.getName(), newNode);
		handleChildren(parser, input, newNode);

		ViewNode viewNode;
		viewNode.name = input.getName();
		viewNode.node = newNode;
		viewNodes_->push_back(viewNode);
		REGEN_INFO("View: " << viewNode.name);
	}

protected:
	ViewNodeList *viewNodes_;

	void handleChildren(
			SceneParser *parser,
			SceneInputNode &input,
			const ref_ptr<StateNode> &newNode) {
		// Process node children
		const list<ref_ptr<SceneInputNode> > &childs = input.getChildren();
		for (list<ref_ptr<SceneInputNode> >::const_iterator
					 it = childs.begin(); it != childs.end(); ++it) {
			const ref_ptr<SceneInputNode> &x = *it;
			// First try node processor
			ref_ptr<NodeProcessor> nodeProcessor = parser->getNodeProcessor(x->getCategory());
			if (nodeProcessor.get() != NULL) {
				nodeProcessor->processInput(parser, *x.get(), newNode);
				continue;
			}
			// Second try state processor
			ref_ptr<StateProcessor> stateProcessor = parser->getStateProcessor(x->getCategory());
			if (stateProcessor.get() != NULL) {
				stateProcessor->processInput(parser, *x.get(), newNode->state());
				continue;
			}
			REGEN_WARN("No processor registered for '" << x->getDescription() << "'.");
		}
	}
};

#endif /* SCENE_DISPLAY_VIEW_NODE_H_ */

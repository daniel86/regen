#ifndef REGEN_SCENE_PICKING_H_
#define REGEN_SCENE_PICKING_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/states/geometric-picking.h>
#include "regen/scene/nodes/scene-node.h"
#include <regen/application.h>

#define REGEN_PICKING_STATE_CATEGORY "picking"

namespace regen {
	namespace scene {
		/**
		 * Updates hovered node in the application.
		 */
		class PickingUpdater : public State {
		public:
			explicit PickingUpdater(Application *app, const ref_ptr<GeomPicking> &geomPicking)
					: State(), app_(app), geomPicking_(geomPicking) {}

			void disable(RenderState *rs) override {
				State::disable(rs);
				if (geomPicking_->hasPickedObject()) {
					auto &pickedNode = app_->getObjectWithID(geomPicking_->pickedObject()->objectID);
					if (pickedNode.get() != nullptr) {
						app_->setHoveredObject(pickedNode, geomPicking_->pickedObject());
					} else {
						REGEN_WARN("Unable to find object with ID " <<
							geomPicking_->pickedObject()->objectID << " in scene.");
						app_->unsetHoveredObject();
					}
				} else {
					app_->unsetHoveredObject();
				}
			}

		protected:
			Application *app_;
			ref_ptr<GeomPicking> geomPicking_;
		};

		/**
		 * Processes SceneInput and creates picking node which is
		 * a root node for performing object picking.
		 */
		class PickingNodeProvider : public NodeProcessor {
		public:
			PickingNodeProvider()
					: NodeProcessor(REGEN_PICKING_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<StateNode> &parent) override {
				auto userCamera = parser->getResources()->getCamera(parser, input.getValue("camera"));
				if (userCamera.get() == nullptr) {
					REGEN_WARN("Unable to find Camera for '" << input.getDescription() << "'.");
					return;
				}
				auto &mouseTexco = parser->getMouseTexco();
				auto geomPicking = ref_ptr<GeomPicking>::alloc(userCamera, mouseTexco);
				parent->addChild(geomPicking);

				// create picking updater
				auto pickingUpdater = ref_ptr<PickingUpdater>::alloc(parser->application(), geomPicking);
				geomPicking->state()->joinStates(pickingUpdater);

				// load children nodes
				SceneNodeProcessor::handleChildren(parser, input, geomPicking);
			}
		};
	}
}

#endif /* REGEN_SCENE_PICKING_H_ */

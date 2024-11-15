/*
 * material.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_MATERIAL_H_
#define REGEN_SCENE_MATERIAL_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/scene/resource-manager.h>

#define REGEN_MATERIAL_STATE_CATEGORY "material"

#include <regen/states/fbo-state.h>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates Material's.
		 */
		class MaterialStateProvider : public StateProcessor {
		public:
			MaterialStateProvider()
					: StateProcessor(REGEN_MATERIAL_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &state) override {
				ref_ptr<Material> mat = ref_ptr<Material>::alloc();

				if (input.hasAttribute("asset")) {
					ref_ptr<AssetImporter> assetLoader =
							parser->getResources()->getAsset(parser, input.getValue("asset"));
					if (assetLoader.get() == nullptr) {
						REGEN_WARN("Skipping unknown Asset for '" << input.getDescription() << "'.");
					} else {
						const vector<ref_ptr<Material> > materials = assetLoader->materials();
						auto materialIndex = input.getValue<GLuint>("asset-index", 0u);
						if (materialIndex >= materials.size()) {
							REGEN_WARN("Invalid Material index '" << materialIndex <<
																  "' for Asset '" << input.getValue("asset") << "'.");
						} else {
							mat = materials[materialIndex];
						}
					}
				} else if (input.hasAttribute("preset")) {
					string presetVal(input.getValue("preset"));
					if (presetVal == "jade") mat->set_jade();
					else if (presetVal == "ruby") mat->set_ruby();
					else if (presetVal == "chrome") mat->set_chrome();
					else if (presetVal == "gold") mat->set_gold();
					else if (presetVal == "copper") mat->set_copper();
					else if (presetVal == "silver") mat->set_silver();
					else if (presetVal == "pewter") mat->set_pewter();
					else {
						REGEN_WARN("Unknown Material preset '" << presetVal <<
															   "' for node '" << input.getDescription() << "'.");
						mat->set_copper();
					}
				}
				if (input.hasAttribute("ambient"))
					mat->ambient()->setVertex(0,
											  input.getValue<Vec3f>("ambient", Vec3f(0.0f)));
				if (input.hasAttribute("diffuse"))
					mat->diffuse()->setVertex(0,
											  input.getValue<Vec3f>("diffuse", Vec3f(1.0f)));
				if (input.hasAttribute("specular"))
					mat->specular()->setVertex(0,
											   input.getValue<Vec3f>("specular", Vec3f(0.0f)));
				if (input.hasAttribute("shininess"))
					mat->shininess()->setVertex(0,
												input.getValue<GLfloat>("shininess", 1.0f));
				if (input.hasAttribute("emission"))
					mat->set_emission(input.getValue<Vec3f>("emission", Vec3f(0.0f)));

				mat->alpha()->setVertex(0,
										input.getValue<GLfloat>("alpha", 1.0f));
				mat->refractionIndex()->setVertex(0,
												  input.getValue<GLfloat>("refractionIndex", 0.95f));
				mat->set_fillMode(glenum::fillMode(
						input.getValue<string>("fill-mode", "FILL")));
				if (input.getValue<bool>("two-sided", false)) {
					// this conflicts with shadow mapping front face culling.
					mat->set_twoSided(true);
				}

				state->joinStates(mat);
			}
		};
	}
}

#endif /* REGEN_SCENE_MATERIAL_H_ */

/*
 * blit.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_BLIT_H_
#define REGEN_SCENE_BLIT_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/scene/resource-manager.h>

#define REGEN_BLIT_STATE_CATEGORY "blit"

#include <regen/states/blit-state.h>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates BlitState.
		 */
		class BlitStateProvider : public StateProcessor {
		public:
			BlitStateProvider()
					: StateProcessor(REGEN_BLIT_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &state) override {
				if (!input.hasAttribute("src-fbo")) {
					REGEN_WARN("Ignoring " << input.getDescription() << " without src-fbo attribute.");
					return;
				}
				if (!input.hasAttribute("dst-fbo")) {
					REGEN_WARN("Ignoring " << input.getDescription() << " without dst-fbo attribute.");
					return;
				}
				const string srcID = input.getValue("src-fbo");
				const string dstID = input.getValue("dst-fbo");
				ref_ptr<FBO> src, dst;
				if (srcID != "SCREEN") {
					src = parser->getResources()->getFBO(parser, srcID);
					if (src.get() == nullptr) {
						REGEN_WARN("Unable to find FBO with name '" << srcID << "'.");
						return;
					}
				}
				if (dstID != "SCREEN") {
					dst = parser->getResources()->getFBO(parser, dstID);
					if (dst.get() == nullptr) {
						REGEN_WARN("Unable to find FBO with name '" << dstID << "'.");
						return;
					}
				}
				bool keepAspect = input.getValue<GLuint>("keep-aspect", false);
				if (src.get() != nullptr && dst.get() != nullptr) {
					if (input.getValue("src-attachment") == "depth" || input.getValue("dst-attachment") == "depth") {
						auto blit = ref_ptr<BlitToFBO>::alloc(
								src, dst,
								GL_DEPTH_ATTACHMENT,
								GL_DEPTH_ATTACHMENT,
								keepAspect);
						blit->set_sourceBuffer(GL_DEPTH_BUFFER_BIT);
						blit->set_filterMode(GL_NEAREST);
						state->joinStates(blit);
						return;
					} else {
						auto srcAttachment = input.getValue<GLuint>("src-attachment", 0u);
						auto dstAttachment = input.getValue<GLuint>("dst-attachment", 0u);
						state->joinStates(ref_ptr<BlitToFBO>::alloc(
								src, dst,
								GL_COLOR_ATTACHMENT0 + srcAttachment,
								GL_COLOR_ATTACHMENT0 + dstAttachment,
								keepAspect));
					}
				} else if (src.get() != nullptr) {
					// Blit Texture to Screen
					auto srcAttachment = input.getValue<GLuint>("src-attachment", 0u);
					state->joinStates(ref_ptr<BlitToScreen>::alloc(src,
																   parser->getViewport(),
																   GL_COLOR_ATTACHMENT0 + srcAttachment,
																   keepAspect));
				} else if (dst.get() != nullptr) {
					REGEN_WARN(input.getDescription() << ", blitting Screen to FBO not supported.");
				} else {
					REGEN_WARN("No src or dst FBO specified for " << input.getDescription() << ".");
				}
			}
		};
	}
}

#endif /* REGEN_SCENE_BLIT_H_ */

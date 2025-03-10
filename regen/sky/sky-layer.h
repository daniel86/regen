/*
 * sky-layer.h
 *
 *  Created on: Jan 4, 2014
 *      Author: daniel
 */

#ifndef SKY_LAYER_H_
#define SKY_LAYER_H_

#include <regen/meshes/mesh-state.h>
#include <regen/states/state.h>
#include <regen/states/state-node.h>
#include <regen/states/shader-state.h>

namespace regen {
	class Sky;

	class SkyLayer : public StateNode {
	public:
		explicit SkyLayer(const ref_ptr<Sky> &sky);

		~SkyLayer() override;

		void updateSky(RenderState *rs, GLdouble dt);

		virtual void set_updateInterval(GLdouble interval_ms) { updateInterval_ = interval_ms; }

		virtual GLdouble updateInterval() const { return updateInterval_; }

		virtual void updateSkyLayer(RenderState *rs, GLdouble dt) {}

		virtual void createUpdateShader() {}

		ref_ptr<State> updateState() { return updateState_; }

		virtual ref_ptr<Mesh> getMeshState() = 0;

		virtual ref_ptr<HasShader> getShaderState() = 0;

	protected:
		ref_ptr<Sky> sky_;
		ref_ptr<State> updateState_;

		GLdouble updateInterval_;
		GLdouble dt_;
	};

	class SkyLayerView : public SkyLayer {
	public:
		SkyLayerView(const ref_ptr<Sky> &sky, const ref_ptr<SkyLayer> &source)
				: SkyLayer(sky) {
			source_ = source;
			state()->joinStates(source_->state());
			shader_ = ref_ptr<HasShader>::alloc(source->getShaderState()->shaderKey());
			state()->joinStates(shader_->shaderState());
			mesh_ = ref_ptr<Mesh>::alloc(source->getMeshState());
			state()->joinStates(mesh_);
		}

		void set_updateInterval(GLdouble interval_ms) override { source_->set_updateInterval(interval_ms); }

		GLdouble updateInterval() const override { return source_->updateInterval(); }

		ref_ptr<Mesh> getMeshState() override { return mesh_; }

		ref_ptr<HasShader> getShaderState() override { return shader_; }

		void updateSkyLayer(RenderState *rs, GLdouble dt) override {}

	protected:
		ref_ptr<SkyLayer> source_;
		ref_ptr<Mesh> mesh_;
		ref_ptr<HasShader> shader_;
	};
}

#endif /* SKY_LAYER_H_ */

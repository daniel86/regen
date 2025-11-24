#include "light-camera.h"

using namespace regen;

namespace regen {
	class LightCameraAnimation : public Animation {
	public:
		explicit LightCameraAnimation(LightCamera *camera)
				: Animation(false, true),
				  camera_(camera) {}
		void cpuUpdate(double dt) override {
			if(camera_->updateLight()) {
				camera_->lightCamera()->updateShaderData(static_cast<float>(dt));
				camera_->updateShadowData();
			}
		}
	private:
		LightCamera *camera_;
	};
}

LightCamera::LightCamera(const ref_ptr<Light> &light, Camera *camera)
		: light_(light), camera_(camera) {
	v_lightMatrix_.resize(1, Mat4f::identity());
	sh_lightMatrix_ = ref_ptr<ShaderInputMat4>::alloc("lightMatrix");
	sh_lightMatrix_->setUniformUntyped();

	shadowBuffer_ = ref_ptr<UBO>::alloc("Shadow", light_->lightUBO()->bufferUpdateHints());
	shadowBuffer_->setStagingAccessMode(BUFFER_CPU_WRITE);
	shadowBuffer_->addStagedInput(sh_lightMatrix_);

	lightCameraAnimation_ = ref_ptr<LightCameraAnimation>::alloc(this);
	lightCameraAnimation_->setAnimationName("light-camera");
	lightCameraAnimation_->startAnimation();
}

void LightCamera::updateShadowData() {
	auto m_data = sh_lightMatrix_->mapClientData<Mat4f>(BUFFER_GPU_WRITE);
	std::memcpy(m_data.w.data(), v_lightMatrix_.data(), v_lightMatrix_.size() * sizeof(Mat4f));
}

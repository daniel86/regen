#ifndef REGEN_LIGHT_STATE_H
#define REGEN_LIGHT_STATE_H

#include <regen/camera/camera.h>
#include <regen/compute/vector.h>
#include <regen/objects/model-transformation.h>
#include <regen/animation/bone-tree.h>
#include <regen/animation/animation.h>

namespace regen {
	/**
	 * \brief A light emitting point in space.
	 */
	class Light : public State {
	public:
		static constexpr const char *TYPE_NAME = "Light";

		/**
		 * \brief defines the light type
		 */
		enum Type {
			DIRECTIONAL,//!< directional light
			SPOT,       //!< spot light
			POINT       //!< point light
		};

		/**
		 * @param lightType the light type.
		 */
		explicit Light(Type lightType,
				const BufferUpdateFlags &updateFlags = BufferUpdateFlags::FULL_PER_FRAME);

		static ref_ptr<Light> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * @return the light type.
		 */
		Type lightType() const { return lightType_; }

		/**
		 * @return the light uniforms.
		 */
		const ref_ptr<UBO> &lightUBO() const { return lightBuffer_; }

		/**
		 * Sets whether the light is distance attenuated.
		 */
		void set_isAttenuated(bool isAttenuated) { isAttenuated_ = isAttenuated; }

		/**
		 * @return is the light distance attenuated.
		 */
		bool isAttenuated() const { return isAttenuated_; }

		/**
		 * Get the shader input for the light position.
		 * @return the shader input for the light position.
		 */
		const ref_ptr<ShaderInput4f> &position() const { return lightPosition_; }

		/**
		 * Get the light position for a specific layer which has arrived in staging.
		 * @param idx the layer index.
		 * @return the light position for the specified layer.
		 */
		ClientVertex_rw<Vec4f> positionStaged(uint32_t idx) const {
			return lightPosition_->mapClientVertex<Vec4f>(BUFFER_GPU_READ, idx);
		}

		/**
		 * Sets the light position for a specific layer.
		 * @param idx the layer index.
		 * @param v the light position to set.
		 */
		void setPosition(uint32_t idx, const Vec3f &v) {
			lightPosition_->setVertex3(idx, v);
		}

		/**
		 * Get the shader input for the light direction.
		 * @return the shader input for the light direction.
		 */
		const ref_ptr<ShaderInput3f> &direction() const { return lightDirection_; }

		/**
		 * Get the light direction for a specific layer.
		 * @param idx the layer index.
		 * @return the light direction for the specified layer.
		 */
		ClientVertex_rw<Vec3f> directionStaged(uint32_t idx) const {
			return lightDirection_->mapClientVertex<Vec3f>(BUFFER_GPU_READ, idx);
		}

		/**
		 * Sets the light direction for a specific layer.
		 * @param idx the layer index.
		 * @param v the light direction to set.
		 */
		void setDirection(uint32_t idx, const Vec3f &v) {
			lightDirection_->setVertex(idx, v);
		}

		/**
		 * Get the shader input for the light radius.
		 * @return the shader input for the light radius.
		 */
		const ref_ptr<ShaderInput2f> &radius() const { return lightRadius_; }

		/**
		 * Get the light radius for a specific layer.
		 * @param idx the layer index.
		 * @return the light radius for the specified layer.
		 */
		ClientVertex_rw<Vec2f> radiusStaged(uint32_t idx) const {
			return lightRadius_->mapClientVertex<Vec2f>(BUFFER_GPU_READ, idx);
		}

		/**
		 * Get the cone angle for a specific layer.
		 * @param idx the layer index.
		 * @return the cone angle for the specified layer.
		 */
		ClientVertex_rw<Vec2f> coneAngleStaged(uint32_t idx) const {
			return lightConeAngles_->mapClientVertex<Vec2f>(BUFFER_GPU_READ, idx);
		}

		/**
		 * Get the shader input for the light cone angles.
		 * @return the shader input for the light cone angles.
		 */
		const ref_ptr<ShaderInput2f> &coneAngle() const { return lightConeAngles_; }

		/**
		 * @param deg inner angle in degree.
		 */
		void setConeAngles(float inner, float outer);

		/**
		 * Sets the light radius for a specific layer.
		 * @param idx the layer index.
		 * @param v the light radius to set.
		 */
		void setRadius(uint32_t idx, const Vec2f &v) {
			lightRadius_->setVertex(idx, v);
		}

		/**
		 * Get the diffuse light color for a specific layer.
		 * @param idx the layer index.
		 * @return the diffuse light color for the specified layer.
		 */
		void setDiffuse(uint32_t idx, const Vec3f &v) {
			lightDiffuse_->setVertex(idx, v);
		}

		/**
		 * Get the specular light color for a specific layer.
		 * @param idx the layer index.
		 * @return the specular light color for the specified layer.
		 */
		void setSpecular(uint32_t idx, const Vec3f &v) {
			lightSpecular_->setVertex(idx, v);
		}

		/**
		 * Updates the cone matrix.
		 */
		bool updateConeMatrix();

	protected:
		const Type lightType_;
		bool isAttenuated_;

		ref_ptr<UBO> lightBuffer_;
		ref_ptr<ShaderInput4f> lightPosition_;
		ref_ptr<ShaderInput3f> lightDirection_;
		ref_ptr<ShaderInput3f> lightDiffuse_;
		ref_ptr<ShaderInput3f> lightSpecular_;
		ref_ptr<ShaderInput2f> lightConeAngles_;
		ref_ptr<ShaderInput2f> lightRadius_;
		ref_ptr<ShaderInputMat4> coneMatrix_;
		uint32_t lightConeStamp_ = 0u;

		ref_ptr<Animation> coneAnimation_;
	};

	std::ostream &operator<<(std::ostream &out, const Light::Type &v);

	std::istream &operator>>(std::istream &in, Light::Type &v);

	/**
	 * \brief Configures filtering.
	 */
	enum ShadowFilterMode {
		SHADOW_FILTERING_NONE,        //!< No special filtering
		SHADOW_FILTERING_PCF_GAUSSIAN,//!< PCF filtering using Gauss kernel
		SHADOW_FILTERING_VSM          //!< VSM filtering
	};

	std::ostream &operator<<(std::ostream &out, const ShadowFilterMode &mode);

	std::istream &operator>>(std::istream &in, ShadowFilterMode &mode);
} // namespace

#endif /* REGEN_LIGHT_STATE_H */

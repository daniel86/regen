#ifndef REGEN_LIGHT_STATE_H
#define REGEN_LIGHT_STATE_H

#include <regen/states/model-transformation.h>
#include <regen/camera/camera.h>
#include <regen/math/vector.h>
#include <regen/meshes/primitives/cone.h>
#include <regen/animations/animation-node.h>
#include <regen/animations/animation.h>

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
		 * @return the world space light position.
		 * @note undefined for directional lights.
		 */
		const std::vector<Vec4f> &position() const { return lightPosition_; }

		/**
		 * Get the shader input for the light position.
		 * @return the shader input for the light position.
		 */
		const ref_ptr<ShaderInput4f> &sh_position() const { return sh_lightPosition_; }

		/**
		 * Get the light position for a specific layer.
		 * @param idx the layer index.
		 * @return the light position for the specified layer.
		 */
		const Vec4f &position(uint32_t idx) const { return lightPosition_[idx]; }

		/**
		 * @return the stamp indicating when the light position was last updated.
		 */
		uint32_t positionStamp() const { return lightPosStamp_; }

		/**
		 * Sets the light position for a specific layer.
		 * @param idx the layer index.
		 * @param v the light position to set.
		 */
		void setPosition(uint32_t idx, const Vec3f &v) {
			setStamped3(lightPosition_, lightPosStamp_, idx, v);
		}

		/**
		 * @return the light direction.
		 * @note undefined for point lights.
		 */
		const std::vector<Vec3f> &direction() const { return lightDirection_; }

		/**
		 * Get the light direction for a specific layer.
		 * @param idx the layer index.
		 * @return the light direction for the specified layer.
		 */
		const Vec3f &direction(uint32_t idx) const { return lightDirection_[idx]; }

		/**
		 * @return the stamp indicating when the light direction was last updated.
		 */
		uint32_t directionStamp() const { return lightDirStamp_; }

		/**
		 * Sets the light direction for a specific layer.
		 * @param idx the layer index.
		 * @param v the light direction to set.
		 */
		void setDirection(uint32_t idx, const Vec3f &v) {
			setStamped(lightDirection_, lightDirStamp_, idx, v);
		}

		/**
		 * @return diffuse light color.
		 */
		const std::vector<Vec3f> &diffuse() const { return lightDiffuse_; }

		/**
		 * Get the diffuse light color for a specific layer.
		 * @param idx the layer index.
		 * @return the diffuse light color for the specified layer.
		 */
		void setDiffuse(uint32_t idx, const Vec3f &v) {
			setStamped(lightDiffuse_, lightDiffuseStamp_, idx, v);
		}

		/**
		 * @return specular light color.
		 */
		const std::vector<Vec3f> &specular() const { return lightSpecular_; }

		/**
		 * Get the specular light color for a specific layer.
		 * @param idx the layer index.
		 * @return the specular light color for the specified layer.
		 */
		void setSpecular(uint32_t idx, const Vec3f &v) {
			setStamped(lightSpecular_, lightSpecularStamp_, idx, v);
		}

		/**
		 * @return inner and outer light radius.
		 */
		const std::vector<Vec2f> &radius() const { return lightRadius_; }

		/**
		 * Get the light radius for a specific layer.
		 * @param idx the layer index.
		 * @return the light radius for the specified layer.
		 */
		const Vec2f &radius(uint32_t idx) const { return lightRadius_[idx]; }

		/**
		 * @return the stamp indicating when the light radius was last updated.
		 */
		uint32_t radiusStamp() const { return lightRadiusStamp_; }

		/**
		 * Sets the light radius for a specific layer.
		 * @param idx the layer index.
		 * @param v the light radius to set.
		 */
		void setRadius(uint32_t idx, const Vec2f &v) {
			setStamped(lightRadius_, lightRadiusStamp_, idx, v);
		}

		/**
		 * @return inner and outer cone angles.
		 */
		const std::vector<Vec2f> &coneAngle() const { return lightConeAngles_; }

		/**
		 * Get the cone angle for a specific layer.
		 * @param idx the layer index.
		 * @return the cone angle for the specified layer.
		 */
		const Vec2f &coneAngle(uint32_t idx) const { return lightConeAngles_[idx]; }

		/**
		 * @return the stamp indicating when the cone angles were last updated.
		 */
		uint32_t coneAngleStamp() const { return lightConeAnglesStamp_; }

		/**
		 * @param deg inner angle in degree.
		 */
		void set_innerConeAngle(float deg);

		/**
		 * @param deg outer angle in degree.
		 */
		void set_outerConeAngle(float deg);

		/**
		 * Updates the cone matrix.
		 */
		bool updateConeMatrix();

		/**
		 * @brief Update the shader data for this light.
		 */
		void updateShaderData();

		/**
		 * Make sure local data arrays match size of shader data arrays.
		 */
		void resizeLocalData();

	protected:
		const Type lightType_;
		bool isAttenuated_;

		ref_ptr<UBO> lightBuffer_;
		ref_ptr<ShaderInput4f> sh_lightPosition_;
		ref_ptr<ShaderInput3f> sh_lightDirection_;
		ref_ptr<ShaderInput3f> sh_lightDiffuse_;
		ref_ptr<ShaderInput3f> sh_lightSpecular_;
		ref_ptr<ShaderInput2f> sh_lightConeAngles_;
		ref_ptr<ShaderInput2f> sh_lightRadius_;
		ref_ptr<ShaderInputMat4> sh_coneMatrix_;

		std::vector<Vec4f> lightPosition_;
		std::vector<Vec3f> lightDirection_;
		std::vector<Vec3f> lightDiffuse_;
		std::vector<Vec3f> lightSpecular_;
		std::vector<Vec2f> lightConeAngles_;
		std::vector<Vec2f> lightRadius_;
		std::vector<Mat4f> coneMatrix_;

		uint32_t lightPosStamp_ = 1;
		uint32_t lightDirStamp_ = 1;
		uint32_t lightDiffuseStamp_ = 1;
		uint32_t lightSpecularStamp_ = 1;
		uint32_t lightConeAnglesStamp_ = 1;
		uint32_t lightRadiusStamp_ = 1;
		uint32_t lightConeStamp_ = 0;

		uint32_t lastPosStamp_ = 0;
		uint32_t lastDirStamp_ = 0;
		uint32_t lastDiffuseStamp_ = 0;
		uint32_t lastSpecularStamp_ = 0;
		uint32_t lastConeAnglesStamp_ = 0;
		uint32_t lastRadiusStamp_ = 0;
		uint32_t lastConeStamp_ = 0;

		ref_ptr<Animation> coneAnimation_;

		template<typename T>
		static inline void setStamped(
				std::vector<T> &vec, uint32_t &stamp, uint32_t idx, const T &value) {
			vec[idx] = value;
			stamp += 1;
		}

		static inline void setStamped3(
				std::vector<Vec4f> &vec, uint32_t &stamp, uint32_t idx, const Vec3f &value) {
			vec[idx].xyz_() = value;
			stamp += 1;
		}
	};

	std::ostream &operator<<(std::ostream &out, const Light::Type &v);

	std::istream &operator>>(std::istream &in, Light::Type &v);

	/**
	 * \brief Animates Light position using an AnimationNode.
	 */
	class LightNode : public State {
	public:
		/**
		 * @param light a light.
		 * @param n a animation node.
		 */
		LightNode(
				const ref_ptr<Light> &light,
				const ref_ptr<AnimationNode> &n);

		/**
		 * @param dt update light position using the niamtion node.
		 */
		void update(GLdouble dt);

	protected:
		ref_ptr<Light> light_;
		ref_ptr<AnimationNode> animNode_;
	};

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

#ifndef MESH_ANIMATION_GPU_H_
#define MESH_ANIMATION_GPU_H_

#include <regen/animation/animation.h>
#include "regen/shader/shader-input.h"
#include "regen/shader/shader.h"
#include <regen/objects/mesh.h>

namespace regen {
	/**
	 * \brief Animates vertex attributes.
	 *
	 * The animation is done using key frames. At least
	 * two frames are loaded into VRAM during animation.
	 * A shader is used to compute an interpolation between those
	 * two frames. Transform feedback is used to read
	 * the interpolated value into a feedback buffer.
	 * This interpolated value is used by meshes during
	 * regular rendering.
	 */
	class MeshAnimation : public Animation {
	public:
		/**
		 * Interpolation is done in a GLSL shader.
		 * Different modes can be accessed by their name.
		 * This struct is used to select the interpolation mode
		 * used in the generated shader.
		 */
		struct Interpolation {
			std::string attributeName; /**< attribute to interpolate. */
			std::string interpolationName; /**< name of the interpolation. */
			std::string interpolationKey; /**< include path for the interpolation GLSL code. */

			/**
			 * @param a_name attribute name.
			 * @param i_name interpolation mode name.
			 */
			Interpolation(std::string_view a_name, std::string_view i_name)
					: attributeName(a_name), interpolationName(i_name), interpolationKey("") {}

			/**
			 * @param a_name attribute name.
			 * @param i_name interpolation mode name.
			 * @param i_key interpolation include key.
			 */
			Interpolation(std::string_view a_name, std::string_view i_name, std::string_view i_key)
					: attributeName(a_name), interpolationName(i_name), interpolationKey(i_key) {}
		};

		/**
		 * @param mesh a mesh.
		 * @param interpolations list of interpolation modes for attributes.
		 */
		MeshAnimation(const ref_ptr<Mesh> &mesh, const std::list<Interpolation> &interpolations);

		/**
		 * Set the active tick range.
		 * This resets some internal states and the animation will continue
		 * next step with the start tick of the given range.
		 * @param tickRange number of ticks for this morph.
		 */
		void setTickRange(const Vec2d &tickRange);

		/**
		 * Set the friction of the animation.
		 */
		void setFriction(float friction);

		/**
		 * Set the frequency of the animation.
		 */
		void setFrequency(float frequency);

		/**
		 * Add a custom mesh frame.
		 * @param attributes target attributes.
		 * @param timeInTicks number of ticks for this morph.
		 */
		void addFrame(
				const std::list<ref_ptr<ShaderInput> > &attributes,
				double timeInTicks);

		/**
		 * Add a frame for the original mesh attributes.
		 * @param timeInTicks number of ticks for this morph.
		 */
		void addMeshFrame(double timeInTicks);

		/**
		 * Projects each vertex of the mesh to a sphere.
		 * @param horizontalRadius horizontal sphere radius.
		 * @param verticalRadius vertical sphere radius.
		 * @param timeInTicks number of ticks for this morph.
		 */
		void addSphereAttributes(
				float horizontalRadius,
				float verticalRadius,
				double timeInTicks,
				const Vec3f &offset = Vec3f(0.0f, 0.0f, 0.0f));

		/**
		 * Projects each vertex of the mesh to a box.
		 * @param width box width.
		 * @param height box height.
		 * @param depth box depth.
		 * @param timeInTicks number of ticks for this morph.
		 */
		void addBoxAttributes(
				float width,
				float height,
				float depth,
				double timeInTicks,
				const Vec3f &offset = Vec3f(0.0f, 0.0f, 0.0f));

		// override
		void gpuUpdate(RenderState *rs, double dt) override;

	protected:
		struct KeyFrame {
			std::list<InputLocation> attributes;
			double timeInTicks;
			double startTick;
			double endTick;
			ref_ptr<SSBO> buffer;
		};

		ShaderInput1f *frameTimeUniform_;
		ShaderInput1f *frictionUniform_;
		ShaderInput1f *frequencyUniform_;
		ref_ptr<State> meshAnimState_;
		ref_ptr<State> interpolationState_;

		ref_ptr<Mesh> mesh_;

		int32_t lastFrameBindingPoint_ = -1;
		int32_t nextFrameBindingPoint_ = -1;

		int lastFrame_, nextFrame_;
		uint32_t bufferSize_;

		ref_ptr<SSBO> pingBuffer_;
		ref_ptr<SSBO> pongBuffer_;
		int pingFrame_, pongFrame_;
		ref_ptr<BufferReference> pingIt_;
		ref_ptr<BufferReference> pongIt_;
		std::vector<KeyFrame> frames_;
		std::set<std::string> animAttributes_;

		// milliseconds from start of animation
		double elapsedTime_;
		double ticksPerSecond_;
		double lastTime_;
		Vec2d tickRange_;
		uint32_t lastFramePosition_;
		uint32_t startFramePosition_;

		uint32_t mapOffset_, mapSize_;

		void loadFrame(uint32_t frameIndex, bool isPongFrame);

		ref_ptr<ShaderInput> findLastAttribute(const std::string &name);

		static void findFrameAfterTick(
				double tick, int &frame, std::vector<KeyFrame> &keys);

		static void findFrameBeforeTick(
				double &tick, uint32_t &frame, std::vector<KeyFrame> &keys);
	};
} // namespace

#endif /* MESH_ANIMATION_GPU_H_ */

#ifndef REGEN_BLANKET_TRAIL_H_
#define REGEN_BLANKET_TRAIL_H_

#include <regen/objects/terrain/ground.h>
#include <regen/objects/primitives/blanket.h>

namespace regen {
	/**
	 * \brief A trail of blankets.
	 *
	 * This class is used to create a trail of blankets.
	 * The blankets are created at a specified frequency and are used to create effects
	 * such as dust trails, footprints, or other effects that require a series of
	 * quads following a path. Note you still need to insert the blankets along the path
	 * manually, e.g., based on the movement of a NPC.
	 *
	 * The trail mesh mimics the ground mesh, i.e., it uses the same shader except
	 * that it inserts additional shader defines to alter the rendering.
	 */
	class BlanketTrail : public Blanket {
	public:
		struct TrailConfig : public Blanket::BlanketConfig {
			// the number of blanket trails to create
			uint32_t numTrails = 1;
			// the frequency of blankets along trail. This is an upper bound
			// used to create the number of instances. Actual frequency
			// depends on the speed of the trail.
			// A value of 0.1 means a blanket can at max created every 0.1 seconds.
			float blanketFrequency = 0.1f;
		};

		/**
		 * Constructor.
		 * @param groundMesh the ground mesh to mimic.
		 * @param cfg the trail configuration.
		 */
		BlanketTrail(
			const ref_ptr<Ground> &groundMesh,
			const TrailConfig &cfg,
			const SystemTime &systemTime);

		/**
		 * Assigns a blanket mask texture.
		 * @param tex the blanket mask texture. This should be a 2D array
		 *        texture with one layer per mask channel.
		 */
		void setBlanketMask(const ref_ptr<Texture> &tex);

		/**
		 * Assigns a model transformation to this ground.
		 * @param tf the model transformation to assign.
		 */
		void setModelTransform(const ref_ptr<ModelTransformation> &tf);

		/**
		 * @return the model transformation assigned to this ground.
		 */
		const ref_ptr<ModelTransformation> &tf() const { return tf_; }

		/**
		 * Inserts a blanket at the specified position and direction.
		 * @param pos the position to insert the blanket.
		 * @param dir the direction of the blanket (used to rotate it).
		 * @param maskIndex the index into the blanket mask texture to use
		 *        for this blanket. This is used e.g. to create footprints
		 *        for left and right foot using a mask array texture.
		 */
		void insertBlanket(const Vec3f &pos, const Vec3f &dir, uint32_t maskIndex=0);

		static ref_ptr<BlanketTrail> load(LoadingContext &ctx,
			scene::SceneInputNode &input,
			const std::vector<uint32_t> &lodLevels);

	protected:
		ref_ptr<Ground> groundMesh_;
		ref_ptr<ModelTransformation> tf_;
		float insertHeight_ = 0.0f;
		Mat4f tmpMat_;

		ref_ptr<Texture> blanketMask_;
		ref_ptr<TextureState> blanketMaskState_;
		// Per-instance index into the blanket mask texture array.
		// Only used in case we have a mask texture with depth>1.
		ref_ptr<ShaderInput1ui> u_maskIndex_;
		std::vector<uint32_t> maskIndex_;
	};
} // namespace

#endif /* REGEN_BLANKET_TRAIL_H_ */

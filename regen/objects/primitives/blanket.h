#ifndef REGEN_BLANKET_H_
#define REGEN_BLANKET_H_

#include <regen/objects/primitives/rectangle.h>

namespace regen {
	/**
	 * \brief A blanket mesh.
	 *
	 * This class is used to create a blanket mesh.
	 * The blankets are created as rectangles with a specified size and texture coordinates.
	 * The blankets can be used to create effects such as dust, footprints, or other effects
	 * that require quads.
	 *
	 * The blankets are initially dead, i.e., they are not rendered.
	 * They can be revived by calling reviveBlanket(), which returns the index of the
	 * revived blanket instance. The lifetime of the blanket is then reset and it will
	 * be rendered until its lifetime expires.
	 */
	class Blanket : public Rectangle {
	public:
		/**
		 * Configuration for a blanket mesh.
		 */
		struct BlanketConfig {
			// if true, all blankets are initially dead
			bool isInitiallyDead = false;
			// size of each blanket
			Vec2f blanketSize = Vec2f(1.0f, 1.0f);
			// texture coordinate scaling
			Vec2f texcoScale = Vec2f(1.0f, 1.0f);
			// zero means infinite
			float blanketLifetime = 0.0f;
			// tessellation LOD levels
			std::vector<uint32_t> levelOfDetails = { 1 };
			// buffer update configuration
			ClientAccessMode accessMode = BUFFER_CPU_WRITE;
			BufferUpdateFlags updateHint = BufferUpdateFlags::NEVER;
			BufferMapMode mapMode = BUFFER_MAP_DISABLED;
		};

		/**
		 * Constructor.
		 * @param cfg the blanket configuration.
		 * @param numInstances the number of blanket instances.
		 */
		Blanket(const BlanketConfig &cfg,
			uint32_t numInstances,
			const SystemTime &systemTime);

		/**
		 * Revive a dead blanket instance, returning its index
		 * and resetting its lifetime.
		 * @return index of the revived blanket instance index.
		 **/
		uint32_t reviveBlanket();

		/**
		 * Update the lifetime of all blankets.
		 * @param deltaSeconds the time in seconds since the last update.
		 */
		void updateLifetime(float deltaSeconds);

		// override
		void updateAttributes() override;

	protected:
		uint32_t numBlankets_;
		uint32_t numDeadBlankets_ = 0;
		float blanketLifetimeMax_;
		std::vector<uint32_t> deadBlankets_;
		std::vector<float> blanketLifetime_;
		uint32_t blanketTraversalMask_;
		ref_ptr<Animation> lifetimeAnimation_;
		const SystemTime *systemTime_ = nullptr;

		ref_ptr<BufferBlock> blanketBuffer_;
		ref_ptr<ShaderInput1f> timeOfBirth_;
	};
} // namespace

#endif /* REGEN_BLANKET_H_ */

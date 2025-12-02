#ifndef REGEN_PERSON_CONTROLLER_H_
#define REGEN_PERSON_CONTROLLER_H_

#include <regen/behavior/npc-controller.h>
#include "world/world-model.h"
#include "behavior-tree.h"
#include "perception/perception-system.h"
#include "regen/objects/model-transformation.h"
#include "regen/objects/terrain/blanket-trail.h"
#include "skeleton/bone-controller.h"

namespace regen {
	class PersonController : public NonPlayerCharacterController {
	public:
		static std::vector<ref_ptr<PersonController>> load(LoadingContext &ctx, scene::SceneInputNode &node);

		PersonController(
			const ref_ptr<Mesh> &mesh,
			const Indexed<ref_ptr<ModelTransformation>> &tfIndexed,
			const ref_ptr<BoneAnimationItem> &animItem,
			const ref_ptr<WorldModel> &world);

		~PersonController() override = default;

		/**
		 * @param mesh A mesh that is only seen when the NPC is holding a weapon.
		 */
		void setWeaponMesh(const ref_ptr<Mesh> &mesh);

		/**
		 * Set the footstep trail for visualizing footsteps.
		 * @param trail the footstep trail.
		 */
		void setFootstepTrail(
					const ref_ptr<BlanketTrail> &trail,
					float leftFootTime, float rightFootTime) {
			footstepTrail_ = trail;
			footTime_[0] = leftFootTime;
			footTime_[1] = rightFootTime;
		}

		// override
		void initializeController() override;

		// override
		void updateController(double dt) override;

	protected:
		ref_ptr<Mesh> weaponMesh_;
		ref_ptr<BoundingShape> weaponShape_;
		bool hasDrawnWeapon_ = false;
		uint32_t weaponMask_ = 0;

		ref_ptr<BlanketTrail> footstepTrail_;
		uint32_t footIdx_[2] = { 0, 1 };
		bool footDown_[2] = { false, false };
		float footTime_[2] = { 0.25f, 0.8f };
		float footLastElapsed_ = 0.0f;

		void hideWeapon();

		void drawWeapon();
	};
} // namespace

#endif /* REGEN_PERSON_CONTROLLER_H_ */

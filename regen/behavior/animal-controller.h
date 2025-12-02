#ifndef REGEN_ANIMAL_CONTROLLER_H_
#define REGEN_ANIMAL_CONTROLLER_H_

#include <regen/compute/vector.h>
#include <regen/textures/texture.h>
#include <regen/behavior/npc-controller.h>
#include "regen/shapes/bounds.h"
#include "regen/objects/model-transformation.h"

namespace regen {
	class AnimalController : public NonPlayerCharacterController {
	public:
		static std::vector<ref_ptr<AnimalController>> load(LoadingContext &ctx, scene::SceneInputNode &node);

		AnimalController(
				const ref_ptr<Mesh> &mesh,
				const Indexed<ref_ptr<ModelTransformation> > &tfIndexed,
				const ref_ptr<BoneAnimationItem> &animItem,
				const ref_ptr<WorldModel> &world);

		~AnimalController() override = default;

		/**
		 * Set the territory bounds.
		 * @param center the center.
		 * @param size the size.
		 */
		void setTerritoryBounds(const Vec2f &center, const Vec2f &size);

		// override
		void updateController(double dt) override;

	protected:
		Bounds<Vec2f> territoryBounds_;
		ref_ptr<Place> animalTerritory_;

	};
} // namespace

#endif /* REGEN_ANIMAL_CONTROLLER_H_ */

/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef REGEN_CHARACTER_CONTROLLER_H
#define REGEN_CHARACTER_CONTROLLER_H

#include <BulletDynamics/Character/btKinematicCharacterController.h>

namespace regen {
	class CharacterController : public btKinematicCharacterController {
	public:
		CharacterController(
				btPairCachingGhostObject *ghostObject,
				btConvexShape *convexShape,
				btScalar stepHeight)
				: btKinematicCharacterController(ghostObject, convexShape, stepHeight) {}
	};
}

#endif //REGEN_CHARACTER_CONTROLLER_H

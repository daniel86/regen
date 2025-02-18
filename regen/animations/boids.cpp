#include "boids.h"

using namespace regen;

// TODO: Make the neighborhood computation faster by using a spatial index.
//       Currently the computation is O(n^2), which is not very efficient for large number of boids.
//       An approach could be a grid-based indexed with fixed size cells, then boid positions can easily
//       be mapped to the grid cell they belong to. Then the neighborhood search can be limited to
//       through adjacent cells plus knowing their size plus the visual range of the boids.
//       Maybe an option would be a C array with std::vector elements?
// TODO: Implement GPU version of the boids simulation.

BoidsSimulation_CPU::BoidsSimulation_CPU(const ref_ptr<ModelTransformation> &tf)
		: Animation(false, true),
		  tf_(tf),
		  bounds_(-10.0f, 10.0f) {
	setAnimationName("boids");
	// use a dedicated thread for the boids simulation which is not synchronized with the graphics thread,
	// i.e. it can be slower or faster than the graphics thread.
	setSynchronized(false);

	coherenceWeight_ = ref_ptr<ShaderInput1f>::alloc("coherenceWeight");
	coherenceWeight_->setUniformData(1.0f);
	animationState()->joinShaderInput(coherenceWeight_);

	alignmentWeight_ = ref_ptr<ShaderInput1f>::alloc("alignmentWeight");
	alignmentWeight_->setUniformData(1.0f);
	animationState()->joinShaderInput(alignmentWeight_);

	separationWeight_ = ref_ptr<ShaderInput1f>::alloc("separationWeight");
	separationWeight_->setUniformData(1.0f);
	animationState()->joinShaderInput(separationWeight_);

	avoidanceWeight_ = ref_ptr<ShaderInput1f>::alloc("avoidanceWeight");
	avoidanceWeight_->setUniformData(1.0f);
	animationState()->joinShaderInput(avoidanceWeight_);

	avoidanceDistance_ = ref_ptr<ShaderInput1f>::alloc("avoidanceDistance");
	avoidanceDistance_->setUniformData(0.1f);
	animationState()->joinShaderInput(avoidanceDistance_);

	visualRange_ = ref_ptr<ShaderInput1f>::alloc("visualRange");
	visualRange_->setUniformData(1.0f);
	animationState()->joinShaderInput(visualRange_);

	lookAheadDistance_ = ref_ptr<ShaderInput1f>::alloc("lookAheadDistance");
	lookAheadDistance_->setUniformData(0.1f);
	animationState()->joinShaderInput(lookAheadDistance_);

	repulsionFactor_ = ref_ptr<ShaderInput1f>::alloc("repulsionFactor");
	repulsionFactor_->setUniformData(20.0f);
	animationState()->joinShaderInput(repulsionFactor_);

	// compute the maximum number of neighbors, use 100 neighbors as default maximum.
	// if below, try to use half of the number of boids.
	auto maxNumNeighbors = tf_->get()->numInstances();
	if (maxNumNeighbors > 10) { maxNumNeighbors /= 2; }
	maxNumNeighbors = std::max(50u, maxNumNeighbors);
	maxNumNeighbors_ = ref_ptr<ShaderInput1ui>::alloc("maxNumNeighbors");
	maxNumNeighbors_->setUniformData(maxNumNeighbors);
	animationState()->joinShaderInput(maxNumNeighbors_);

	maxBoidSpeed_ = ref_ptr<ShaderInput1f>::alloc("maxBoidSpeed");
	maxBoidSpeed_->setUniformData(1.0f);
	animationState()->joinShaderInput(maxBoidSpeed_);

	maxAngularSpeed_ = ref_ptr<ShaderInput1f>::alloc("maxAngularSpeed");
	maxAngularSpeed_->setUniformData(0.05f);
	animationState()->joinShaderInput(maxAngularSpeed_);

	auto tfInput = tf_->get();
	auto tfData = tfInput->mapClientData<Mat4f>(ShaderData::READ);
	boidsScale_ = tfData.r[0].scaling();
	// allocate boids data
	boidData_.resize(tfInput->numInstances());
	for (GLuint i = 0; i < tfInput->numInstances(); ++i) {
		auto &d = boidData_[i];
		d.position = tfData.r[i].position();
		d.velocity = Vec3f::zero();
	}
}

void BoidsSimulation_CPU::setBounds(const Bounds<Vec3f> &bounds) {
	bounds_ = bounds;
}

void BoidsSimulation_CPU::setMap(
				const Vec3f &mapCenter,
				const Vec2f &mapSize,
				const ref_ptr<Texture2D> &heightMap,
				float heightMapFactor) {
	mapCenter_ = mapCenter;
	mapSize_ = mapSize;
	heightMap_ = heightMap;
	heightMapFactor_ = heightMapFactor;
}

Vec2f BoidsSimulation_CPU::computeUV(const Vec3f &boidPosition, const Vec3f &mapCenter, const Vec2f &mapSize) {
	Vec2f boidCoord(
			boidPosition.x - mapCenter.x,
			boidPosition.z - mapCenter.z);
	return boidCoord / mapSize + Vec2f(0.5f);
}

void BoidsSimulation_CPU::animate(double dt) {
	auto dt_f = static_cast<float>(dt) * 0.001f;
	// advance boids simulation
	simulateBoids(dt_f);
	// update boids model transformation using the boids data
	{
		auto &tfInput = tf_->get();
		auto tfData = tfInput->mapClientData<Mat4f>(ShaderData::READ | ShaderData::WRITE);
		for (GLuint i = 0; i < tfInput->numInstances(); ++i) {
			auto &d = boidData_[i];
			// calculate boid matrix, also need to compute rotation from velocity, model z
			//       should point in the direction of velocity.
			if (d.velocity.length() > 0.001f) {
				// Convert the normalized direction vector to Euler angles
				boidRotation_.setEuler(
						atan2(-d.direction.x, d.direction.z) + baseOrientation_,
						asin(-d.direction.y),
						0.0f);
				tfData.w[i] = boidRotation_.calculateMatrix();
				tfData.w[i].scale(boidsScale_);
				tfData.w[i].translate(d.position);
			} else {
				tfData.w[i] = tfData.r[i];
			}
		}
	}
	// recompute neighborhood relationships of all boids
	auto maxDistance = visualRange_->getVertex(0);
	auto maxNeighbours = maxNumNeighbors_->getVertex(0);
	for (size_t i = 0; i < boidData_.size(); i++) {
		auto &b1 = boidData_[i];
		unsigned int numNeighbors = 0;
		for (size_t j = i + 1; j < boidData_.size(); j++) {
			auto &b2 = boidData_[j];
			auto distance = (b1.position - b2.position).length();
			if (distance < maxDistance.r) {
				b1.neighbors.push_back(&b2);
				b2.neighbors.push_back(&b1);
				numNeighbors++;
				if (numNeighbors >= maxNeighbours.r) {
					break;
				}
			}
		}
	}
}

void BoidsSimulation_CPU::simulateBoids(float dt) {
	for (auto &d: boidData_) {
		simulateBoid(d, dt);
		d.neighbors.clear();
	}
}

void BoidsSimulation_CPU::simulateBoid(BoidData &boid, float dt) {
	boid.force = Vec3f::zero();
	// a boid is lost if it is outside the bounds
	bool isBoidLost = !bounds_.contains(boid.position);
	if (boid.neighbors.empty()) {
		// a boid without neighbors is lost
		isBoidLost = true;
	} else {
		// simulate the boid using the three rules of boids
		avgPosition_ = Vec3f::zero();
		avgVelocity_ = Vec3f::zero();
		separation_ = Vec3f::zero();
		for (auto neighbor: boid.neighbors) {
			avgPosition_ += neighbor->position;
			avgVelocity_ += neighbor->velocity;
			boidDirection_ = boid.position - neighbor->position;
			float distance = boidDirection_.length();
			if (distance < 0.001f) {
				boidDirection_ = Vec3f::random();
				// avoid "jumping" up
				//boidDirection_.y = -abs(boidDirection_.y);
				boidDirection_.normalize();
				distance = 0.0f;
			} else {
				boidDirection_ /= distance * distance;
			}
			if (distance < avoidanceDistance_->getVertex(0).r) {
				separation_ += boidDirection_;
			}
		}
		avgPosition_ /= static_cast<float>(boid.neighbors.size());
		avgVelocity_ /= static_cast<float>(boid.neighbors.size());
		boid.force +=
				separation_ * separationWeight_->getVertex(0).r +
				(avgVelocity_ - boid.velocity) * alignmentWeight_->getVertex(0).r +
				(avgPosition_ - boid.position) * coherenceWeight_->getVertex(0).r;
	}

	// put some restrictions on the boid's velocity.
	// a boid that cannot avoid collisions is considered lost.
	isBoidLost = !avoidCollisions(boid, dt) || isBoidLost;
	auto isInDanger = !dangers_.empty() && !avoidDanger(boid);
	if (!isInDanger && !attractors_.empty()) {
		// note: ignore attractors if in danger
		attract(boid);
	}
	// drift towards home if lost
	if (isBoidLost) { homesickness(boid); }

	boid.position += boid.velocity * dt;
	boid.velocity += boid.force * dt;
	limitVelocity(boid);

	boid.direction = boid.velocity;
	boid.direction.normalize();
}

void BoidsSimulation_CPU::limitVelocity(BoidData &boid) {
	// limit translation speed
	auto maxSpeed = maxBoidSpeed_->getVertex(0).r;
	if (boid.velocity.length() > maxSpeed) {
		boid.velocity.normalize();
		boid.velocity *= maxSpeed;
	}

	// limit angular speed
	auto maxAngularSpeed = maxAngularSpeed_->getVertex(0).r;
	boidDirection_ = boid.velocity;
	boidDirection_.normalize();
	auto angle = acos(boidDirection_.dot(boid.direction));
	if (angle > maxAngularSpeed) {
		auto axis = boidDirection_.cross(boid.direction);
		axis.normalize();
		boidRotation_.setAxisAngle(axis, maxAngularSpeed);
		auto newDirection = boidRotation_.rotate(boid.direction);
		boid.velocity = newDirection * boid.velocity.length();
	}
}

void BoidsSimulation_CPU::homesickness(BoidData &boid) {
	// a boid seems to have lost track, and wants to go home!
	// first find closest home point...
	const Vec3f *closestHomePoint = &Vec3f::zero();
	float minDistance;
	if (!homePoints_.empty()) {
		minDistance = std::numeric_limits<float>::max();
		for (auto &home: homePoints_) {
			auto distance = (boid.position - home).length();
			if (distance < minDistance) {
				minDistance = distance;
				closestHomePoint = &home;
			}
		}
	} else {
		minDistance = (boid.position - *closestHomePoint).length();
	}
	// second steer towards the closest home point ...
	boidDirection_ = *closestHomePoint - boid.position;
	if (minDistance < 0.001f) {
		boidDirection_ = Vec3f::random();
	} else {
		boidDirection_ /= minDistance;
	}
	auto repulsionFactor = repulsionFactor_->getVertex(0).r * separationWeight_->getVertex(0).r;
	boid.force += boidDirection_ * repulsionFactor * 0.1;
}

bool BoidsSimulation_CPU::avoidDanger(BoidData &boid) {
	auto maxDistance = visualRange_->getVertex(0).r;
	bool isInDanger = false;
	for (auto &danger: dangers_) {
		auto dangerDirection = danger.get()->getVertex(0).r.position() - boid.position;
		auto distance = dangerDirection.length();
		if (distance < maxDistance) {
			if (distance < 0.001f) {
				dangerDirection = Vec3f::random();
			} else {
				dangerDirection /= distance;
			}
			auto repulsionFactor = repulsionFactor_->getVertex(0).r * separationWeight_->getVertex(0).r;
			boid.force += dangerDirection * repulsionFactor;
			isInDanger = true;
		}
	}
	return !isInDanger;
}

void BoidsSimulation_CPU::attract(BoidData &boid) {
	auto maxDistance = visualRange_->getVertex(0).r;
	for (auto &attractor: attractors_) {
		auto attractorDirection =
			attractor.get()->getVertex(0).r.position() - boid.position;
		auto distance = attractorDirection.length();
		if (distance < maxDistance && distance > 0.001f) {
			auto attractionFactor = repulsionFactor_->getVertex(0).r * separationWeight_->getVertex(0).r;
			boid.force += attractorDirection * attractionFactor / distance;
		}
	}
}

bool BoidsSimulation_CPU::avoidCollisions(BoidData &boid, float dt) {
	auto repulsionFactor = repulsionFactor_->getVertex(0).r * separationWeight_->getVertex(0).r;
	auto avoidDistance = avoidanceDistance_->getVertex(0).r;
	auto avoidDistanceHalf = avoidDistance * 0.5f;
	auto nextVelocity = boid.velocity + boid.force * dt;
	nextVelocity.normalize();
	auto lookAhead = boid.position + nextVelocity * lookAheadDistance_->getVertex(0).r;
	bool isCollisionFree = true;

	////////////////
	/////// Collision with boundaries.
	////////////////
	if (lookAhead.y < bounds_.min.y + avoidDistance) {
		boid.force.y += repulsionFactor;
	}
	if (lookAhead.y > bounds_.max.y - avoidDistance) {
		boid.force.y -= repulsionFactor;
	}
	if (lookAhead.x < bounds_.min.x + avoidDistance) {
		boid.force.x += repulsionFactor;
	}
	if (lookAhead.x > bounds_.max.x - avoidDistance) {
		boid.force.x -= repulsionFactor;
	}
	if (lookAhead.z < bounds_.min.z + avoidDistance) {
		boid.force.z += repulsionFactor;
	}
	if (lookAhead.z > bounds_.max.z - avoidDistance) {
		boid.force.z -= repulsionFactor;
	}

	////////////////
	/////// Collision with height map.
	////////////////
	if (heightMap_.get()) {
		// boid position in height map space [0, 1]
		auto boidCoord = computeUV(boid.position, mapCenter_, mapSize_);
		auto currentY = heightMap_->sampleLinear<float>(boidCoord, heightMap_->textureData());
		currentY *= heightMapFactor_;
		currentY += mapCenter_.y + avoidDistanceHalf;
		// sample the height map at the projected boid position
		boidCoord = computeUV(lookAhead, mapCenter_, mapSize_);
		auto nextY = heightMap_->sampleLinear<float>(boidCoord, heightMap_->textureData());
		nextY *= heightMapFactor_;
		nextY += mapCenter_.y + avoidDistanceHalf;

		if (boid.position.y < currentY) {
			// The boid is below the minimum height of the height map -> push boid up.
			if (currentY < bounds_.max.y - avoidDistanceHalf) {
				// only push up in case the height map y is below the maximum y of boids
				boid.force.y += repulsionFactor;
			}
			isCollisionFree = false;
		}
		if (lookAhead.y < nextY) {
			// Assuming the boid follows the lookAhead direction, it will eventually reach a point where
			// it is below the surface of the height map -> push boid up.
			if (nextY < bounds_.max.y - avoidDistanceHalf) {
				// only push up in case the height map y is below the maximum y of boids
				boid.force.y += repulsionFactor;
			}
		}
		if (nextY > bounds_.max.y - avoidDistanceHalf) {
			// Assuming the boid follows the lookAhead direction, it will eventually reach a point where
			// it is above the max y of the boid bounds.
			// We could sample normal map here, and compute reflection vector. But this might be a bit
			// overkill for huge number of boids.
			// An easy approach is to push the boid to where it came from:
			boid.force.x -= nextVelocity.x * repulsionFactor;
			boid.force.z -= nextVelocity.z * repulsionFactor;
		 }
	}

	return isCollisionFree;
}

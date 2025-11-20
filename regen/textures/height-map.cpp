#include "height-map.h"

using namespace regen;

HeightMap::HeightMap() : Texture2D() {
}

HeightMap::HeightMap(const Texture2D &other) : Texture2D(other) {
}

void HeightMap::setMapCenter(const Vec2f &center) {
	mapCenter_ = center;
	mapBounds_.min = mapCenter_ - mapSize_ * 0.5f;
	mapBounds_.max = mapCenter_ + mapSize_ * 0.5f;
}

void HeightMap::setMapSize(const Vec2f &size) {
	mapSize_ = size;
	mapBounds_.min = mapCenter_ - mapSize_ * 0.5f;
	mapBounds_.max = mapCenter_ + mapSize_ * 0.5f;
}

void HeightMap::setMapFactor(float factor) {
	mapFactor_ = factor;
}

float HeightMap::sampleHeight(const Vec2f &pos) {
	// compute UV for height map sampling
	auto uv = (pos - mapCenter_) / mapSize_ + Vec2f::create(0.5f);
	auto mapValue = sampleLinear<float,1>(uv, textureData());
	mapValue *= mapFactor_;
	// increase by small bias to avoid intersection with the floor
	mapValue += 0.02f;
	return mapValue - mapFactor_ * 0.5f;
}

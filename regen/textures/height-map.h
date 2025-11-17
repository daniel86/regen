#ifndef REGEN_HEIGHT_MAP_H_
#define REGEN_HEIGHT_MAP_H_

#include <regen/textures/texture.h>

namespace regen {
	/**
	 * \brief A height map texture.
	 *
	 * The height map is a 2D texture that encodes height values
	 * in the red channel. The height map can be sampled
	 * at a given 2D position to obtain the height value.
	 *
	 * The height map has a center position and a size in world units.
	 * The height values are scaled by a factor to obtain
	 * the final height in world units.
	 */
	class HeightMap : public Texture2D {
	public:
		HeightMap();

		/**
		 * Copy constructor.
		 */
		explicit HeightMap(const Texture2D &other);

		/**
		 * @return the center position of the height map in world units.
		 */
		const Vec2f& mapCenter() const { return mapCenter_; }

		/**
		 * @return the size of the height map in world units.
		 */
		const Vec2f& mapSize() const { return mapSize_; }

		/**
		 * @return the bounds of the height map in world units.
		 */
		const Bounds<Vec2f>& mapBounds() const { return mapBounds_; }

		/**
		 * @return the height scaling factor.
		 */
		float mapFactor() const { return mapFactor_; }

		/**
		 * Set the center position of the height map in world units.
		 * @param center the center position.
		 */
		void setMapCenter(const Vec2f &center);

		/**
		 * Set the size of the height map in world units.
		 * @param size the size.
		 */
		void setMapSize(const Vec2f &size);

		/**
		 * Set the height scaling factor.
		 * @param factor the scaling factor.
		 */
		void setMapFactor(float factor);

		/**
		 * Sample the height at the given 2D position.
		 * @param pos the 2D position in world units.
		 * @return the height value in world units.
		 */
		float sampleHeight(const Vec2f &pos);

	protected:
		Vec2f mapCenter_ = Vec2f(0.0f);
		Vec2f mapSize_ = Vec2f(10.0f);
		float mapFactor_ = 8.0f;
		Bounds<Vec2f> mapBounds_ = Bounds<Vec2f>::create(Vec2f(-1.0f), Vec2f(1.0f));
	};
} // namespace

#endif /* REGEN_HEIGHT_MAP_H_ */

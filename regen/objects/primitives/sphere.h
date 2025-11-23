/*
 * sphere.h
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#ifndef SPHERE_H_
#define SPHERE_H_

#include "regen/shader/shader-state.h"
#include <regen/objects/mesh.h>
#include <regen/math/vector.h>

namespace regen {
	/**
	 * \brief Round geometrical object in three-dimensional space - a sphere ;)
	 *
	 * The sphere is centered at (0,0,0).
	 * A LoD factor can be used to configure sphere tesselation.
	 * @note take a look at SphereSprite for an alternative.
	 */
	class Sphere : public Mesh {
	public:
		/**
		 * Configures texture coordinates.
		 */
		enum TexcoMode {
			/** do not generate texture coordinates */
			TEXCO_MODE_NONE,
			/** generate 2D uv coordinates */
			TEXCO_MODE_UV
		};

		/**
		 * Vertex data configuration.
		 */
		struct Config {
			/** scaling for the position attribute. */
			Vec3f posScale;
			/** scaling vector for TEXCO_MODE_UV */
			Vec2f texcoScale;
			/** number of surface divisions */
			std::vector<uint32_t> levelOfDetails;
			/** texture coordinate mode */
			TexcoMode texcoMode;
			/** generate normal attribute */
			bool isNormalRequired;
			/** generate tangent attribute */
			bool isTangentRequired;
			/** If true only bottom half sphere is used. */
			bool isHalfSphere;
			/** Buffer usage hints. */
			ClientAccessMode accessMode = BUFFER_CPU_WRITE;
			BufferUpdateFlags updateHint = BufferUpdateFlags::NEVER;
			BufferMapMode mapMode = BUFFER_MAP_DISABLED;

			Config();
		};

		/**
		 * @param cfg the mesh configuration.
		 */
		explicit Sphere(const Config &cfg = Config());

		/**
		 * @return the radius of the sphere.
		 */
		auto radius() const { return radius_; }

		/**
		 * Updates vertex data based on given configuration.
		 * @param cfg vertex data configuration.
		 */
		void updateAttributes(const Config &cfg = Config());

	protected:
		ref_ptr<ShaderInput3f> pos_;
		ref_ptr<ShaderInput3f> nor_;
		ref_ptr<ShaderInput2f> texco_;
		ref_ptr<ShaderInput4f> tan_;
		ref_ptr<ShaderInput> indices_;
		float radius_;

		void generateLODLevel(const Config &cfg,
				uint32_t lodLevel,
				uint32_t vertexOffset,
				uint32_t indexOffset);
	};

	std::ostream &operator<<(std::ostream &out, const Sphere::TexcoMode &mode);

	std::istream &operator>>(std::istream &in, Sphere::TexcoMode &mode);

	/**
	 * \brief A sprite that generates sphere normals.
	 *
	 * It is not possible to define per vertex attributes
	 * for the sphere because each sphere is extruded from a single point
	 * in space. You can only add per sphere attributes to this mesh.
	 * This is a nice way to handle spheres because you get a perfectly round shape
	 * on the render target you use and you can anti-alias edges in the fragment shader
	 * easily.
	 */
	class SphereSprite : public Mesh, public HasShader {
	public:
		/**
		 * Vertex data configuration.
		 */
		struct Config {
			/** one radius for each sphere. */
			float *radius;
			/** one position for each sphere. */
			Vec3f *position;
			/** number of spheres. */
			uint32_t sphereCount;
			/** Buffer usage hints. */
			ClientAccessMode accessMode = BUFFER_CPU_WRITE;
			BufferUpdateFlags updateHint = BufferUpdateFlags::NEVER;
			BufferMapMode mapMode = BUFFER_MAP_DISABLED;

			Config();
		};

		/**
		 * @param cfg the mesh configuration.
		 */
		explicit SphereSprite(const Config &cfg);

		/**
		 * Updates vertex data based on given configuration.
		 * @param cfg vertex data configuration.
		 */
		void updateAttributes(const Config &cfg);
	};
} // namespace

#endif /* SPHERE_H_ */

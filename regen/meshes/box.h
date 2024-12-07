/*
 * box.h
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#ifndef REGEN_BOX_H__
#define REGEN_BOX_H__

#include <regen/meshes/mesh-state.h>
#include <regen/math/vector.h>
#include <regen/meshes/tessellation.h>

namespace regen {
	/**
	 * \brief Three-dimensional solid object bounded by six square faces,
	 * facets or sides, with three meeting at each vertex - a cube ;)
	 *
	 * The cube is centered at (0,0,0).
	 */
	class Box : public Mesh {
	public:
		/**
		 * A box with each side having a length of 2.
		 * No tangents, normals and texture coordinates are generated for this cube.
		 * @return the static unit cube (in range [-1,1]).
		 */
		static ref_ptr<Box> getUnitCube();

		/**
		 * Configures texture coordinates.
		 */
		enum TexcoMode {
			TEXCO_MODE_NONE,   //!< do not generate texture coordinates
			TEXCO_MODE_UV,     //!< generate 2D uv coordinates
			TEXCO_MODE_CUBE_MAP//!< generate 3D coordinates for cube mapping
		};

		/**
		 * Vertex data configuration.
		 */
		struct Config {
			/** number of surface divisions. */
			std::vector<GLuint> levelOfDetails;
			/** scaling for the position attribute. */
			Vec3f posScale;
			/** cube xyz rotation. */
			Vec3f rotation;
			/** scaling vector for TEXCO_MODE_UV. */
			Vec2f texcoScale;
			/** texture coordinate mode. */
			TexcoMode texcoMode;
			/** generate normal attribute ?. */
			GLboolean isNormalRequired;
			/** generate tangent attribute ?. */
			GLboolean isTangentRequired;
			/** VBO usage hint. */
			VBO::Usage usage;

			Config();
		};

		/**
		 * @param cfg the mesh configuration.
		 */
		explicit Box(const Config &cfg = Config());

		/**
		 * @param other Another Box.
		 */
		explicit Box(const ref_ptr<Box> &other);

		/**
		 * Updates vertex data based on given configuration.
		 * @param cfg vertex data configuration.
		 */
		void updateAttributes(const Config &cfg = Config());

	protected:
		ref_ptr<ShaderInput1ui> indices_;
		ref_ptr<ShaderInput3f> pos_;
		ref_ptr<ShaderInput3f> nor_;
		ref_ptr<ShaderInput4f> tan_;
		ref_ptr<ShaderInput> texco_;
		TexcoMode texcoMode_;
		Mat4f modelRotation_;

		void generateLODLevel(
				const Config &cfg,
				GLuint sideIndex,
				GLuint lodLevel,
				const std::vector<Tessellation> &tessellations);
	};

	std::ostream &operator<<(std::ostream &out, const Box::TexcoMode &mode);

	std::istream &operator>>(std::istream &in, Box::TexcoMode &mode);
} // namespace

#endif /* REGEN_BOX_H__ */

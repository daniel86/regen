#ifndef REGEN_Torus_H__
#define REGEN_Torus_H__

#include <regen/meshes/mesh-state.h>
#include <regen/math/vector.h>

namespace regen {
	/**
	 * \brief Three-dimensional solid object bounded by six square faces,
	 * facets or sides, with three meeting at each vertex - a cube ;)
	 *
	 * The cube is centered at (0,0,0).
	 */
	class Torus : public Mesh {
	public:
		/**
		 * A box with each side having a length of 2.
		 * No tangents, normals and texture coordinates are generated for this cube.
		 * @return the static unit cube (in range [-1,1]).
		 */
		static ref_ptr<Torus> getUnitTorus();

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
			GLuint levelOfDetail;
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
			/** radius of the torus ring. */
			GLfloat ringRadius;
			/** radius of the tube. */
			GLfloat tubeRadius;

			Config();
		};

		/**
		 * @param cfg the mesh configuration.
		 */
		explicit Torus(const Config &cfg = Config());

		/**
		 * @param other Another Box.
		 */
		explicit Torus(const ref_ptr<Torus> &other);

		/**
		 * Updates vertex data based on given configuration.
		 * @param cfg vertex data configuration.
		 */
		void updateAttributes(const Config &cfg = Config());

	protected:
		ref_ptr<ShaderInput3f> pos_;
		ref_ptr<ShaderInput3f> nor_;
		ref_ptr<ShaderInput4f> tan_;
		ref_ptr<ShaderInput> texco_;
		ref_ptr<ShaderInput1ui> indices_;
	};

	std::ostream &operator<<(std::ostream &out, const Torus::TexcoMode &mode);

	std::istream &operator>>(std::istream &in, Torus::TexcoMode &mode);
} // namespace

#endif /* REGEN_BOX_H__ */

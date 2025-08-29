#ifndef REGEN_Disc_H__
#define REGEN_Disc_H__

#include <regen/meshes/mesh-state.h>
#include <regen/math/vector.h>

namespace regen {
	/**
	 * \brief A disc is a 2D geometric shape that is defined by a center and a radius.
	 * The disc is centered at (0,0,0).
	 */
	class Disc : public Mesh {
	public:
		/**
		 * A disc with a radius of 1.
		 * No tangents, normals and texture coordinates are generated for this disc.
		 * @return the static unit disc.
		 */
		static ref_ptr<Disc> getUnitDisc();

		/**
		 * Configures texture coordinates.
		 */
		enum TexcoMode {
			TEXCO_MODE_NONE,   //!< do not generate texture coordinates
			TEXCO_MODE_UV      //!< generate 2D uv coordinates
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
			/** radius of the disc. */
			GLfloat discRadius;
			/** Buffer usage hints. */
			ClientAccessMode accessMode = BUFFER_CPU_WRITE;
			BufferUpdateFlags updateHint = BufferUpdateFlags::NEVER;
			BufferMapMode mapMode = BUFFER_MAP_DISABLED;

			Config();
		};

		/**
		 * @param cfg the mesh configuration.
		 */
		explicit Disc(const Config &cfg = Config());

		/**
		 * @param other Another Box.
		 */
		explicit Disc(const ref_ptr<Disc> &other);

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
		ref_ptr<ShaderInput> indices_;

		void generateLODLevel(const Config &cfg,
				GLuint lodLevel,
				GLuint vertexOffset,
				GLuint indexOffset);
	};

	std::ostream &operator<<(std::ostream &out, const Disc::TexcoMode &mode);

	std::istream &operator>>(std::istream &in, Disc::TexcoMode &mode);
} // namespace

#endif /* REGEN_Disc_H__ */

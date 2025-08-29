#ifndef REGEN_RECTANGLE_H_
#define REGEN_RECTANGLE_H_

#include <regen/meshes/mesh-state.h>
#include "../lod/tessellation.h"

namespace regen {
	/**
	 * \brief A polygon with four edges, vertices and right angles.
	 */
	class Rectangle : public Mesh {
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
			/** number of surface divisions. */
			std::vector<GLuint> levelOfDetails;
			/** scaling for the position attribute. */
			Vec3f posScale;
			/** cube xyz rotation. */
			Vec3f rotation;
			/** cube xyz translation. */
			Vec3f translation;
			/** scaling vector for TEXCO_MODE_UV. */
			Vec2f texcoScale;
			/** generate normal attribute */
			GLboolean isNormalRequired;
			/** generate texture coordinates */
			GLboolean isTexcoRequired;
			/** generate tangent attribute */
			GLboolean isTangentRequired;
			/** flag indicating if the quad center should be translated to origin. */
			GLboolean centerAtOrigin;
			/** Buffer usage hints. */
			ClientAccessMode accessMode = BUFFER_CPU_WRITE;
			BufferUpdateFlags updateHint = BufferUpdateFlags::NEVER;
			BufferMapMode mapMode = BUFFER_MAP_DISABLED;

			Config();
		};

		/**
		 * The unit quad has side length 2.0 and is parallel to the xy plane
		 * with z=0. No normals,tangents and texture coordinates are generated.
		 * @return the unit quad.
		 */
		static ref_ptr<Rectangle> getUnitQuad();

		/**
		 * Creates a rectangle mesh.
		 * @param cfg the mesh configuration.
		 * @return the rectangle mesh.
		 */
		[[nodiscard]]
		static ref_ptr<Rectangle> create(const Config &cfg) {
			auto rect = ref_ptr<Rectangle>::alloc(cfg);
			rect->updateAttributes();
			return rect;
		}

		/**
		 * @param cfg the mesh configuration.
		 */
		explicit Rectangle(const Config &cfg = Config());

		/**
		 * @param other Another Rectangle.
		 */
		explicit Rectangle(const ref_ptr<Rectangle> &other);

		/**
		 * Updates vertex data based on given configuration.
		 * @param cfg vertex data configuration.
		 */
		virtual void updateAttributes();

		const ref_ptr<ShaderInput3f>& pos() const { return pos_; }

	protected:
		Config rectangleConfig_;
		ref_ptr<ShaderInput3f> pos_;
		ref_ptr<ShaderInput3f> nor_;
		ref_ptr<ShaderInput4f> tan_;
		ref_ptr<ShaderInput2f> texco_;
		ref_ptr<ShaderInput> indices_;

		virtual void generateLODLevel(const Config &cfg,
				const Tessellation &tessellation,
				const Mat4f &rotMat,
				GLuint vertexOffset,
				GLuint indexOffset,
				GLuint lodLevel);

		virtual void tessellateRectangle(uint32_t lod, Tessellation &t);
	};
} // namespace

#endif /* REGEN_RECTANGLE_H_ */

#ifndef REGEN_GROUND_PATH_H__
#define REGEN_GROUND_PATH_H__

#include <regen/objects/mesh.h>
#include <regen/compute/vector.h>
#include "regen/compute/bezier.h"

namespace regen {
	class GroundPath : public Mesh {
	public:
		/**
		 * Vertex data configuration.
		 */
		struct Config {
			/** splines defining the path. */
			std::vector<math::Bezier<Vec2f>> splines;
			/** number of samples per LOD level. */
			std::vector<uint32_t> levelOfDetails;
			/** scaling for the position attribute. */
			Vec3f posScale;
			/** cube xyz rotation. */
			Vec3f rotation;
			/** scaling vector for TEXCO_MODE_UV. */
			Vec2f texcoScale;
			/** base height of the path. */
			float baseHeight = 0.0f;
			/** width of the path. */
			float pathWidth = 4.0f;
			/** generate texture coordinates ?. */
			bool isTexCoordRequired;
			/** generate normal attribute ?. */
			bool isNormalRequired;
			/** generate tangent attribute ?. */
			bool isTangentRequired;
			/** radius of the disc. */
			float discRadius;
			/** Buffer usage hints. */
			ClientAccessMode accessMode = BUFFER_CPU_WRITE;
			BufferUpdateFlags updateHint = BufferUpdateFlags::NEVER;
			BufferMapMode mapMode = BUFFER_MAP_DISABLED;

			Config();
		};

		/**
		 * Loads a PathMesh from a scene input node.
		 * @param ctx loading context.
		 * @param input scene input node.
		 * @return the loaded PathMesh.
		 */
		static ref_ptr<GroundPath> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * @param cfg the mesh configuration.
		 */
		explicit GroundPath(const Config &cfg = Config());

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
				uint32_t lodLevel,
				uint32_t vertexOffset,
				uint32_t indexOffset);
	};
} // namespace

#endif /* REGEN_PATH_MESH_H__ */

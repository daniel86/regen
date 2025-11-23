#ifndef REGEN_FrameMesh_H__
#define REGEN_FrameMesh_H__

#include <regen/objects/mesh.h>
#include <regen/math/vector.h>

namespace regen {
	/**
	 * \brief Three-dimensional solid box-shape frame with a whole in the middle.
	 *
	 * The frame is centered at (0,0,0).
	 * The border width of the frame determines the thickness of the frame.
	 */
	class FrameMesh : public Mesh {
	public:
		/**
		 * @return the static unit frame (in range [-1,1]).
		 */
		static ref_ptr<FrameMesh> getUnitFrame();

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
			uint32_t levelOfDetail;
			/** scaling for the position attribute. */
			Vec3f posScale;
			/** cube xyz rotation. */
			Vec3f rotation;
			/** scaling vector for TEXCO_MODE_UV. */
			Vec2f texcoScale;
			/** texture coordinate mode. */
			TexcoMode texcoMode;
			/** generate normal attribute ?. */
			bool isNormalRequired;
			/** generate tangent attribute ?. */
			bool isTangentRequired;
			/** size of the frame border. */
			float borderSize;
			/** Buffer usage hints. */
			ClientAccessMode accessMode = BUFFER_CPU_WRITE;
			BufferUpdateFlags updateHint = BufferUpdateFlags::NEVER;
			BufferMapMode mapMode = BUFFER_MAP_DISABLED;

			Config();
		};

		/**
		 * @param cfg the mesh configuration.
		 */
		explicit FrameMesh(const Config &cfg = Config());

		/**
		 * @param other Another Box.
		 */
		explicit FrameMesh(const ref_ptr<FrameMesh> &other);

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

	std::ostream &operator<<(std::ostream &out, const FrameMesh::TexcoMode &mode);

	std::istream &operator>>(std::istream &in, FrameMesh::TexcoMode &mode);
} // namespace

#endif /* REGEN_FrameMesh_H__ */

/*
 * cone.h
 *
 *  Created on: 03.02.2013
 *      Author: daniel
 */

#ifndef REGEN_MESH_CONE_H_
#define REGEN_MESH_CONE_H_

#include <regen/objects/mesh.h>
#include <regen/math/vector.h>

namespace regen {
	class Cone : public Mesh {
	public:
		Cone(GLenum primitive, const BufferUpdateFlags &hints);

	protected:
		ref_ptr<ShaderInput3f> nor_;
		ref_ptr<ShaderInput3f> pos_;
	};
	/**
	 * \brief A cone is an n-dimensional geometric shape that tapers smoothly from a base
	 * to a point called the apex.
	 *
	 * The base has a circle shape.
	 * OpenedCone does not handle the base geometry, it is defined using GL_TRIANGLE_FAN.
	 */
	class ConeOpened : public Cone {
	public:
		/**
		 * Vertex data configuration.
		 */
		struct Config {
			/** cosine of cone angle */
			float cosAngle;
			/** distance from apex to base */
			float height;
			/** generate normal attribute ? */
			bool isNormalRequired;
			/** subdivisions = 4*levelOfDetail^2 */
			std::vector<uint32_t> levelOfDetails;
			/** Buffer usage hints. */
			ClientAccessMode accessMode = BUFFER_CPU_WRITE;
			BufferUpdateFlags updateHint = BufferUpdateFlags::NEVER;
			BufferMapMode mapMode = BUFFER_MAP_DISABLED;

			Config();
		};

		/**
		 * @param cfg the mesh configuration.
		 */
		explicit ConeOpened(const Config &cfg = Config());

		/**
		 * Updates vertex data based on given configuration.
		 * @param cfg vertex data configuration.
		 */
		void updateAttributes(const Config &cfg = Config());

	protected:
		void generateLODLevel(const Config &cfg,
				uint32_t lodLevel,
				uint32_t vertexOffset,
				uint32_t indexOffset);
	};

	/**
	 * \brief A cone is an n-dimensional geometric shape that tapers smoothly from a base
	 * to a point called the apex.
	 *
	 * The base has a circle shape.
	 * ClosedCone does handle the base geometry, it is defined using GL_TRIANGLES.
	 */
	class ConeClosed : public Cone {
	public:
		/**
		 * The 'base' cone has apex=(0,0,0) and opens
		 * in positive z direction. The base radius is 0.5 and the apex base
		 * distance is 1.0.
		 * @return the base cone.
		 */
		static ref_ptr<Mesh> getBaseCone();

		/**
		 * Vertex data configuration.
		 */
		struct Config {
			/** the base radius */
			float radius;
			/** the base apex distance */
			float height;
			/** generate cone normals */
			bool isNormalRequired;
			/** generate cone base geometry */
			bool isBaseRequired;
			/** level of detail for base circle */
			std::vector<uint32_t> levelOfDetails;
			/** Buffer usage hints. */
			ClientAccessMode accessMode = BUFFER_CPU_WRITE;
			BufferUpdateFlags updateHint = BufferUpdateFlags::NEVER;
			BufferMapMode mapMode = BUFFER_MAP_DISABLED;

			Config();
		};

		/**
		 * @param cfg the mesh configuration.
		 */
		explicit ConeClosed(const Config &cfg = Config());

		/**
		 * Updates vertex data based on given configuration.
		 * @param cfg vertex data configuration.
		 */
		void updateAttributes(const Config &cfg = Config());

	protected:
		ref_ptr<ShaderInput> indices_;

		void generateLODLevel(const Config &cfg,
				uint32_t lodLevel,
				uint32_t vertexOffset,
				uint32_t indexOffset);
	};
} // namespace

#endif /* REGEN_MESH_CONE_H_ */

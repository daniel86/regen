#ifndef REGEN_GL_ENUM_H_
#define REGEN_GL_ENUM_H_

#include <GL/glew.h>

#include <string>
#include <regen/utility/logging.h>

namespace regen {
	namespace glenum {
		/**
		 * Specifies the depth comparison function.
		 * @param val input string.
		 * @return GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL,
		 *         GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, or GL_ALWAYS.
		 */
		GLenum compareFunction(const std::string &val);

		/**
		 * Specifies the stencil comparison function.
		 * @param val input string.
		 * @return GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL,
		 *         GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, or GL_ALWAYS.
		 */
		GLenum stencilOp(const std::string &val_);

		/**
		 * Specifies the texture comparison mode for currently bound depth textures.
		 * That is, a texture whose internal format is GL_DEPTH_COMPONENT_*
		 */
		GLenum compareMode(const std::string &val_);

		/**
		 * Specifies how source and destination colors are combined.
		 * It must be GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX.
		 * Initially, both the RGB blend equation and the alpha blend equation are set to GL_FUNC_ADD.
		 */
		GLenum blendFunction(const std::string &val_);

		/**
		 * Specifies the buffers whose contents are to be synchronized.
		 * @param val input string.
		 * @return one of the GL_*_BARRIER_BIT constants.
		 */
		GLenum barrierBit(const std::string &val);

		/**
		 * The cull face specifies whether front- or back-facing facets are candidates for culling.
		 * @param val input string.
		 * @return GL_FRONT, GL_BACK, or GL_FRONT_AND_BACK.
		 */
		GLenum cullFace(const std::string &val);

		/**
		 * Specifies the orientation of front-facing polygons.
		 * The default value is "CCW".
		 * @param val "CW" and "CCW" are accepted.
		 * @return GL_CW or GL_CCW.
		 */
		GLenum frontFace(const std::string &val);

		/**
		 * Fill mode selects a polygon rasterization mode.
		 * @param val input string.
		 * @return GL_FILL, L_LINE or GL_POINT.
		 */
		GLenum fillMode(const std::string &val);

		/**
		 * Specifies the screen draw buffer.
		 * @param val input string.
		 * @return GL_FRONT, GL_BACK or GL_FRONT_AND_BACK.
		 */
		GLenum drawBuffer(const std::string &val_);

		/**
		 * Primitives are ways that OpenGL interprets vertex streams,
		 * converting them from vertices into triangles, lines, points, and so forth.
		 * @param val input string.
		 * @return GL_PATCHES, GL_POINTS, GL_LINES, GL_LINE_LOOP,
		 *          GL_LINE_STRIP, GL_LINES_ADJACENCY, GL_LINE_STRIP_ADJACENCY,
		 *          GL_TRIANGLES, GL_TRIANGLE_FAN, GL_TRIANGLE_STRIP,
		 *          GL_TRIANGLES_ADJACENCY or GL_TRIANGLE_STRIP_ADJACENCY
		 */
		GLenum primitive(const std::string &val);

		/**
		 * The filter mode is used whenever the level-of-detail function
		 * used when sampling from the texture determines that the texture should
		 * be minified or magnified.
		 * @param val input string.
		 * @return GL_NEAREST, GL_LINEAR,
		 *         GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR,
		 *         GL_LINEAR_MIPMAP_NEAREST or GL_LINEAR_MIPMAP_LINEAR.
		 */
		GLenum filterMode(const std::string &val);

		/**
		 * The wrap parameter for texture coordinates.
		 * @param val input string.
		 * @return GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE,
		 *         GL_MIRRORED_REPEAT or GL_REPEAT.
		 */
		GLenum wrappingMode(const std::string &val);

		/**
		 * Defines base type of texel data.
		 * @param val the type string.
		 * @return GL_HALF_FLOAT, GL_FLOAT, GL_UNSIGNED_BYTE
		 *         GL_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_INT,
		 *         GL_UNSIGNED_INT or GL_DOUBLE.
		 */
		GLenum pixelType(const std::string &val);

		/**
		 * Defines the number of components per texel.
		 * @param val the type string.
		 * @return 1, 2, 3 or 4.
		 */
		inline uint32_t pixelComponents(GLenum format) {
			switch (format) {
				case GL_RED:
				case GL_GREEN:
				case GL_BLUE:
				case GL_ALPHA:
				case GL_LUMINANCE:
				case GL_DEPTH_COMPONENT:
				case GL_STENCIL_INDEX:
				case GL_DEPTH_STENCIL:
					return 1;
				case GL_RG:
				case GL_LUMINANCE_ALPHA:
					return 2;
				case GL_RGB:
				case GL_BGR:
					return 3;
				case GL_RGBA:
				case GL_BGRA:
					return 4;
				default:
					REGEN_WARN("Unknown pixel format 0x" << std::hex << format << std::dec
							   << ". Using default 4 components.");
					return 4;
			}
		}

		/**
		 * Defines the format of texel data that is visible to the outside.
		 * @param numComponents Number of components per texel.
		 * @return On of the GL_R,GL_RG,GL_RGB,GL_RGBA constants.
		 */
		inline GLenum textureFormat(uint32_t numComponents) {
			switch (numComponents) {
				case 1:
					return GL_RED;
				case 2:
					return GL_RG;
				case 3:
					return GL_RGB;
				case 4:
					return GL_RGBA;
				default:
					REGEN_WARN("Unknown number of components " << numComponents
							   << ". Using default GL_RGBA.");
					return GL_RGBA;
			}
		}

		/**
		 * Defines the format of texel data that is visible to the outside.
		 * @param val the type string.
		 * @return One of the GL_R,GL_RG,GL_RGB,GL_RGBA constants.
		 */
		GLenum textureFormat(const std::string &val);

		/**
		 * Defines the texture type.
		 * @param val the texture target string.
		 * @return One of the GL_TEXTURE_1D,GL_TEXTURE_1D_ARRAY,GL_TEXTURE_2D,
		 *         GL_TEXTURE_2D_ARRAY,GL_TEXTURE_2D_MULTISAMPLE,GL_TEXTURE_3D,
		 *         GL_TEXTURE_CUBE_MAP,GL_TEXTURE_DEPTH,GL_TEXTURE_SHADOW constants.
		 */
		GLenum textureTarget(const std::string &val);

		/**
		 * Sets the swizzle that will be applied to the rgba components of a texel before it is returned to the shader.
		 * Valid values for param are GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA, GL_ZERO and GL_ONE.
		 */
		GLenum textureSwizzle(const std::string &val_);

		/**
		 * Defines the format of texel data that is used texture intern.
		 * @param pixelType the texture pixel type.
		 * @param numComponents number of components per texels.
		 * @param bytesPerComponent bytes per component.
		 * @return On of the GL_R,GL_RG,GL_RGB,GL_RGBA constants.
		 */
		inline GLenum textureInternalFormat(GLenum pixelType, uint32_t numComponents, uint32_t bytesPerComponent) {
			uint32_t i = 1;
			if (bytesPerComponent <= 8) i = 0;
			else if (bytesPerComponent <= 16) i = 1;
			else if (bytesPerComponent <= 32) i = 2;

			uint32_t j = numComponents - 1;

			if (pixelType == GL_FLOAT || pixelType == GL_DOUBLE || pixelType == GL_HALF_FLOAT) {
				static GLenum values[3][4] = {
						{GL_NONE, GL_NONE,  GL_NONE,   GL_NONE},
						{GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F},
						{GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F}
				};
				return values[i][j];
			} else if (pixelType == GL_UNSIGNED_INT) {
				static GLenum values[3][4] = {
						{GL_R8UI,  GL_RG8UI,  GL_RGB8UI,  GL_RGBA8UI},
						{GL_R16UI, GL_RG16UI, GL_RGB16UI, GL_RGBA16UI},
						{GL_R32UI, GL_RG32UI, GL_RGB32UI, GL_RGBA32UI}
				};
				return values[i][j];
			} else if (pixelType == GL_INT) {
				static GLenum values[3][4] = {
						{GL_R8I,  GL_RG8I,  GL_RGB8I,  GL_RGBA8I},
						{GL_R16I, GL_RG16I, GL_RGB16I, GL_RGBA16I},
						{GL_R32I, GL_RG32I, GL_RGB32I, GL_RGBA32I}
				};
				return values[i][j];
			} else {
				static GLenum values[3][4] = {
						{GL_R8,   GL_RG8,  GL_RGB8,  GL_RGBA8},
						{GL_R16,  GL_RG16, GL_RGB16, GL_RGBA16},
						{GL_NONE, GL_NONE, GL_NONE,  GL_NONE}
				};
				return values[i][j];
			}
			return GL_RGBA;
		}

		/**
		 * Defines the format of texel data that is used texture intern.
		 * @param textureFormat the texture pixel type.
		 * @return On of the GL_R,GL_RG,GL_RGB,GL_RGBA constants.
		 */
		inline GLenum textureInternalFormat(GLenum textureFormat) {
			switch (textureFormat) {
				case GL_RED:
					return GL_R8;
				case GL_RG:
					return GL_RG8;
				case GL_RGB:
					return GL_RGB8;
				case GL_RGBA:
					return GL_RGBA8;
				default:
					return textureFormat;
			}
		}

		/**
		 * Defines the format of texel data that is used texture intern.
		 * @param val the type string.
		 * @return On of the GL_R,GL_RG,GL_RGB,GL_RGBA constants.
		 */
		GLenum textureInternalFormat(const std::string &val);

		/**
		 * Maps [0,5] to cube map layer enum.
		 */
		inline GLenum cubeMapLayer(uint32_t layer) {
			const GLenum cubeMapLayer[] = {
					GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
					GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
					GL_TEXTURE_CUBE_MAP_POSITIVE_X,
					GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
					GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
					GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
			};
			return cubeMapLayer[layer];
		}

		/**
		 * Array of known shader stages Enumerations.
		 */
		const GLenum *glslStages();

		/**
		 * Number of known shader stages.
		 */
		int glslStageCount();

		/**
		 * Maps stage enum to name representation.
		 */
		std::string glslStageName(GLenum stage);

		/**
		 * Maps stage enum to prefix for input variables in GLSL code.
		 */
		std::string glslStagePrefix(GLenum stage);

		/**
		 * Maps pixel type and values per element to the GLSL data type.
		 */
		std::string glslDataType(GLenum pixelType, uint32_t valsPerElement);

		/**
		 * @param baseType the base type of the data, e.g. GL_FLOAT
		 * @param valsPerElement the number of values per element, e.g. 4
		 * @return the data type for the shader, e.g. GL_RGBA32F
		 */
		GLenum dataType(GLenum baseType, uint32_t valsPerElement);

		/**
		 * True if the data type is a signed integer type.
		 */
		inline bool isSignedIntegerType(GLenum dataType) {
			return dataType == GL_BYTE || dataType == GL_SHORT || dataType == GL_INT ||
				   dataType == GL_R8I || dataType == GL_RG8I || dataType == GL_RGB8I ||
				   dataType == GL_RGBA8I || dataType == GL_R16I || dataType == GL_RG16I ||
				   dataType == GL_RGB16I || dataType == GL_RGBA16I || dataType == GL_R32I ||
				   dataType == GL_RG32I || dataType == GL_RGB32I || dataType == GL_RGBA32I;
		}

		/**
		 * True if the data type is a unsigned integer type.
		 */
		inline bool isUnsignedIntegerType(GLenum dataType) {
			return dataType == GL_UNSIGNED_BYTE || dataType == GL_UNSIGNED_SHORT ||
				   dataType == GL_UNSIGNED_INT || dataType == GL_R8UI || dataType == GL_RG8UI ||
				   dataType == GL_RGB8UI || dataType == GL_RGBA8UI || dataType == GL_R16UI ||
				   dataType == GL_RG16UI || dataType == GL_RGB16UI || dataType == GL_RGBA16UI ||
				   dataType == GL_R32UI || dataType == GL_RG32UI || dataType == GL_RGB32UI ||
				   dataType == GL_RGBA32UI;
		}
	}
} // namespace

#endif /* REGEN_GL_ENUM_H_ */

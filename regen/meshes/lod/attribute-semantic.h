#ifndef REGEN_ATTRIBUTE_SEMANTIC_H
#define REGEN_ATTRIBUTE_SEMANTIC_H

namespace regen {
	/**
	 * \brief Attribute semantic enum.
	 *
	 * This enum is used to identify the semantic of an attribute in a mesh.
	 */
	enum class AttributeSemantic {
		POSITION,
		NORMAL,
		TANGENT,
		BITANGENT,
		TEXCOORD,
		COLOR,
		UNKNOWN // keep last
	};
}

#endif //REGEN_ATTRIBUTE_SEMANTIC_H

#ifndef REGEN_LOD_ATTRIBUTE_H
#define REGEN_LOD_ATTRIBUTE_H

#include <vector>
#include <regen/gl-types/shader-input.h>
#include "attribute-semantic.h"

namespace regen {
	/**
	 * \brief Base class for LOD attributes.
	 *
	 * This class represents a base class for LOD attributes in a mesh.
	 * It contains the semantic and the shader input for the attribute.
	 */
	class LODAttribute {
	public:
		virtual ~LODAttribute() = default;

		LODAttribute(const LODAttribute &other) = delete;

		void operator=(const LODAttribute &other) = delete;

		AttributeSemantic semantic;
		ref_ptr<ShaderInput> attribute;

		virtual void setVertex(uint32_t newIdx, const LODAttribute *other, uint32_t otherIndex) = 0;

		virtual LODAttribute *makeSibling(uint32_t numVertices) const = 0;

		virtual byte *clientData() = 0;

		virtual const byte *clientData() const = 0;

		virtual uint32_t numVertices() const = 0;

		virtual uint32_t inputSize() const = 0;

	protected:
		LODAttribute(AttributeSemantic semantic, const ref_ptr<ShaderInput> &attr)
				: semantic(semantic), attribute(attr) {
		}
	};

	/**
	 * \brief Templated class for LOD attributes.
	 *
	 * This class represents a templated class for LOD attributes in a mesh.
	 * It contains the semantic, the shader input for the attribute, and the data.
	 */
	template<typename T>
	class LODAttributeT : public LODAttribute {
	public:
		LODAttributeT(AttributeSemantic semantic, const ref_ptr<ShaderInput> &attr)
				: LODAttribute(semantic, attr),
				  data(attr->numVertices()) {
			std::memcpy(
					(byte *) data.data(),
					attr->clientData(),
					data.size() * attribute->elementSize());
		}

		LODAttributeT(AttributeSemantic semantic, const ref_ptr<ShaderInput> &attr, uint32_t numVertices)
				: LODAttribute(semantic, attr),
				  data(numVertices) {
		}

		explicit LODAttributeT(const LODAttribute *other)
				: LODAttribute(other->semantic, other->attribute),
				  data(other->numVertices()) {
			std::memcpy(
					(byte*)this->data.data(),
					other->clientData(),
					data.size() * attribute->elementSize());
		}

		~LODAttributeT() override = default;

		std::vector <T> data;

		LODAttribute *makeSibling(uint32_t numVertices) const override {
			return new LODAttributeT<T>(semantic, attribute, numVertices);
		}

		void setVertex(uint32_t newIdx, const LODAttribute *other, uint32_t otherIndex) override {
			auto &otherData = ((const LODAttributeT*) other)->data;
			data[newIdx] = otherData[otherIndex];
		}

		byte *clientData() override { return (byte *) data.data(); }

		const byte *clientData() const override { return (const byte *) data.data(); }

		uint32_t numVertices() const override { return data.size(); }

		uint32_t inputSize() const override { return data.size() * attribute->elementSize(); }
	};
}

#endif //REGEN_LOD_ATTRIBUTE_H

#ifndef REGEN_INPUT_SCHEMA_H_
#define REGEN_INPUT_SCHEMA_H_

#include <optional>
#include <istream>
#include <ostream>

namespace regen {
	/**
	 * \brief Describes the semantics of an input, and what values are valid.
	 */
	class InputSchema {
	public:
		enum Semantics {
			UNKNOWN=0,
			VECTOR,
			SCALAR,
			COLOR,
			ALPHA,
			POSITION,
			DIRECTION,
			SCALE,
			TRANSFORM,
			TEXTURE
		};

		explicit InputSchema(Semantics semantics);

		Semantics semantics() const { return semantics_; }

		std::optional<std::pair<float, float>> limits(int index) const;

		void setLimits(int index, float min, float max);

		static const InputSchema *getDefault(Semantics semantics);

		static const InputSchema *unknown();

		static const InputSchema *color();

		static const InputSchema *alpha();

		static const InputSchema *position();

		static const InputSchema *direction();

		static const InputSchema *scale();

		static const InputSchema *transform();

		static const InputSchema *texture();

		static const InputSchema *scalar(float min, float max);

		static InputSchema *alloc(Semantics semantics);

	private:
		Semantics semantics_;
		std::map<int, std::pair<float, float>> limits_;
	};

	std::ostream &operator<<(std::ostream &out, const InputSchema::Semantics &semantics);

	std::istream &operator>>(std::istream &in, InputSchema::Semantics &semantics);
}

#endif /* REGEN_INPUT_SCHEMA_H_ */

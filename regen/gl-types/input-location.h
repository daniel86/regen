#ifndef REGEN_INPUT_LOCATION_H_
#define REGEN_INPUT_LOCATION_H_

#include "regen/shader/shader-input.h"

namespace regen {
	/**
	 * \brief Maps input to shader location.
	 */
	struct InputLocation {
		ref_ptr<ShaderInput> input; /**< the shader input. */
		int location; /**< the input location. */
		uint32_t uploadStamp; /**< time stamp of last upload. */
		/**
		 * @param _input input data.
		 * @param _location input location in shader.
		 */
		InputLocation(const ref_ptr<ShaderInput> &_input, int _location)
				: input(_input), location(_location), uploadStamp(0) {}

		InputLocation()
				: location(-1), uploadStamp(0) {}
	};
} // namespace

#endif /* REGEN_INPUT_LOCATION_H_ */

#ifndef REGEN_GL_PARAM_H_
#define REGEN_GL_PARAM_H_

#include <GL/glew.h>
#include <map>

#include <regen/math/vector.h>

namespace regen {
	// introduce a template function to get the value of a GL parameter
    template<typename T> T glParam(GLenum param);

    /**
     * A simple parameter store. One singleton per datatype T.
     * @tparam T
     */
	template<typename T> class GLParameterStore {
	public:
		static GLParameterStore<T> &instance() {
			static GLParameterStore<T> instance;
			return instance;
		}
		std::map<GLenum, T> params_;
	};
} // namespace

#endif /* REGEN_GL_PARAM_H_ */

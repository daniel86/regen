#include "gl-param.h"

namespace regen {
	template<> bool glParam<bool>(GLenum param) {
		auto &store = GLParameterStore<bool>::instance();
		auto it = store.params_.find(param);
		if (it != store.params_.end()) {
			return it->second;
		} else {
			GLboolean value;
			glGetBooleanv(param, &value);
			store.params_[param] = value;
			return value;
		}
	}

    template<> float glParam<float>(GLenum param) {
		auto &store = GLParameterStore<float>::instance();
		auto it = store.params_.find(param);
		if (it != store.params_.end()) {
			return it->second;
		} else {
			float value;
			glGetFloatv(param, &value);
			store.params_[param] = value;
			return value;
		}
	}

    template<> double glParam<double>(GLenum param) {
		auto &store = GLParameterStore<double>::instance();
		auto it = store.params_.find(param);
		if (it != store.params_.end()) {
			return it->second;
		} else {
			double value;
			glGetDoublev(param, &value);
			store.params_[param] = value;
			return value;
		}
	}

	template<> int glParam<int>(GLenum param) {
		auto &store = GLParameterStore<int>::instance();
		auto it = store.params_.find(param);
		if (it != store.params_.end()) {
			return it->second;
		} else {
			int value;
			glGetIntegerv(param, &value);
			store.params_[param] = value;
			return value;
		}
	}

	template<> unsigned int glParam<unsigned int>(GLenum param) {
		auto &store = GLParameterStore<unsigned int>::instance();
		auto it = store.params_.find(param);
		if (it != store.params_.end()) {
			return it->second;
		} else {
			uint32_t value;
			glGetIntegerv(param, (int *) &value);
			store.params_[param] = value;
			return value;
		}
	}

	template<> Vec3f glParam<Vec3f>(GLenum param) {
		auto &store = GLParameterStore<Vec3f>::instance();
		auto it = store.params_.find(param);
		if (it != store.params_.end()) {
			return it->second;
		} else {
			Vec3f value;
			glGetFloati_v(param, 0, &value.x);
			glGetFloati_v(param, 1, &value.y);
			glGetFloati_v(param, 2, &value.z);
			store.params_[param] = value;
			return value;
		}
	}

	template<> Vec3i glParam<Vec3i>(GLenum param) {
		Vec3i value;
		glGetIntegeri_v(param, 0, &value.x);
		glGetIntegeri_v(param, 1, &value.y);
		glGetIntegeri_v(param, 2, &value.z);
		return value;
	}
} // namespace

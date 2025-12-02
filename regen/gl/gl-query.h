#ifndef REGEN_GL_QUERY_H_
#define REGEN_GL_QUERY_H_

#include <regen/gl/gl-object.h>

namespace regen {
	/**
	 * \brief Base class for OpenGL queries.
	 *
	 * This class provides a base for OpenGL queries.
	 * It is used to create and manage OpenGL queries.
	 *
	 * \tparam T The type of the query result.
	 */
	template<typename T>
	class GLQuery : public GLObject {
	public:
		/**
		 * Constructor.
		 *
		 * @param target The target of the query.
		 */
		explicit GLQuery(GLenum target) :
				GLObject(glGenQueries, glDeleteQueries, 1),
				target_(target) {}

		~GLQuery() override = default;

		GLQuery(const GLQuery &other) = delete;

		/**
		 * Begin the query.
		 */
		void begin() const {
			glBeginQuery(target_, id());
		}

		/**
		 * End the query.
		 *
		 * @return The result of the query.
		 */
		T end() const {
			glEndQuery(target_);
			return readQueryResult();
		}

	protected:
		GLenum target_;

		virtual T readQueryResult() const = 0;
	};
} // namespace

#endif /* REGEN_GL_QUERY_H_ */

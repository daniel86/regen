#ifndef REGEN_FILESYSTEM_H_
#define REGEN_FILESYSTEM_H_

#include <string>
#include <list>

namespace regen {
	/**
	 * @return the user base directory.
	 */
	std::string userDirectory();

	/**
	 * \brief A choice of multiple paths.
	 */
	struct PathChoice {
		/**
		 * The list of paths.
		 */
		std::list<std::string> choices_;

		/**
		 * @return first path in the choices list that exists or an empty string if none exists.
		 */
		std::string firstValidPath();
	};

	/**
	 * Build a filesystem path.
	 * @param baseDirectory the base directory. It's prepended as is.
	 * @param pathString the path string. Directory names are separated by one of the specified separators.
	 * @param separators string that contains separator characters.
	 * @return the filesystem path.
	 */
	std::string filesystemPath(
			const std::string &baseDirectory,
			const std::string &pathString,
			const std::string &separators = "/");

	/**
	 * Get the absolute path of a resource.
	 * @param relPath the relative path.
	 * @return the absolute path.
	 */
	std::string resourcePath(const std::string &relPath);
} // namespace

#endif /* REGEN_FILESYSTEM_H_ */

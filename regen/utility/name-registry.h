#ifndef NAME_REGISTRY_H_
#define NAME_REGISTRY_H_

#include <string>
#include <string_view>
#include <unordered_set>
#include <mutex>

namespace regen {
	/**
	 * \brief Global registry for shader input names and other frequently used strings.
	 * 
	 * This provides a global atom table for string names, allowing us to use
	 * std::string_view safely by ensuring the underlying strings have stable lifetimes.
	 */
	class NameRegistry {
	public:
		/**
		 * Get the singleton instance.
		 */
		static NameRegistry& instance();

		/**
		 * Register a name and return a string_view to the stored copy.
		 * The returned string_view is guaranteed to remain valid for the
		 * lifetime of the program.
		 */
		std::string_view registerName(std::string_view name);

		/**
		 * Register a name and return a string_view to the stored copy.
		 * Convenience overload for std::string.
		 */
		std::string_view registerName(const std::string& name);

	private:
		NameRegistry() = default;
		~NameRegistry() = default;
		NameRegistry(const NameRegistry&) = delete;
		NameRegistry& operator=(const NameRegistry&) = delete;

		std::mutex mutex_;
		std::unordered_set<std::string> names_;
	};
}

#endif /* NAME_REGISTRY_H_ */
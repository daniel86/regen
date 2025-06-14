#ifndef REGEN_BINDING_MANAGER_H_
#define REGEN_BINDING_MANAGER_H_

#include <set>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

namespace regen {
	/**
	 * \brief A class that manages the binding points of buffer objects.
	 *
	 * This class is used to avoid binding point conflicts between different
	 * buffer objects. It keeps track of the maximum number of bindings for
	 * each type of buffer object and assigns binding points to them.
	 */
	class BindingManager {
	public:
		/**
		 * The type of buffer object.
		 */
		enum BlockType {
			UBO = 0,
			SSBO,
			LAST_BLOCK_TYPE
		};

		/**
		 * Clear all binding points and reset the manager.
		 */
		static void clear();

		/**
		 * Request a binding point for a buffer object.
		 * @param blockType The type of buffer object.
		 * @param bufferID The ID of the buffer object.
		 * @param blockName The name of the buffer object.
		 * @param avoidBindingPoints A set of binding points to avoid.
		 * @return The assigned binding point.
		 */
		[[nodiscard]]
		static int32_t request(
				BlockType blockType,
				std::uintptr_t bufferID,
				const std::string &blockName,
				const std::set<int32_t> &avoidBindingPoints);

	protected:
		BindingManager();

		virtual ~BindingManager() = default;

		int32_t maxBindings_[LAST_BLOCK_TYPE];
		int32_t nextBindingPoint_[LAST_BLOCK_TYPE];
		std::unordered_map<std::uintptr_t, int32_t> bufferBindings_;
		std::unordered_map<std::string, int32_t> namedBindings_;
		// counts how often a binding point was used before
		std::vector<int32_t> bindingPointCounter_[LAST_BLOCK_TYPE];

		static BindingManager &instance() {
			static BindingManager instance;
			return instance;
		}

		int32_t request_(
				BlockType blockType,
				const std::set<int32_t> &avoidBindingPoints,
				const std::string &blockName,
				std::uintptr_t bufferID);
	};
} // namespace

#endif /* REGEN_BOUNDING_BOX_BUFFER_H_ */

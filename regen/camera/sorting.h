#ifndef REGEN_SORTING_H
#define REGEN_SORTING_H

namespace regen {
	typedef enum {
		FRONT_TO_BACK,
		BACK_TO_FRONT,
		NO_SORTING
	} SortMode;

	std::ostream &operator<<(std::ostream &out, const SortMode &mode);

	std::istream &operator>>(std::istream &in, SortMode &mode);
}

#endif //REGEN_SORTING_H

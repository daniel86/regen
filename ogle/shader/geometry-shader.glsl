-- defines
#if GS_INPUT_PRIMITIVE == points
#define GS_NUM_VERTICES 1
#elif GS_INPUT_PRIMITIVE == lines || GS_INPUT_PRIMITIVE == lines_adjacency
#define GS_NUM_VERTICES 2
#else
#define GS_NUM_VERTICES 3
#endif


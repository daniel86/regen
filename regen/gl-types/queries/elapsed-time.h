#ifndef REGEN_ELAPSED_TIME_H_
#define REGEN_ELAPSED_TIME_H_

#include <chrono>
#include <regen/gl-types/gl-query.h>

namespace regen {
	class TimeElapsedQuery : public GLQuery<float> {
	public:
		TimeElapsedQuery() : GLQuery<float>(GL_TIME_ELAPSED) {}

	protected:
		float readQueryResult() const override {
			GLuint64 elapsedNano;
			glGetQueryObjectui64v(id(), GL_QUERY_RESULT, &elapsedNano);
			return elapsedNano * 1e-6f; // Convert nanoseconds to milliseconds
		}
	};

	/**
	 * \brief A class for debugging elapsed CPU and GPU time.
	 */
	class ElapsedTimeDebugger {
	public:
		enum Mode {
			CPU_ONLY,
			CPU_AND_GPU
		};

		ElapsedTimeDebugger(std::string_view sessionName, uint32_t numFrames, Mode mode = CPU_AND_GPU);

		~ElapsedTimeDebugger();

		void beginFrame();

		void push(std::string_view name);

		void endFrame();

	protected:
		const std::string sessionName_;
		const uint32_t numFrames_;
		const Mode mode_;
		ref_ptr<TimeElapsedQuery> query_;
		std::chrono::time_point<std::chrono::system_clock> cpuLastTime_;
		// CPU time in milliseconds
		std::vector<float> cpuTimes_;
		// GPU time in milliseconds
		std::vector<float> gpuTimes_;
		// Names of the key points
		std::vector<std::string> pushNames_;
		// Number of times each key point was pushed
		std::vector<uint32_t> pushCounts_;
		uint32_t pushIdx_ = 0;
		uint32_t frameIdx_ = 0;

		void pushTime(std::string_view name);

		void printResults();
	};
} // namespace

#endif /* REGEN_ELAPSED_TIME_H_ */

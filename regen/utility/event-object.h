#ifndef REGEN_EVENT_OBJECT_H_
#define REGEN_EVENT_OBJECT_H_

#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <functional>
#include <regen/utility/ref-ptr.h>

namespace regen {
	/**
	 * \brief Data passed from event emitter to event handlers.
	 */
	class EventData {
	public:
		virtual ~EventData() = default;
		/**
		 * The event identification number.
		 */
		uint32_t eventID = 0u;
	};

	class EventHandler; // forward declaration
	/**
	 * \brief Allows to integrate events into subclasses.
	 *
	 * Signal handler must implement the EventCallable interface.
	 * EventObject allows to queue emitting a signal in one thread
	 * and emit it in another (using emitQueued()).
	 */
	class EventObject {
	public:
		EventObject();

		virtual ~EventObject();

		EventObject(const EventObject &) = delete;

		EventObject &operator=(const EventObject &other) = delete;

		/**
		 * Register a single event on this object.
		 * The event is only identified by an unique name.
		 * Returns the event id.
		 * @param eventName name of the event, must be unique on the object
		 * @return the event id
		 */
		static uint32_t registerEvent(const std::string &eventName);

		/**
		 * Connect an event handler.
		 * You must save the returned id somewhere to be able to disconnect
		 * the handler.
		 */
		uint32_t connect(uint32_t eventId, const ref_ptr<EventHandler> &callable);

		/**
		 * Connect an event handler.
		 */
		uint32_t connect(const std::string &eventName, const ref_ptr<EventHandler> &callable);

		/**
		 * Disconnect an event handler.
		 */
		void disconnect(uint32_t connectionID);

		/**
		 * Disconnect all event handlers.
		 * This will disconnect all handlers registered on this object.
		 */
		void disconnectAll();

		/**
		 * Emit an event, call all handlers.
		 */
		void emitEvent(uint32_t eventID, const ref_ptr<EventData> &data = {});

		/**
		 * Emit an event, call all handlers.
		 */
		void emitEvent(const std::string &eventName, const ref_ptr<EventData> &data = {});

		/**
		 * Queue this event for emitting.
		 * It will be emitted next time emitQueue() called.
		 */
		void queueEmit(uint32_t eventID, const ref_ptr<EventData> &data = {});

		/**
		 * Un-queue previously queued event.
		 */
		void unQueueEmit(uint32_t eventID) const;

		/**
		 * Queue this event for emitting.
		 * It will be emitted next time emitQueue() called.
		 */
		void queueEmit(const std::string &eventName, const ref_ptr<EventData> &data = {});

		/**
		 * Emit previously queued events.
		 */
		static void dispatchEvents();

	protected:
		struct QueuedEvent {
			EventObject *emitter;
			ref_ptr<EventData> data;
			uint32_t eventID;
		};
		ref_ptr<EventData> fallbackEventData_;

		struct StaticData {;
			std::vector<QueuedEvent> eventQueue_[2];
			// index of the queue being written to by emitters
			std::atomic<int> eventPushIndex_{0};
			// the number of emitters currently writing to each queue
			std::atomic<int> eventPusherCount_[2];
		};
		static StaticData staticData_;

	private:
		uint32_t handlerCounter_ = 0;

		typedef std::pair<ref_ptr<EventHandler>, uint32_t> EventHandlerData;
		typedef std::vector<EventHandlerData> EventHandlerList;
		typedef std::unordered_map<uint32_t, EventHandlerList> EventHandlers;
		typedef std::unordered_map<uint32_t, uint32_t> EventHandlerIds;

		EventHandlers eventHandlers_;
		EventHandlerIds eventHandlerIds_;

		static std::unordered_map<std::string, uint32_t> &eventIds();

		static uint32_t &numEvents();

		static int pushLock();

		static void pushUnlock(int insertIdx);
	};

	/**
	 * \brief Baseclass for event handler.
	 */
	class EventHandler {
	public:
		EventHandler() = default;

		virtual ~EventHandler() = default;

		/**
		 * Call the event handler.
		 * @param emitter the EventObject that generated the event.
		 * @param data event data.
		 */
		virtual void call(EventObject *emitter, EventData *data) = 0;
	};

	/**
	 * \brief Event handler that calls a lambda function.
	 */
	class LambdaEventHandler : public EventHandler {
	public:
		/**
		 * @param f the lambda function to call.
		 */
		explicit LambdaEventHandler(std::function<void(EventObject *, EventData *)> f) : f_(f) {}

		~LambdaEventHandler() override = default;

		void call(EventObject *emitter, EventData *data) override {
			f_(emitter, data);
		}

	private:
		std::function<void(EventObject *, EventData *)> f_;
	};
} // namespace

#endif /* REGEN_EVENT_OBJECT_H_ */

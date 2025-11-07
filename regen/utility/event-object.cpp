#include "logging.h"
#include "event-object.h"
#include "threading.h"
#include <ranges>

using namespace regen;

//// static

EventObject::StaticData EventObject::staticData_;

unsigned int &EventObject::numEvents() {
	static unsigned int numEvents_ = 0;
	return numEvents_;
}

std::unordered_map<std::string, unsigned int> &EventObject::eventIds() {
	static auto eventIds_ = std::unordered_map<std::string, unsigned int>();
	return eventIds_;
}

///// none static

EventObject::EventObject() {
	fallbackEventData_ = ref_ptr<EventData>::alloc();
}

EventObject::~EventObject() {
	for (;;) {
		// load the current push index
		int insertIdx = pushLock();
		if (insertIdx == -1) continue;
		// mark event as removed
		for (auto & ev : staticData_.eventQueue_[0]) {
			if (ev.emitter == this) {
				ev.emitter = nullptr;
			}
		}
		// and decrement pusher count
		pushUnlock(insertIdx);
		break;
	}
}

unsigned int EventObject::registerEvent(const std::string &eventName) {
	++numEvents();
	eventIds().insert(make_pair(eventName, numEvents()));
	return numEvents();
}

unsigned int EventObject::connect(unsigned int eventId, const ref_ptr<EventHandler> &callable) {
	auto it = eventHandlers_.find(eventId);

	++handlerCounter_;
	eventHandlerIds_[handlerCounter_] = eventId;

	EventHandlerData p(callable, handlerCounter_);
	if (it != eventHandlers_.end()) {
		it->second.push_back(p);
	} else {
		EventHandlerList newList;
		newList.push_back(p);
		eventHandlers_[eventId] = newList;
	}

	return handlerCounter_;
}

unsigned int EventObject::connect(const std::string &eventName, const ref_ptr<EventHandler> &callable) {
	auto it = eventIds().find(eventName);
	if (it == eventIds().end()) {
		REGEN_WARN("Event name '" << eventName << "' not registered.");
		return 0;
	}
	return connect(it->second, callable);
}

void EventObject::disconnect(unsigned int connectionID) {
	const auto idNeedle = eventHandlerIds_.find(connectionID);
	if (idNeedle == eventHandlerIds_.end()) {
		REGEN_WARN("Signal with id=" << connectionID << " no known.");
		return; // handler id not found!
	}

	const unsigned int eventId = idNeedle->second;
	const auto signalHandlers = eventHandlers_.find(eventId);
	if (signalHandlers == eventHandlers_.end()) {
		REGEN_WARN("Signal with id=" << connectionID << " has no connected handlers.");
		return; // no handlers not found!
	}

	auto &l = signalHandlers->second;
	for (auto it = l.begin(); it != l.end(); ++it) {
		if (it->second != connectionID) continue;
		l.erase(it);
		break;
	}

	eventHandlerIds_.erase(idNeedle);
}

void EventObject::disconnectAll() {
	eventHandlers_.clear();
	eventHandlerIds_.clear();
	handlerCounter_ = 0;
}

void EventObject::emitEvent(unsigned int eventID, const ref_ptr<EventData> &data) {
	// make sure event data specifies at least event ID
	const ref_ptr<EventData> &d = data.get() ? data : fallbackEventData_;
	d->eventID = eventID;

	if (auto handlers = eventHandlers_.find(eventID); handlers != eventHandlers_.end()) {
		for (const auto &handler: handlers->second | std::views::keys) {
			handler->call(this, d.get());
		}
	}
}

void EventObject::emitEvent(const std::string &eventName, const ref_ptr<EventData> &data) {
	emitEvent(eventIds()[eventName], data);
}

void EventObject::dispatchEvents() {
	// Swap queues
	int dispatchIdx = staticData_.eventPushIndex_.load(std::memory_order_acquire);
	int nextPushIdx = (dispatchIdx == 0) ? 1 : 0;
	staticData_.eventPushIndex_.store(nextPushIdx, std::memory_order_release);

	// Wait for any concurrent pushers on dispatchIdx to finish
	while (staticData_.eventPusherCount_[dispatchIdx].load(std::memory_order_acquire) > 0) {
		CPU_PAUSE();
	}

	// Process events
	auto &queue = staticData_.eventQueue_[dispatchIdx];
	for (const auto &ev : queue) {
		if (ev.emitter) {
			ev.emitter->emitEvent(ev.eventID, ev.data);
		}
	}
	queue.clear();
}

int EventObject::pushLock() {
	// load the current push index
	int insertIdx = staticData_.eventPushIndex_.load(std::memory_order_acquire);
	// increment pusher count
	auto &count = staticData_.eventPusherCount_[insertIdx];
	count.fetch_add(1, std::memory_order_acquire);
	// make sure insertIdx is still valid, as it could be that dispatch has swapped the queues
	// in the meantime. If not, we are protected by the counter and can proceed.
	if (insertIdx != staticData_.eventPushIndex_.load(std::memory_order_acquire)) {
		count.fetch_sub(1, std::memory_order_release);
		return -1;
	}
	return insertIdx;
}

void EventObject::pushUnlock(int insertIdx) {
	// and decrement pusher count
	staticData_.eventPusherCount_[insertIdx].fetch_sub(1, std::memory_order_release);
}

void EventObject::queueEmit(unsigned int eventID, const ref_ptr<EventData> &data) {
	for (;;) {
		// load the current push index
		int insertIdx = pushLock();
		if (insertIdx == -1) continue;
		// Push the event to the queue
		staticData_.eventQueue_[insertIdx].emplace_back(this, data, eventID);
		// and decrement pusher count
		pushUnlock(insertIdx);
		break;
	}
}

void EventObject::unQueueEmit(unsigned int eventID) const {
	for (;;) {
		// load the current push index
		int insertIdx = pushLock();
		if (insertIdx == -1) continue;
		// mark event as removed
		for (auto & ev : staticData_.eventQueue_[insertIdx]) {
			if (ev.emitter == this && ev.eventID == eventID) {
				ev.emitter = nullptr;
				break;
			}
		}
		// and decrement pusher count
		pushUnlock(insertIdx);
		break;
	}
}

void EventObject::queueEmit(const std::string &eventName, const ref_ptr<EventData> &data) {
	queueEmit(eventIds()[eventName], data);
}

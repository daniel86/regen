/*
 * event-object.h
 *
 *  Created on: 29.01.2011
 *      Author: daniel
 */

#ifndef EVENT_OBJECT_H_
#define EVENT_OBJECT_H_

#include <string>
#include <map>
#include <set>
#include <vector>
using namespace std;

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <regen/utility/ref-ptr.h>

namespace regen {

class EventHandler; // forward declaration

/**
 * \brief Data passed from event emitter to event handlers.
 */
class EventData {
public:
  /**
   * The event identification number.
   */
  unsigned int eventID;
};

/**
 * \brief Allows to integrate events into subclasses.
 *
 * Signal handler must implement the EventCallable interface.
 * EventObject allows to queue emitting a signal in one thread
 * and emit it in another (using emitQueued()).
 */
class EventObject {
public:
  /**
   * Emit previously queued events.
   */
  static void emitQueued();

  EventObject();

  /**
   * Register a single event on this object.
   * The event is only identified by an unique name.
   * Returns the event id.
   * @param eventName name of the event, must be unique on the object
   * @return the event id
   */
  static unsigned int registerEvent(const string &eventName);

  /**
   * Connect an event handler.
   * You must save the returned id somewhere to be able to disconnect
   * the handler.
   * no out of bounds check performed!
   */
  unsigned int connect(unsigned int eventId, const ref_ptr<EventHandler> &callable);
  /**
   * Connect an event handler.
   */
  unsigned int connect(const string &eventName, const ref_ptr<EventHandler> &callable);

  /**
   * Disconnect an event handler.
   */
  void disconnect(unsigned int connectionID);
  /**
   * Disconnect an event handler.
   */
  void disconnect(const ref_ptr<EventHandler> &c);

  /**
   * Emit an event, call all handlers.
   * The data stays owned by caller.
   */
  void emitEvent(unsigned int eventID, EventData *data=NULL);
  /**
   * Emit an event, call all handlers.
   * The data stays owned by caller.
   */
  void emitEvent(const string &eventName, EventData *data=NULL);

  /**
   * Queue this event for emitting.
   * It will be emitted next time emitQueue() called.
   * The data is not owned by caller anymore.
   */
  void queueEmit(unsigned int eventID, EventData *data=NULL);
  /**
   * Queue this event for emitting.
   * It will be emitted next time emitQueue() called.
   * The data is not owned by caller anymore.
   */
  void queueEmit(const string &eventName, EventData *data=NULL);

protected:
  struct QueuedEvent {
    QueuedEvent(EventObject *_emitter, EventData *_data, unsigned int _eventID)
    : emitter(_emitter), data(_data), eventID(_eventID) {}
    EventObject *emitter;
    EventData *data;
    unsigned int eventID;
  };

  typedef pair<ref_ptr<EventHandler>, unsigned int> EventHandlerData;
  typedef vector< EventHandlerData > EventHandlerList;
  typedef map< unsigned int, EventHandlerList > EventHandlers;
  typedef map< unsigned int, unsigned int > EventHandlerIds;

  static list<QueuedEvent> pingQueue_;
  static list<QueuedEvent> pongQueue_;
  static list<QueuedEvent> *queued_;
  static list<QueuedEvent> *processing_;
  static boost::mutex eventLock_;

private:
  unsigned int handlerCounter_;

  EventHandlers eventHandlers_;
  EventHandlerIds eventHandlerIds_;

  EventObject(const EventObject&);
  EventObject& operator=(const EventObject &other);

  static map<string,unsigned int> &eventIds();
  static unsigned int &numEvents();

};

/**
 * \brief Baseclass for event handler.
 */
class EventHandler
{
public:
  EventHandler() {}
  virtual ~EventHandler() {}

  /**
   * Call the event handler.
   * @param emitter the EventObject that generated the event.
   * @param data event data.
   */
  virtual void call(EventObject *emitter, EventData *data) = 0;

  /**
   * @return the handler id.
   */
  unsigned int handlerID() const
  { return handlerID_; }
  /**
   * @param handlerID the handler id.
   */
  void set_handlerID(unsigned int handlerID)
  { handlerID_ = handlerID; }

private:
  unsigned int handlerID_;
};

} // namespace

#endif /* EVENT_OBJECT_H_ */

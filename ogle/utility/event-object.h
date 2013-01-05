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

#include <ogle/utility/callable.h>
#include <ogle/utility/ref-ptr.h>

class EventCallable; // forward declaration

/**
 * Allows to integrate events into subclasses.
 * Signal handler must implement the Callable interface.
 * EventObject allows to queue emitting a signal in one thread
 * and emit it in another (using emitQueued()).
 */
class EventObject {
public:
  typedef pair<ref_ptr<EventCallable>, unsigned int> EventData;
  typedef vector< EventData > EventHandlerList;
  /**
   * Maps event id to connected handlers.
   */
  typedef map< unsigned int, EventHandlerList > EventHandlers;
  /**
   * Maps handler id to signal id.
   */
  typedef map< unsigned int, unsigned int > EventHandlerIds;

  /**
   * Emit previously queued events.
   * Note: queueEmit() / emitQueued() are threadsafe functions
   */
  static void emitQueued();

  EventObject();

  /**
   * Register a single event on this object.
   * The event is only identified by an unique name.
   * Returns the event id.
   * @param eventName: name of the event, must be unique on the object
   * @return the event id
   */
  static unsigned int registerEvent(const string &eventName);

  /**
   * Connect an event handler.
   * You must save the returned id somewhere to be able to disconnect
   * the handler.
   * no out of bounds check performed!
   */
  unsigned int connect(unsigned int eventId, const ref_ptr<EventCallable> &callable);
  /**
   * Connect an event handler.
   */
  unsigned int connect(const string &eventName, const ref_ptr<EventCallable> &callable);

  /**
   * Disconnect an event handler.
   */
  void disconnect(unsigned int connectionID);
  /**
   * Disconnect an event handler.
   */
  void disconnect(const ref_ptr<EventCallable> &c);

  /**
   * emit an event, call all handlers.
   * no out of bounds check performed!
   */
  void emitEvent(unsigned int eventID, void *data=NULL);
  /**
   * emit an event, call all handlers.
   */
  void emitEvent(const string &eventName, void *data=NULL);

  /**
   * Queue this event for emitting.
   * It will be emitted next time emitQueue() called.
   */
  void queueEmit(unsigned int eventID);
  /**
   * Queue this event for emitting.
   * It will be emitted next time emitQueue() called.
   */
  void queueEmit(const string &eventName);

protected:
  static set< pair<EventObject*, unsigned int> > queued_;
  static boost::mutex eventLock_;

private:
  unsigned int handlerCounter_;

  /**
   * event handler.
   */
  EventHandlers eventHandlers_;
  EventHandlerIds eventHandlerIds_;

  EventObject(const EventObject&);
  EventObject& operator=(const EventObject &other);

  static map<string,unsigned int> &eventIds();
  static unsigned int &numEvents();

};

/**
 * Baseclass for event callbacks.
 */
class EventCallable : public Callable2<EventObject>
{
public:
  EventCallable() : Callable2<EventObject>() {}
  virtual ~EventCallable() {}

  unsigned int id() const;
  void set_id(unsigned int);
private:
  unsigned int id_;
};

#endif /* EVENT_OBJECT_H_ */

/*
 * event-object.cpp
 *
 *  Created on: 29.01.2011
 *      Author: daniel
 */

#include "event-object.h"
using namespace regen;

list<EventObject::QueuedEvent> EventObject::pingQueue_ = list<EventObject::QueuedEvent>();
list<EventObject::QueuedEvent> EventObject::pongQueue_ = list<EventObject::QueuedEvent>();
list<EventObject::QueuedEvent> *EventObject::queued_ = &EventObject::pingQueue_;
list<EventObject::QueuedEvent> *EventObject::processing_ = &EventObject::pongQueue_;

boost::mutex EventObject::eventLock_;

//// static

unsigned int &EventObject::numEvents()
{
  static unsigned int numEvents_ = 0;
  return numEvents_;
}

map<string,unsigned int> &EventObject::eventIds()
{
  static map<string,unsigned int> eventIds_ = map<string,unsigned int>();
  return eventIds_;
}

///// none static

EventObject::EventObject()
: handlerCounter_(0),
  eventHandlers_()
{
}

unsigned int EventObject::registerEvent(const string &eventName)
{
  ++EventObject::numEvents();
  EventObject::eventIds().insert( pair<string,unsigned int>(
      eventName, EventObject::numEvents()));
  return EventObject::numEvents();
}

unsigned int EventObject::connect(unsigned int eventId, const ref_ptr<EventHandler> &callable)
{
  EventHandlers::iterator it = eventHandlers_.find(eventId);

  ++handlerCounter_;
  eventHandlerIds_[handlerCounter_] = eventId;

  EventHandlerData p(callable, handlerCounter_);
  if( it != eventHandlers_.end() ) {
    it->second.push_back(p);
  } else {
    EventHandlerList newList;
    newList.push_back( p );
    eventHandlers_[eventId] = newList;
  }

  callable->set_handlerID(handlerCounter_);

  return handlerCounter_;
}
unsigned int EventObject::connect(const string &eventName, const ref_ptr<EventHandler> &callable)
{
  return connect(EventObject::eventIds()[eventName], callable);
}

void EventObject::disconnect(unsigned int connectionID)
{
  EventHandlerIds::iterator idNeedle =
      eventHandlerIds_.find(connectionID);
  if(idNeedle == eventHandlerIds_.end()) return; // handler id not found!

  unsigned int eventId = idNeedle->second;
  EventHandlers::iterator signalHandlers =
      eventHandlers_.find(eventId);
  if(signalHandlers == eventHandlers_.end()) return; // no handlers not found!

  EventHandlerList &l = signalHandlers->second;
  for(EventHandlerList::iterator it = l.begin(); it != l.end(); ++it)
  {
    if(it->second != connectionID) continue;

    l.erase(it);
    break;
  }

  eventHandlerIds_.erase(idNeedle);
}
void EventObject::disconnect(const ref_ptr<EventHandler> &c)
{
  disconnect(c->handlerID());
}

void EventObject::emitEvent(unsigned int eventID, EventData *data)
{
  // make sure event data specifies at least event ID
  EventData *data_, *d;
  if(!data) {
    data_ = new EventData;
    d = data_;
  } else {
    data_ = NULL;
    d = data;
  }
  d->eventID = eventID;

  EventHandlers::iterator it = eventHandlers_.find(eventID);
  if( it != eventHandlers_.end() ) {
    EventHandlerList::iterator jt;
    for(jt = it->second.begin(); jt != it->second.end(); ++jt)
    {
      jt->first->call(this,d);
    }
  }

  if(data_!=NULL) delete data_;
}
void EventObject::emitEvent(const string &eventName, EventData *data)
{
  emitEvent(EventObject::eventIds()[eventName], data);
}

void EventObject::emitQueued()
{
  // ping-pong event queue to avoid copy and long locks
  eventLock_.lock(); {
    list<QueuedEvent> *buf = queued_;
    queued_ = processing_;
    processing_ = buf;
  } eventLock_.unlock();

  // process queued events
  while(!processing_->empty())
  {
    QueuedEvent &ev = processing_->front();
    // emit event and delete data
    ev.emitter->emitEvent(ev.eventID, ev.data);
    // allocated from emitter but must be deleted here
    if(ev.data) delete ev.data;
    // pop out processed event
    processing_->pop_front();
  }
}

void EventObject::queueEmit(unsigned int eventID, EventData *data)
{
  eventLock_.lock(); {
    queued_->push_back(QueuedEvent(this,data,eventID));
  } eventLock_.unlock();
}
void EventObject::queueEmit(const string &eventName, EventData *data)
{
  queueEmit(EventObject::eventIds()[eventName],data);
}

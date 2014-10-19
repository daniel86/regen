/*
 * event-object.cpp
 *
 *  Created on: 29.01.2011
 *      Author: daniel
 */

#include "logging.h"
#include "event-object.h"
using namespace regen;

std::list<EventObject::QueuedEvent> EventObject::pingQueue_ = std::list<EventObject::QueuedEvent>();
std::list<EventObject::QueuedEvent> EventObject::pongQueue_ = std::list<EventObject::QueuedEvent>();
std::list<EventObject::QueuedEvent> *EventObject::queued_ = &EventObject::pingQueue_;
std::list<EventObject::QueuedEvent> *EventObject::processing_ = &EventObject::pongQueue_;

boost::mutex EventObject::eventLock_;

//// static

unsigned int &EventObject::numEvents()
{
  static unsigned int numEvents_ = 0;
  return numEvents_;
}

std::map<std::string,unsigned int> &EventObject::eventIds()
{
  static std::map<std::string,unsigned int> eventIds_ = std::map<std::string,unsigned int>();
  return eventIds_;
}

///// none static

EventObject::EventObject()
: handlerCounter_(0),
  eventHandlers_()
{
}
EventObject::~EventObject()
{
  eventLock_.lock(); {
    std::list<QueuedEvent>::iterator i = queued_->begin();
    while(i != queued_->end()) {
        if(i->emitter == this) queued_->erase(i++);
        else ++i;
    }
  } eventLock_.unlock();

  for(EventHandlers::iterator
      it=eventHandlers_.begin(); it!=eventHandlers_.end(); ++it) {
    for(EventHandlerList::iterator
        jt=it->second.begin(); jt!=it->second.end(); ++jt) {
      jt->first->set_handlerID(-1);
    }
  }
}

unsigned int EventObject::registerEvent(const std::string &eventName)
{
  ++EventObject::numEvents();
  EventObject::eventIds().insert(make_pair(eventName, EventObject::numEvents()));
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
unsigned int EventObject::connect(const std::string &eventName, const ref_ptr<EventHandler> &callable)
{
  return connect(EventObject::eventIds()[eventName], callable);
}

void EventObject::disconnect(unsigned int connectionID)
{
  EventHandlerIds::iterator idNeedle =
      eventHandlerIds_.find(connectionID);
  if(idNeedle == eventHandlerIds_.end()) {
    REGEN_WARN("Signal with id=" << connectionID << " no known.");
    return; // handler id not found!
  }

  unsigned int eventId = idNeedle->second;
  EventHandlers::iterator signalHandlers =
      eventHandlers_.find(eventId);
  if(signalHandlers == eventHandlers_.end()) {
    REGEN_WARN("Signal with id=" << connectionID << " has no connected handlers.");
    return; // no handlers not found!
  }

  EventHandlerList &l = signalHandlers->second;
  for(EventHandlerList::iterator it = l.begin(); it != l.end(); ++it)
  {
    if(it->second != connectionID) continue;
    it->first->set_handlerID(-1);
    l.erase(it);
    break;
  }

  eventHandlerIds_.erase(idNeedle);
}
void EventObject::disconnect(const ref_ptr<EventHandler> &c)
{
  disconnect(c->handlerID());
}

void EventObject::emitEvent(unsigned int eventID, const ref_ptr<EventData> &data)
{
  // make sure event data specifies at least event ID
  ref_ptr<EventData> d = data;
  if(!d.get())
  { d = ref_ptr<EventData>::alloc(); }
  d->eventID = eventID;

  EventHandlers::iterator it = eventHandlers_.find(eventID);
  if( it != eventHandlers_.end() ) {
    EventHandlerList::iterator jt;
    for(jt = it->second.begin(); jt != it->second.end(); ++jt)
    {
      jt->first->call(this,d.get());
    }
  }
}
void EventObject::emitEvent(const std::string &eventName, const ref_ptr<EventData> &data)
{
  emitEvent(EventObject::eventIds()[eventName], data);
}

void EventObject::emitQueued()
{
  // ping-pong event queue to avoid copy and long locks
  eventLock_.lock(); {
    std::list<QueuedEvent> *buf = queued_;
    queued_ = processing_;
    processing_ = buf;
  } eventLock_.unlock();

  // process queued events
  while(!processing_->empty())
  {
    QueuedEvent &ev = processing_->front();
    // emit event and delete data
    ev.emitter->emitEvent(ev.eventID, ev.data);
    // pop out processed event
    processing_->pop_front();
  }
}

void EventObject::unqueueEmit(unsigned int eventID)
{
  eventLock_.lock(); {
    for(std::list<QueuedEvent>::iterator it=queued_->begin(); it!=queued_->end(); ++it) {
      const QueuedEvent &ev = *it;
      if(ev.emitter==this && ev.eventID==eventID) {
        queued_->erase(it);
        break;
      }
    }
  } eventLock_.unlock();
}
void EventObject::queueEmit(unsigned int eventID, const ref_ptr<EventData> &data)
{
  eventLock_.lock(); {
    queued_->push_back(QueuedEvent(this,data,eventID));
  } eventLock_.unlock();
}
void EventObject::queueEmit(const std::string &eventName, const ref_ptr<EventData> &data)
{
  queueEmit(EventObject::eventIds()[eventName],data);
}

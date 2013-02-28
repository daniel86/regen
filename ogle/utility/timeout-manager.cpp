/*
 * timeout-manager.cpp
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#include "timeout-manager.h"
#include "logging.h"

#include <ogle/config.h>

/**
 * Milliseconds to sleep per loop in idle mode.
 */
#define IDLE_SLEEP_MS 100

// boost adds 100ms to desired interval !?!
//  * with version 1.50.0-2
//  * not known as of 14.08.2012
#define BOOST_SLEEP_BUG

void Timeout::timeout__(
    const boost::posix_time::time_duration &dt,
    const boost::int64_t &milliSeconds)
{
  timeout(milliSeconds);
  time_ += dt;
}

void Timeout::set_time(const boost::posix_time::ptime &time)
{
  time_ = time;
}

const boost::posix_time::ptime& Timeout::time() const
{
  return time_;
}

////////////////
////////////////

TimeoutManager& TimeoutManager::get()
{
  static TimeoutManager manager;
  return manager;
}

TimeoutManager::TimeoutManager()
: closeFlag_(false)
{
  DEBUG_LOG("entering timeout thread.");
  time_ = boost::posix_time::ptime(
      boost::posix_time::microsec_clock::local_time());
  lastTime_ = time_;

  timeoutThread_ = boost::thread(&TimeoutManager::run, this);
}

TimeoutManager::~TimeoutManager()
{
  DEBUG_LOG("exiting timeout thread.");
  timeoutLock_.lock(); {
    closeFlag_ = true;
  } timeoutLock_.unlock();
  timeoutThread_.join();
}

void TimeoutManager::addTimeout(Timeout* timeout, bool callTimeout)
{
  // queue adding the timeout
  // in the timeout thread
  timeoutLock_.lock(); { // lock shared newTimeouts_
    newTimeouts_.push_back(timeout);
  } timeoutLock_.unlock();

  if(callTimeout) {
    boost::posix_time::time_duration dt = time_ - lastTime_;
    boost::int64_t milliSeconds = dt.total_milliseconds();
    timeout->timeout__(dt, milliSeconds);
  }
}
void TimeoutManager::removeTimeout(Timeout* timeout)
{
  timeoutLock_.lock(); { // lock shared removedTtimeout_
    removedTimeouts_.push_back(timeout);
  } timeoutLock_.unlock();
}

void TimeoutManager::run()
{
  while(!closeFlag_) { // execute timeouts

    time_ = boost::posix_time::ptime(
        boost::posix_time::microsec_clock::local_time());

    // handle added/removed timeouts
    timeoutLock_.lock(); {
      // remove completed timeouts
      list< Timeout* >::iterator it, jt;
      for(it = removedTimeouts_.begin(); it!=removedTimeouts_.end(); ++it)
      {
        for(jt = timeouts_.begin(); jt!=timeouts_.end(); ++jt)
        {
          if(*it == *jt) {
            timeouts_.erase(jt);
            break;
          }
        }
      }
      removedTimeouts_.clear();

      // and add new timeouts
      for(it = newTimeouts_.begin(); it!=newTimeouts_.end(); ++it)
      {
        (*it)->set_time(time_);
        timeouts_.push_back(*it);
      }
      newTimeouts_.clear();
    } timeoutLock_.unlock();

    if(timeouts_.size() > 0) {
      boost::posix_time::time_duration dt = time_ - lastTime_;
      boost::int64_t milliSeconds = dt.total_milliseconds();

      boost::posix_time::time_duration minIntervall(
          boost::posix_time::milliseconds(IDLE_SLEEP_MS));
      for(list< Timeout* >::iterator it = timeouts_.begin();
          it != timeouts_.end(); ++it)
      {
        Timeout *timeout = *it;

        if(timeout->interval() < minIntervall) {
          minIntervall = timeout->interval();
        }
        if(timeout->time()+timeout->interval() > time_) {
          continue; // no need to recalculate
        }

        timeout->timeout__(dt, milliSeconds);
      }

#ifdef UNIX
      // i have a strange problem with boost::this_thread here.
      // it just adds 100ms to the interval provided :/
      usleep(minIntervall.total_microseconds());
#else
      boost::this_thread::sleep(boost::posix_time::milliseconds(minIntervall.total_milliseconds()));
#endif
    } else {
#ifdef UNIX
      usleep(IDLE_SLEEP_MS*1000);
#else
      boost::this_thread::sleep(boost::posix_time::milliseconds(IDLE_SLEEP_MS));
#endif
    }
    // remember last time
    lastTime_ = time_;
  }
}

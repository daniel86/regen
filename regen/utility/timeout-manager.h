/*
 * timeout-manager.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef __TIMEOUT_MANAGER_H_
#define __TIMEOUT_MANAGER_H_

#include <list>
#include <set>
using namespace std;

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/xtime.hpp>

namespace regen {
/**
 * \brief Interface for timeouts.
 */
class Timeout {
public:
  /**
   * Make the timeout callback.
   * Calling doCallback() that is implemented by subclasses.
   */
  void timeout__(
      const boost::posix_time::time_duration &dt,
      const boost::int64_t &milliSeconds);
  /**
   * Set last time of execution.
   */
  void set_time(const boost::posix_time::ptime &time);
  /**
   * Get last time of execution.
   */
  const boost::posix_time::ptime& time() const;

  /**
   * Returns the interval of the timout in milliseconds.
   */
  virtual const boost::posix_time::time_duration interval() const = 0;

protected:
  boost::posix_time::ptime time_;

  virtual void timeout(const boost::int64_t &dt) = 0;
};

/**
 * \brief Singleton class that executes timeouts in a separate thread.
 */
class TimeoutManager
{
public:
  /**
   * Get the singleton instance.
   */
  static TimeoutManager& get();

  /**
   * Add a timeout to the manager.
   */
  void addTimeout(Timeout *timeout, bool callTimeout=false);
  /**
   * Remove a timeout from the manager.
   */
  void removeTimeout(Timeout *timeout);

private:
  ///// main thread only
  boost::thread timeoutThread_;

  ///// timeout thread only
  boost::posix_time::ptime time_;
  boost::posix_time::ptime lastTime_;
  list< Timeout* > timeouts_;

  ///// shared
  list< Timeout* > newTimeouts_;
  list< Timeout* > removedTimeouts_;
  boost::mutex timeoutLock_;
  bool closeFlag_;

  /**
   * Private because its a singleton.
   */
  TimeoutManager();
  ~TimeoutManager();

  void run();
  boost::int64_t timeout();
};

} // namespace

#endif /* __TIMEOUT_MANAGER_H_ */

/*
 * threading.h
 *
 *  Created on: 23.04.2013
 *      Author: daniel
 */

#ifndef __THREADING_H_
#define __THREADING_H_

#include <regen/config.h>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

namespace regen {
  /**
   * \brief Simple thread class using boost::thread.
   */
  class Thread {
  public:
    /**
     * Sleep for given number of microseconds.
     */
    static void usleep(unsigned int microSeconds);

    Thread();
  protected:
    boost::thread thread_;
    boost::mutex threadLock_;

    void __run();
    virtual void run()=0;
  };
} // namespace

#endif /* __THREADING_H_ */

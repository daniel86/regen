/*
 * interfaces.h
 *
 *  Created on: 10.03.2013
 *      Author: daniel
 */

#ifndef INTERFACES_H_
#define INTERFACES_H_

namespace ogle {
/**
 * \brief interface for resizable objects.
 */
class Resizable {
public:
  virtual ~Resizable() {}
  /**
   * Resize buffers / textures.
   */
  virtual void resize()=0;
};
} // namespace

#endif /* INTERFACES_H_ */

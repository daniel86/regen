/*
 * buffer-data.h
 *
 *  Created on: 20.03.2011
 *      Author: daniel
 */

#ifndef BUFFER_DATA_H_
#define BUFFER_DATA_H_

/**
 * Stores a data pointer.
 */
class BufferData {
public:
  BufferData(void *data=NULL)
  : data_(data), bufferChanged_(false), offsetInDataBuffer_(0)
  {
  }
  /**
   * Get data pointer.
   */
  void* data()
  {
    return data_;
  }
  void set_data(void *data)
  {
    data_ = data;
  }
  void set_bufferChanged(bool bufferChanged)
  {
    bufferChanged_ = bufferChanged;
  }
  bool bufferChanged() const
  {
    return bufferChanged_;
  }
  void set_offsetInDataBuffer(unsigned int offsetInDataBuffer) {
    offsetInDataBuffer_ = offsetInDataBuffer;
  }
  unsigned int offsetInDataBufferToAnimationBufferStart() const {
    return offsetInDataBuffer_;
  }
protected:
  void *data_;
  bool bufferChanged_;
  unsigned int offsetInDataBuffer_;
};

#endif /* BUFFER_DATA_H_ */

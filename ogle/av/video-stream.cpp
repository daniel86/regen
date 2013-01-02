/*
 * video-stream.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include "video-stream.h"

extern "C" {
  #include <libswscale/swscale.h>
}

#define GL_RGB_PIXEL_FORMAT PIX_FMT_RGB24

VideoStream::VideoStream(AVStream *stream, GLint index, GLuint chachedBytesLimit)
: AudioVideoStream(stream, index, chachedBytesLimit)
{
  stream_ = stream;
  width_ = codecCtx_->width;
  height_ = codecCtx_->height;
  if(width_<1 || height_<1)
  {
    throw new VideoStreamError("invalid video size");
  }
  // get sws context for converting from YUV to RGB
  swsCtx_ = sws_getContext(
      codecCtx_->width,
      codecCtx_->height,
      codecCtx_->pix_fmt,
      codecCtx_->width,
      codecCtx_->height,
      GL_RGB_PIXEL_FORMAT,
      SWS_FAST_BILINEAR,
      NULL, NULL, NULL);
}
VideoStream::~VideoStream()
{
  clearQueue();
}

GLenum VideoStream::texInternalFormat() const
{
  return GL_RGB;
}
GLenum VideoStream::texFormat() const
{
  return GL_RGB;
}
GLenum VideoStream::texPixelType() const
{
  return GL_UNSIGNED_BYTE;
}

void VideoStream::clearQueue()
{
  //boost::lock_guard<boost::mutex> lock(decodingLock_);
  while(decodedFrames_.size()>0) {
    AVFrame *f = frontFrame();
    delete (float*)f->opaque;
    popFrame();
    av_free(f);
  }
}

void VideoStream::decode(AVPacket *packet)
{
  AVFrame *frame = avcodec_alloc_frame();
  int frameFinished = 0;
  // Decode video frame
  avcodec_decode_video2(codecCtx_, frame, &frameFinished, packet);
  // Did we get a video frame?
  if(!frameFinished) {
    av_free(frame);
    return;
  }
  // YUV to RGB conversation. could be done on GPU with pixel shader. Maybe later....
  AVFrame *rgb = avcodec_alloc_frame();
  int numBytes = avpicture_get_size(
      GL_RGB_PIXEL_FORMAT,
      codecCtx_->width,
      codecCtx_->height);
  if(numBytes < 1) {
    av_free(frame);
    return;
  }
  uint8_t *buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
  numBytes = avpicture_fill(
      (AVPicture *)rgb,
      buffer,
      GL_RGB_PIXEL_FORMAT,
      codecCtx_->width,
      codecCtx_->height);
  if(numBytes < 1) {
    av_free(frame);
    av_free(buffer);
    return;
  }
  sws_scale(swsCtx_,
      frame->data,
      frame->linesize,
      0,
      codecCtx_->height,
      rgb->data,
      rgb->linesize);
  av_free(frame);
  // remember timestamp in frame
  float *dt = new float;
  *dt = packet->dts*av_q2d(stream_->time_base);
  //*dt = frame->pts*av_q2d(stream_->time_base);
  rgb->opaque = dt;
  // free package and put the frame in queue of decoded frames
  av_free_packet(packet);
  pushFrame(rgb, numBytes);
}


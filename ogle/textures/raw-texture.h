/*
 * raw-texture.h
 *
 *  Created on: 29.12.2012
 *      Author: daniel
 */

#ifndef RAW_TEXTURE_H_
#define RAW_TEXTURE_H_

#include <ogle/gl-types/texture.h>
#include <ogle/exceptions/io-exceptions.h>

typedef struct {
  string path;
  GLuint width, height, depth;
  GLuint numComponents;
  GLuint bytesPerComponent;
}RAWTextureFile;

class RAWTexture3D : public Texture3D
{
public:
  RAWTexture3D();
  void loadRAWFile(const RAWTextureFile &raw) throw (FileNotFoundException);
};

#endif /* RAW_TEXTURE_H_ */

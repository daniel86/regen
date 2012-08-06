/*
 * volume.h
 *
 *  Created on: 08.03.2012
 *      Author: daniel
 */

#ifndef VOLUME_H_
#define VOLUME_H_

#include <ogle/states/attribute-state.h>
#include <ogle/gl-types/volume-texture.h>
#include <ogle/models/cube.h>

typedef enum {
  VOLUME_DRAW_R_TO_ALPHA
}VolumeDrawMode;

typedef struct {
  ref_ptr<Texture3D> tex_;
  VolumeDrawMode drawMode_;
  Vec3f color_;
}VolumeData;

class Volume
{
public:
  Volume() {}
  void addVolumeData(ref_ptr<VolumeData> data);
  void removeVolumeData(ref_ptr<VolumeData> data);
protected:
  list< ref_ptr<VolumeData> > volumeData_;
};

class BoxVolume : public Volume, public Cube
{
public:
  BoxVolume();
};

#endif /* VOLUME_H_ */

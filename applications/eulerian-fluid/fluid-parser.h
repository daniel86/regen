/*
 * fluid-parser.h
 *
 *  Created on: 09.10.2012
 *      Author: daniel
 */

#ifndef FLUID_PARSER_H_
#define FLUID_PARSER_H_

#include <string>
using namespace std;

#include "fluid.h"

#include <ogle/states/mesh-state.h>

/**
 * Loads Fluid's from XML definition files.
 */
class FluidParser
{
public:
  /**
   * Load fluid from XML file.
   */
  static Fluid* readFluidFileXML(
      MeshState *textureQuad,
      const string &fluidFile);
  /**
   * Load Fluid from XML string.
   */
  static Fluid* parseFluidStringXML(
      MeshState *textureQuad,
      char *xmlString);
};

#endif /* FLUID_PARSER_H_ */

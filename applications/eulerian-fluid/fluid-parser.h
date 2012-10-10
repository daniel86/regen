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

class FluidParser
{
public:
  static Fluid* readFluidFileXML(const string &fluidFile);
  static Fluid* parseFluidStringXML(char *xmlString);
};

#endif /* FLUID_PARSER_H_ */

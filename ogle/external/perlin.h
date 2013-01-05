/*
* perlin.h
*
* Created on: 11.03.2012
* Author: daniel
*/
#ifndef PERLIN_H_
#define PERLIN_H_

double noise1(double);
double noise2(double *);
double noise3(double *);
void normalize3(double *);
void normalize2(double *);
double perlinNoise1D(double,double,double,int);
double perlinNoise2D(double,double,double,double,int);
double perlinNoise3D(double,double,double,double,double,int);

#endif /* PERLIN_H_ */

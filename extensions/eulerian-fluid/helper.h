/*
 * helper.h
 *
 *  Created on: 24.02.2012
 *      Author: daniel
 */

#ifndef EULERIAN_HELPER_H_
#define EULERIAN_HELPER_H_

#include "texture.h"
#include "fbo.h"

#include "primitive.h"

/**
 * Stage in the fluid simulation.
 * Will be updated each simulation step.
 */
class EulerianStage
{
public:
  EulerianStage(EulerianPrimitive *primitive) : primitive_(primitive) {}
  virtual void update() = 0;

  void activateTexture(Shader *shader, Texture *tex, GLuint unit)
  {
    glActiveTexture(GL_TEXTURE0 + unit);
    tex->bind();
    glUniform1i(
        shader->textureToLocation()[tex].first,
        unit );
  }
protected:
  EulerianPrimitive *primitive_;
};

/**
 * A frame buffer plus associated texture(s).
 */
typedef struct {
  ref_ptr<Texture> tex;
  ref_ptr<FrameBufferObject> fbo;
}FluidBuffer;

/**
 * For buffers with multiple textures this will switch
 * frame buffer rendering target and active texture for bind.
 * You can always bind the last rendering result of the frame buffer
 * if you swap after rendering to the fbo.
 */
void swapBuffer(FluidBuffer &slab);

/**
 * Write zeros to the texture(s).
 */
void clearSlab(FluidBuffer &slab);

/**
 * Create a slab suitable for fluid simulation.
 */
FluidBuffer createSlab(
    EulerianPrimitive *primitive,
    int numComponents,
    int numTexs,
    bool useHalfFloats);

/**
 * Create a texture suitable for fluid simulation.
 */
ref_ptr<Texture> createTexture(
    GLint width, GLint height, GLint depth,
    GLint numComponents,
    GLint numTexs,
    GLboolean useHalfFloats);

string findNeighborsGLSL(const string &varName, const string &texName, bool is2D);
string isOutsideSimulationDomainGLSL(bool is2D);
string isNonEmptyCellGLSL(bool is2D);
string gradientGLSL(bool is2D);
string gradientLiquidGLSL(bool is2D);

vector<string> makeShaderArgs();

#endif /* EULERIAN_HELPER_H_ */

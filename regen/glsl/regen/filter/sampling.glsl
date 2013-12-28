
-- computeTexco
#ifndef __computeTexco_Included__
#define2 __computeTexco_Included__
#if RENDER_LAYER > 1
flat in int in_layer;
#endif
#if RENDER_TARGET == CUBE
#include regen.math.computeCubeDirection
vec3 computeTexco(vec2 texco_2D) {
  return computeCubeDirection(vec2(2,-2)*texco_2D + vec2(-1,1),in_layer);
}
#define vecTexco vec3
#elif RENDER_LAYER > 1
vec3 computeTexco(vec2 texco_2D) {
  return vec3(texco_2D,in_layer);
}
#define vecTexco vec3
#else
#define computeTexco(texco_2D) texco_2D
#define vecTexco vec2
#endif
#endif

-- vs
in vec3 in_pos;

void main() {
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
#include regen.states.camera.defines
#if RENDER_LAYER > 1
#extension GL_EXT_geometry_shader4 : enable
#define2 __MAX_VERTICES__ ${${RENDER_LAYER}*3}

layout(triangles) in;
layout(triangle_strip, max_vertices=${__MAX_VERTICES__}) out;

flat out int out_layer;

#define HANDLE_IO(i)

void emitVertex(vec4 pos, int index, int layer) {

  gl_Position = pos;
  HANDLE_IO(index);
  EmitVertex();
}

void main() {
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
  gl_Layer = ${LAYER};
  out_layer = ${LAYER};
  emitVertex(gl_PositionIn[0], 0, ${LAYER});
  emitVertex(gl_PositionIn[1], 1, ${LAYER});
  emitVertex(gl_PositionIn[2], 2, ${LAYER});
  EndPrimitive();
#endif
#endfor
}
#endif

-- fs
#include regen.states.camera.defines

out vec4 out_color;

uniform vec2 in_inverseViewport;
#if RENDER_TARGET == 2D_ARRAY
uniform sampler2DArray in_inputTexture;
#elif RENDER_TARGET == CUBE
uniform samplerCube in_inputTexture;
#else // RENDER_TARGET == 2D
uniform sampler2D in_inputTexture;
#endif
#include regen.filter.sampling.computeTexco

void main()
{
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    out_color = texture(in_inputTexture, computeTexco(texco_2D));
}

----------------------------
----------------------------

-- array-row.vs
#include regen.filter.sampling.vs
-- array-row.fs
out vec4 out_color;
uniform sampler2DArray in_arrayTexture;
uniform int in_arrayTextureSize;
uniform vec2 in_inverseViewport;
uniform vec2 in_viewport;

void main() {
  float size = in_viewport.x/in_arrayTextureSize;
  float diffY = gl_FragCoord.y-in_viewport.y*0.5;
  
  if(abs(diffY) > 0.5*size) {
    out_color = vec4(0);
  }
  else {
    float arrayIndex = floor(gl_FragCoord.x/size);
    // Map in range [0,size] and divide by size to get to range [0,1]
    float texcoX = mod(gl_FragCoord.x,size)/size;
    float texcoY = (diffY + 0.5*size)/size;
    out_color = texture(in_arrayTexture, vec3(texcoX,texcoY,arrayIndex));
  }
}

----------------------------
----------------------------

-- cube-unfold.vs
#include regen.filter.sampling.vs
-- cube-unfold.fs
out vec4 out_color;
uniform sampler2DCube in_cubeTexture;

#include regen.math.computeCubeDirection

void main() {
  float size = in_viewport.y/4.0;
  float diffX = gl_FragCoord.x-in_viewport.x*0.5;
  float diffY = gl_FragCoord.y-in_viewport.y+1.5*size;
  
  if(abs(diffX) <= 0.5*size) { // middle row
    // Map in range [0,size] and divide by size to get to range [0,1]
    float texcoX = (diffX + 0.5*size)/size;
    float texcoY = mod(gl_FragCoord.y,size)/size;
    int layer;
    if(diffY>=0.5*size)       layer = 2; // +Y face
    else if(diffY>=-0.5*size) layer = 4; // +Z face
    else if(diffY>=-1.5*size) layer = 3; // -Y face
    else {                               // -Z face
      layer = 5;
      texcoY = 1.0-texcoY;
      texcoX = 1.0-texcoX;
    }
    out_color = texture(in_cubeTexture,
	computeCubeDirection(2.0*vec2(texcoX,texcoY) - vec2(1.0),layer));
  }
  else if(abs(diffX) < 1.5*size && abs(diffY) < 0.5*size) {
    float texcoX;
    float texcoY = mod(gl_FragCoord.y,size)/size;
    int layer;
    if(diffX <= 0) {
      // -X face
      texcoX = (diffX + 1.5*size)/size;
      layer = 1;
    }
    else {
      // +X face
      texcoX = (diffX - 0.5*size)/size;
      layer = 0;
    }
    out_color = texture(in_cubeTexture,
	computeCubeDirection(2.0*vec2(texcoX,texcoY) - vec2(1.0),layer));
  }
  else {
    out_color = vec4(0);
  }
}

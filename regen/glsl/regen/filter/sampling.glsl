
-- computeTexco
#ifndef sampling_computeTexco_Included
#define2 sampling_computeTexco_Included
#if RENDER_LAYER > 1
flat in int in_layer;
#endif
#if RENDER_TARGET == CUBE
#define vecTexco vec3
    #include regen.math.computeCubeDirection
vec3 computeTexco(vec2 texco_2D) {
    return computeCubeDirection(vec2(2,-2)*texco_2D + vec2(-1,1),in_layer);
}
#elif RENDER_LAYER > 1
#define vecTexco vec3
vec3 computeTexco(vec2 texco_2D) {
    return vec3(texco_2D,in_layer);
}
#else
    #define2 _TEX_ID ${TEX_ID_inputTexture}
    #define2 _TEX_DIM ${TEX_DIM${_TEX_ID}}
    #if ${_TEX_DIM} == 1
#define vecTexco float
#define computeTexco(texco_2D) texco_2D.x
    #else
#define vecTexco vec2
#define computeTexco(texco_2D) texco_2D
    #endif // ${_TEX_DIM} == 1
#endif // sampling_computeTexco_Included

-- vs
in vec3 in_pos;

void main() {
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
#include regen.states.camera.defines
#if RENDER_LAYER > 1
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
  emitVertex(gl_in[0].gl_Position, 0, ${LAYER});
  emitVertex(gl_in[1].gl_Position, 1, ${LAYER});
  emitVertex(gl_in[2].gl_Position, 2, ${LAYER});
  EndPrimitive();
#endif
#endfor
}
#endif

-- fs
#include regen.states.camera.defines

out vec4 out_color;

uniform vec2 in_inverseViewport;
uniform sampler2D in_inputTexture;
#include regen.filter.sampling.computeTexco

void main()
{
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    out_color = texture(in_inputTexture, computeTexco(texco_2D));
}

----------------------------
----------------------------
-- depth.vs
#include regen.filter.sampling.vs
-- depth.gs
#include regen.filter.sampling.gs
-- depth.fs
#include regen.states.camera.defines

out vec4 out_color;

uniform vec2 in_inverseViewport;
uniform sampler2D in_inputTexture;
#include regen.filter.sampling.computeTexco
#include regen.states.camera.linearizeDepth

void main()
{
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    float depth = texture(in_inputTexture, computeTexco(texco_2D)).r;
    depth = linearizeDepth(depth, REGEN_CAM_NEAR_(in_layer), REGEN_CAM_FAR_(in_layer));
    out_color = vec4(depth, depth, depth, 1.0);
}

----------------------------
----------------------------
-- downsample.depth.vs
#define OUTPUT_TYPE DEPTH
#include regen.filter.sampling.vs
-- downsample.depth.gs
#define OUTPUT_TYPE DEPTH
#include regen.filter.sampling.gs
-- downsample.depth.fs
#define OUTPUT_TYPE DEPTH
#include regen.states.camera.defines

uniform vec2 in_inverseViewport;
uniform sampler2D in_inputTexture;
#include regen.filter.sampling.computeTexco

void main()
{
    vec2 texCoord = gl_FragCoord.xy*in_inverseViewport;
    texCoord = computeTexco(texCoord);
    gl_FragDepth = texture(in_inputTexture, texCoord).r;
}

-- downsample.depth.average.2x2
#ifndef downsample_depth_average_2x2_Included
#define2 downsample_depth_average_2x2_Included
float downsample(vec2 texCoord, sampler2D depthTexture, float texelSize) {
    float depth = 0.0;
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            depth += texture(depthTexture, texCoord + offset).r;
        }
    }
    return depth / 4.0;
}
#endif

----------------------------
----------------------------
-- x4.vs
#include regen.filter.sampling.vs
-- x4.fs
#include regen.states.camera.defines

out vec4 out_color;

uniform vec2 in_inverseViewport;
uniform sampler2D in_inputTexture1;
uniform sampler2D in_inputTexture2;
uniform sampler2D in_inputTexture3;
uniform sampler2D in_inputTexture4;

void main()
{
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;

    if (texco_2D.x > 0.5)
    {
        if (texco_2D.y > 0.5)
        {
            out_color = texture(in_inputTexture3, (texco_2D-vec2(0.5))*2.0);
        }
        else
        {
            out_color = texture(in_inputTexture1, (texco_2D-vec2(0.5,0.0))*2.0);
        }
    } else {
        if (texco_2D.y > 0.5)
        {
            out_color = texture(in_inputTexture4, (texco_2D-vec2(0.0,0.5))*2.0);
        }
        else
        {
            out_color = texture(in_inputTexture2, texco_2D*2.0);
        }
    }
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

----------------------------
----------------------------

-- cube-shadow-unfold.vs
#include regen.filter.sampling.vs
-- cube-shadow-unfold.fs
out vec4 out_color;
uniform samplerCubeShadow in_cubeTexture;

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
    vec3 dir = computeCubeDirection(2.0*vec2(texcoX,texcoY) - vec2(1.0),layer);
    float depth = texture(in_cubeTexture, vec4(dir,1.0));
    out_color = vec4(depth, depth, depth, 1.0);
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
    vec3 dir = computeCubeDirection(2.0*vec2(texcoX,texcoY) - vec2(1.0),layer);
    float depth = texture(in_cubeTexture, vec4(dir,1.0));
    out_color = vec4(depth, depth, depth, 1.0);
  }
  else {
    out_color = vec4(0);
  }
}

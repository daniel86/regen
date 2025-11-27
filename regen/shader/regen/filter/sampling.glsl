
-- io
out vec4 out_color;
uniform vec2 in_inverseViewport;
uniform sampler2D in_inputTexture;
const vec3 in_borderColor = vec3(0.3,0.3,0.3);
const float in_borderWidth = 0.02;
#ifndef HAS_near
const float in_near = 0.1;
#endif
#ifndef HAS_far
const float in_far = 100.0;
#endif

-- writeDepth
#ifndef writeDepth_Included
#define2 writeDepth_Included
#include regen.states.camera.linearizeDepth
void writeDepth(float depth) {
#ifdef HAS_cameraProjParams
    depth = linearizeDepth(depth, REGEN_CAM_NEAR_(in_layer), REGEN_CAM_FAR_(in_layer));
#else
    depth = linearizeDepth(depth, in_near, in_far);
#endif
    out_color = vec4(depth, depth, depth, 1.0);
}
#endif

-- drawEdges
#ifndef drawEdges_Included
#define2 drawEdges_Included
void drawEdges(vec2 uv) {
    if(uv.x < in_borderWidth || uv.x + in_borderWidth > 1.0 ||
       uv.y < in_borderWidth || uv.y + in_borderWidth> 1.0) {
        out_color.rgb = in_borderColor;
    }
}
#endif

-- computeTexco
#ifndef sampling_computeTexco_Included
#define2 sampling_computeTexco_Included
#include regen.layered.defines
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
#include regen.defines.all
in vec3 in_pos;
#ifdef VS_LAYER_SELECTION
flat out int out_layer;
#endif
#include regen.layered.VS_SelectLayer

void main() {
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
    VS_SelectLayer(regen_RenderLayer());
}

-- gs
// pass-through geometry shader, e.g. in case it is needed
// for layer selection.
#define SKIP_GS_TRANSFORM
#include regen.models.mesh.gs

-- fs
#include regen.states.camera.defines
#include regen.filter.sampling.io
#include regen.filter.sampling.computeTexco
#if TEXTURE_SEMANTICS == DEPTH || TEXTURE_SEMANTICS == SHADOW
#include regen.filter.sampling.writeDepth
#endif
#ifdef DRAW_BORDERS
#include regen.filter.sampling.drawEdges
#endif
#define NUM_SAMPLES ${TEX_NUM_SAMPLES${TEX_ID_inputTexture}}
#if NUM_SAMPLES > 1
#define HAS_INPUT_MULTISAMPLE
#endif

#ifdef HAS_INPUT_MULTISAMPLE
vec4 coverageAverage_MS(vec2 uv) {
    ivec2 msaaCoord = ivec2(uv * vec2(
        ${TEX_WIDTH${TEX_ID_inputTexture}},
        ${TEX_HEIGHT${TEX_ID_inputTexture}}
    ));
    vec3 color = vec3(0.0);
    float coverageSum = 0.0, coverage;
    vec4 x;
    // Accumulate only samples that actually contributed (alpha > threshold)
#for SAMPLE_I to NUM_SAMPLES
    x = texelFetch(in_inputTexture, msaaCoord, ${SAMPLE_I});
    #ifdef HAS_alphaThreshold
    coverage = step(in_alphaThreshold, x.a);
    #else
    coverage = step(0.1, x.a);
    #endif
    color += coverage*x.rgb;
    coverageSum += coverage;
#endfor
    color /= max(coverageSum, 1.0);
    return vec4(color, coverageSum / float(NUM_SAMPLES));
}
#endif

void main() {
    vec2 uv = gl_FragCoord.xy*in_inverseViewport;
#ifdef HAS_uRange
    if (uv.x < in_uRange.x || uv.x > in_uRange.y) {
        out_color = vec4(0);
        return;
    }
#endif
#ifdef HAS_vRange
    if (uv.y < in_vRange.x || uv.y > in_vRange.y) {
        out_color = vec4(0);
        return;
    }
#endif
#if TEXTURE_SEMANTICS == DEPTH
    #ifdef HAS_INPUT_MULTISAMPLE
    writeDepth(coverageAverage_MS(computeTexco(uv)).r);
    #else
    writeDepth(texture(in_inputTexture, computeTexco(uv)).r);
    #endif
#elif TEXTURE_SEMANTICS == SHADOW
    writeDepth(texture(in_inputTexture, vec3(computeTexco(uv),1.0)));
#else
    #ifdef HAS_INPUT_MULTISAMPLE
    out_color = coverageAverage_MS(computeTexco(uv));
    #else
    out_color = texture(in_inputTexture, computeTexco(uv));
    #endif
#endif
#ifdef DRAW_BORDERS
    drawEdges(uv);
#endif
}

----------------------------
----------------------------
-- color.vs
#include regen.filter.sampling.vs
-- color.gs
#include regen.filter.sampling.gs
-- color.fs
#include regen.filter.sampling.fs

----------------------------
----------------------------
-- depth.vs
#define TEXTURE_SEMANTICS DEPTH
#include regen.filter.sampling.vs
-- depth.gs
#define TEXTURE_SEMANTICS DEPTH
#include regen.filter.sampling.gs
-- depth.fs
#define TEXTURE_SEMANTICS DEPTH
#include regen.filter.sampling.fs

----------------------------
----------------------------
-- shadow.vs
#define TEXTURE_SEMANTICS SHADOW
#include regen.filter.sampling.vs
-- shadow.gs
#define TEXTURE_SEMANTICS SHADOW
#include regen.filter.sampling.gs
-- shadow.fs
#define TEXTURE_SEMANTICS SHADOW
#include regen.filter.sampling.fs

----------------------------
----------------------------
-- array.vs
#include regen.filter.sampling.vs
-- array.fs
#include regen.states.camera.defines
#include regen.filter.sampling.io
#if TEXTURE_SEMANTICS == DEPTH || TEXTURE_SEMANTICS == SHADOW
#include regen.filter.sampling.writeDepth
#endif
#ifdef DRAW_BORDERS
#include regen.filter.sampling.drawEdges
#endif

void main() {
    #define2 TEX_ID ${TEX_ID_inputTexture}
    int numElements = ${TEX_DEPTH${TEX_ID}};
    uint numRows = uint(sqrt(numElements));
    uint numCols = uint(ceil(float(numElements)/float(numRows)));
    float size    = in_viewport.x/numCols;
    float offsetY = 0.5*(float(in_viewport.y) - size*float(numRows));

    if(gl_FragCoord.y < offsetY || gl_FragCoord.y > in_viewport.y - offsetY) {
        out_color = vec4(0);
    }
    else {
        vec2 uv = vec2(
            mod(gl_FragCoord.x, size)/size,
            mod(gl_FragCoord.y - offsetY, size)/size);
        uint idx_x = uint(gl_FragCoord.x/size);
        uint idx_y = uint((gl_FragCoord.y - offsetY)/size);
        uint arrayIndex = idx_x + idx_y*numCols;
        if(arrayIndex >= numElements) {
            out_color = vec4(0);
        } else {
#if TEXTURE_SEMANTICS == DEPTH
            writeDepth(texture(in_inputTexture, vec3(uv.x, uv.y, arrayIndex)).x);
#elif TEXTURE_SEMANTICS == SHADOW
            writeDepth(texture(in_inputTexture, vec4(uv.x, uv.y, arrayIndex, 1.0)));
#else
            out_color = texture(in_inputTexture, vec3(uv.x, uv.y, arrayIndex));
#endif
        }
#ifdef DRAW_BORDERS
        drawEdges(uv);
#endif
    }
}

----------------------------
----------------------------
-- array.color.vs
#include regen.filter.sampling.array.vs
-- array.color.fs
#include regen.filter.sampling.array.fs

----------------------------
----------------------------
-- array.depth.vs
#define TEXTURE_SEMANTICS DEPTH
#include regen.filter.sampling.array.vs
-- array.depth.fs
#define TEXTURE_SEMANTICS DEPTH
#include regen.filter.sampling.array.fs

----------------------------
----------------------------
-- array.shadow.vs
#define TEXTURE_SEMANTICS SHADOW
#include regen.filter.sampling.array.vs
-- array.shadow.fs
#define TEXTURE_SEMANTICS SHADOW
#include regen.filter.sampling.array.fs


----------------------------
----------------------------
-- cube.vs
#include regen.filter.sampling.vs
-- cube.fs
#include regen.states.camera.defines
#include regen.filter.sampling.io
#if TEXTURE_SEMANTICS == DEPTH || TEXTURE_SEMANTICS == SHADOW
#include regen.filter.sampling.writeDepth
#endif
#ifdef DRAW_BORDERS
#include regen.filter.sampling.drawEdges
#endif

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
#if TEXTURE_SEMANTICS == DEPTH
        writeDepth(texture(in_inputTexture, dir).x);
#elif TEXTURE_SEMANTICS == SHADOW
        writeDepth(texture(in_inputTexture, vec4(dir, 1.0)));
#else
        out_color = texture(in_inputTexture, dir);
#endif
#ifdef DRAW_BORDERS
        drawEdges(vec2(texcoX,texcoY));
#endif
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
#if TEXTURE_SEMANTICS == DEPTH
        writeDepth(texture(in_inputTexture, dir));
#elif TEXTURE_SEMANTICS == SHADOW
        writeDepth(texture(in_inputTexture, vec4(dir, 1.0)));
#else
        out_color = texture(in_inputTexture, dir);
#endif
#ifdef DRAW_BORDERS
        drawEdges(vec2(texcoX,texcoY));
#endif
    }
    else {
        out_color = vec4(0);
    }
}

----------------------------
----------------------------
-- cube.color.vs
#include regen.filter.sampling.cube.vs
-- cube.color.fs
#include regen.filter.sampling.cube.fs

----------------------------
----------------------------
-- cube.depth.vs
#define TEXTURE_SEMANTICS DEPTH
#include regen.filter.sampling.cube.vs
-- cube.depth.fs
#define TEXTURE_SEMANTICS DEPTH
#include regen.filter.sampling.cube.fs

----------------------------
----------------------------
-- cube.shadow.vs
#define TEXTURE_SEMANTICS SHADOW
#include regen.filter.sampling.cube.vs
-- cube.shadow.fs
#define TEXTURE_SEMANTICS SHADOW
#include regen.filter.sampling.cube.fs

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
#include regen.filter.sampling.downsample.depth.average.2x2

void main()
{
    vecTexco texCoord = computeTexco(gl_FragCoord.xy*in_inverseViewport);
    gl_FragDepth = downsample(texCoord, in_inputTexture, in_inverseViewport);
}

-- downsample.depth.average.2x2
#ifndef downsample_depth_average_2x2_Included
#define2 downsample_depth_average_2x2_Included
float downsample(vec2 texCoord, sampler2D depthTexture, vec2 texelSize) {
    float depth = 0.0;
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            depth += texture(depthTexture, texCoord + offset).r;
        }
    }
    return depth / 4.0;
}

float downsample(vec3 texCoord, sampler2DArray depthTexture, vec2 texelSize) {
    float depth = 0.0;
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            depth += texture(depthTexture, texCoord + vec3(offset, 0)).r;
        }
    }
    return depth / 4.0;
}

float downsample(vec3 texCoord, samplerCube depthTexture, vec2 texelSize) {
    // TODO: add filtering for cube textures
    return texture(depthTexture, texCoord).r;
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
-- x2.vs
#include regen.filter.sampling.vs
-- x2.fs
#include regen.states.camera.defines

out vec4 out_color;

uniform vec2 in_inverseViewport;
uniform sampler2D in_inputTexture1;
uniform sampler2D in_inputTexture2;

void main()
{
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;

    if (texco_2D.y > 0.25 && texco_2D.y < 0.75) {
        if (texco_2D.x > 0.5) {
            out_color = texture(in_inputTexture1, (texco_2D-vec2(0.5,0.25))*2.0);
        } else {
            out_color = texture(in_inputTexture2, (texco_2D-vec2(0.0,0.25))*2.0);
        }
    } else {
        out_color = vec4(0.0);
    }
}

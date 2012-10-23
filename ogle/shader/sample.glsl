-- vs
#include utility.vs.ortho

-- identity
#include utility.fs.ortho.header
uniform samplerTex quantity;

out vec4 output;

void main() {
    output = texture(quantity, in_texco);
}

-- coarseToFine
#include utility.fs.ortho.header

uniform samplerTex quantityCoarse;
uniform samplerTex quantityCoarse0;
vecTex quantitySizeCoarse;

uniform samplerTex quantityFine0;
vecTex quantitySizeFine;

uniform float alpha;

out vec4 output;

void main() {
    vec4 valCoarse = texture(quantityCoarse, in_texco);
    vec4 valCoarse0 = texture(quantityCoarse0, in_texco);
    vec4 valFine0 = texture(quantityFine0, in_texco);
    output = alpha*valCoarse +
        (1.0-alpha)*(valFine0 + valCoarse - valCoarse0);
}

-- removeLines
#include utility.fs.ortho.header

uniform samplerTex quantity;
uniform vec4 removeColor;

out vec4 output;

void main() {
    ivecTex pos = ifragCoord();
    vec4 col = texelFetchOffset(quantity, pos, 0, ivec2(0,0));
    int count = 0;
    if(distance(texelFetchOffset(quantity, pos, 0, ivec2(0,1)).rgb,col.rgb)<0.0001) {
        count += 1;
    }
    if(distance(texelFetchOffset(quantity, pos, 0, ivec2(0,-1)).rgb,col.rgb)<0.0001) {
        count += 1;
    }
    if(distance(texelFetchOffset(quantity, pos, 0, ivec2(1,0)).rgb,col.rgb)<0.0001) {
        count += 1;
    }
    if(distance(texelFetchOffset(quantity, pos, 0, ivec2(-1,0)).rgb,col.rgb)<0.0001) {
        count += 1;
    }
    if(count>2) {
        output = col;
    } else {
        output = removeColor;
    }
}

-- smooth
#include utility.fs.ortho.header

uniform samplerTex quantity;

out vec4 output;

void main() {
    ivecTex pos = ifragCoord();
    output += texelFetchOffset(quantity, pos, 0, ivec2( 0,  1));
    output += texelFetchOffset(quantity, pos, 0, ivec2( 0, -1));
    output += texelFetchOffset(quantity, pos, 0, ivec2( 1,  0));
    output += texelFetchOffset(quantity, pos, 0, ivec2(-1,  0));
    output /= 4.0;
}

-- scalar
#include utility.fs.ortho.header

uniform samplerTex quantity;
uniform float texelFactor;
uniform vec3 colorPositive;
uniform vec3 colorNegative;

out vec4 output;

void main() {
    float x = texelFactor*texture(quantity,in_texco).r;
    if(x>0.0) {
        output = vec4(colorPositive, x);
    } else {
        output = vec4(colorNegative, -x);
    }
}

-- levelSet
#include utility.fs.ortho.header

uniform samplerTex quantity;
uniform float texelFactor;
uniform vec3 colorPositive;
uniform vec3 colorNegative;

out vec4 output;

void main() {
    float x = texelFactor*texture(quantity,in_texco).r;
    if(x>0.0) {
        output = vec4(colorNegative, 1.0f);
    } else {
        output = vec4(colorPositive, 0.0f);
    }
}

-- rgb
#include utility.fs.ortho.header

uniform samplerTex quantity;
uniform float texelFactor;

out vec4 output;

void main() {
    output.rgb = texelFactor*texture(quantity,in_texco).rgb;
    output.a = min(1.0, length(output.rgb));
}

-- velocity
#include utility.fs.ortho.header

uniform samplerTex quantity;
uniform float texelFactor;

out vec4 output;

void main() {
    output.rgb = texelFactor*texture(quantity,in_texco).rgb;
    output.a = min(1.0, length(output.rgb));
    output.b = 0.0;
    if(output.r < 0.0) output.b += -0.5*output.r;
    if(output.g < 0.0) output.b += -0.5*output.g;
}

-- fire
#include utility.fs.ortho.header

uniform samplerTex quantity;
uniform sampler1D pattern;
uniform float texelFactor;
uniform float fireAlphaMultiplier;
uniform float fireWeight;
uniform float smokeColorMultiplier;
uniform float smokeAlphaMultiplier;
uniform int rednessFactor;
uniform vec3 smokeColor;

out vec4 output;

void main() {
    const float threshold = 1.4;
    const float maxValue = 5;

    float s = texture(quantity,in_texco).r * texelFactor;
    s = clamp(s,0,maxValue);

    if( s > threshold ) { //render fire
        float lookUpVal = ( (s-threshold)/(maxValue-threshold) );
        lookUpVal = 1.0 - pow(lookUpVal, rednessFactor);
        lookUpVal = clamp(lookUpVal,0,1);
        vec3 interpColor = texture(pattern, (1.0-lookUpVal)).rgb;
        vec4 tmp = vec4(interpColor,1);
        float mult = (s-threshold);
        output.rgb = fireWeight*tmp.rgb;
        output.a = min(1.0, fireWeight*mult*mult*fireAlphaMultiplier + 0.5);
    } else { // render smoke
        output.rgb = vec3(fireWeight*s);
        output.a = min(1.0, output.r*smokeAlphaMultiplier);
        output.rgb = output.a * output.rrr * smokeColor * smokeColorMultiplier;
    }
}


-- tbo.float
#ifndef regen_tboRead_float_defined_
#define2 regen_tboRead_float_defined_
#define tboRead_float(tbo, i) texelFetch(tbo, i).r
#endif // regen_tboRead_float_defined_

-- tbo.int
#ifndef regen_tboRead_int_defined_
#define2 regen_tboRead_int_defined_
#define tboRead_int(tbo, i) texelFetch(tbo, i).r
#endif // regen_tboRead_int_defined_

-- tbo.uint
#ifndef regen_tboRead_uint_defined_
#define2 regen_tboRead_uint_defined_
#define tboRead_uint(tbo, i) texelFetch(tbo, i).r
#endif // regen_tboRead_uint_defined_

-- tbo.vec2
#ifndef regen_tboRead_vec2_defined_
#define2 regen_tboRead_vec2_defined_
#define tboRead_vec2(tbo, i) texelFetch(tbo, i).rg
#endif // regen_tboRead_vec2_defined_

-- tbo.ivec2
#ifndef regen_tboRead_ivec2_defined_
#define2 regen_tboRead_ivec2_defined_
#define tboRead_ivec2(tbo, i) ivec2(texelFetch(tbo, i).rg)
#endif // regen_tboRead_ivec2_defined_

-- tbo.uvec2
#ifndef regen_tboRead_uvec2_defined_
#define2 regen_tboRead_uvec2_defined_
#define tboRead_uvec2(tbo, i) uvec2(texelFetch(tbo, i).rg)
#endif // regen_tboRead_uvec2_defined_

-- tbo.vec3
#ifndef regen_tboRead_vec3_defined_
#define2 regen_tboRead_vec3_defined_
#define tboRead_vec3(tbo, i) texelFetch(tbo, i).rgb
#endif // regen_tboRead_vec3_defined_

-- tbo.ivec3
#ifndef regen_tboRead_ivec3_defined_
#define2 regen_tboRead_ivec3_defined_
#define tboRead_ivec3(tbo, i) ivec3(texelFetch(tbo, i).rgb)
#endif // regen_tboRead_ivec3_defined_

-- tbo.uvec3
#ifndef regen_tboRead_uvec3_defined_
#define2 regen_tboRead_uvec3_defined_
#define tboRead_uvec3(tbo, i) uvec3(texelFetch(tbo, i).rgb)
#endif // regen_tboRead_uvec3_defined_

-- tbo.vec4
#ifndef regen_tboRead_vec4_defined_
#define2 regen_tboRead_vec4_defined_
#define tboRead_vec4(tbo, i) texelFetch(tbo, i).rgba
#endif // regen_tboRead_vec4_defined_

-- tbo.ivec4
#ifndef regen_tboRead_ivec4_defined_
#define2 regen_tboRead_ivec4_defined_
#define tboRead_ivec4(tbo, i) ivec4(texelFetch(tbo, i).rgba)
#endif // regen_tboRead_ivec4_defined_

-- tbo.uvec4
#ifndef regen_tboRead_uvec4_defined_
#define2 regen_tboRead_uvec4_defined_
#define tboRead_uvec4(tbo, i) uvec4(texelFetch(tbo, i).rgba)
#endif // regen_tboRead_uvec4_defined_

-- tbo.mat3
#ifndef regen_tboRead_mat3_defined_
#define2 regen_tboRead_mat3_defined_
mat3 tboRead_mat3(samplerBuffer tbo, int objectIndex) {
    int i = objectIndex * 3;
    return mat3(
        texelFetch(tbo, i).rgb,
        texelFetch(tbo, i+1).rgb,
        texelFetch(tbo, i+2).rgb);
}
#endif // regen_tboRead_mat3_defined_

-- tbo.mat4
#ifndef regen_tboRead_mat4_defined_
#define2 regen_tboRead_mat4_defined_
mat4 tboRead_mat4(samplerBuffer tbo, int objectIndex) {
    int i = objectIndex * 4;
    return mat4(
        texelFetch(tbo, i).rgba,
        texelFetch(tbo, i+1).rgba,
        texelFetch(tbo, i+2).rgba,
        texelFetch(tbo, i+3).rgba);
}
#endif // regen_tboRead_mat4_defined_

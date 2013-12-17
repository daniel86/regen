-- vs
layout(location = 0) in vec4 in_moonPosition;
layout(location = 1) in vec3 in_moonColor;

out vec3 out_sunToMoon;
out vec3 out_moonColor;
out float out_moonIndex;
out float out_moonSize;

uniform vec3 in_sunDir;
uniform float in_sunDistance;

void main() {
    vec3 moonDirection = normalize(in_moonPosition.xyz);
    out_moonColor = in_moonColor;
    out_moonIndex = float(gl_VertexID);
    out_moonSize = in_moonPosition.w;
    out_sunToMoon = normalize(
        in_sunDir*in_sunDistance + in_moonPosition.xyz);
    gl_Position = vec4(moonDirection, 1.0);
}

-- gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=12) out;

in float in_moonSize[1];
in float in_moonIndex[1];
in vec3 in_sunToMoon[1];
in vec3 in_moonColor[1];

out vec3 out_pos;
out vec2 out_spriteTexco;
flat out float out_moonIndex;
flat out vec3 out_sunToMoon;
flat out vec3 out_moonColor;

// look at matrices for each cube face
uniform mat4 in_mvpMatrices[6];

#include regen.models.sprite.getSpritePoints
#include regen.models.sprite.getSpriteLayer

void main() {
    vec3 pos = gl_PositionIn[0].xyz;
    vec3 quadPos[4] = getSpritePoints(pos, vec2(in_moonSize[0]));
    int quadLayer[3] = getSpriteLayer(pos);

    out_moonIndex = in_moonIndex[0];
    out_sunToMoon = in_sunToMoon[0];
    out_moonColor = in_moonColor[0];
    // emit geometry to cube layers
    for(int i=0; i<3; ++i) {
        gl_Layer = quadLayer[i];
        mat4 mvp = in_mvpMatrices[gl_Layer];

        out_spriteTexco = vec2(1.0,0.0);
        out_pos = quadPos[0];
        gl_Position = mvp*vec4(out_pos,1.0);
        EmitVertex();
        
        out_spriteTexco = vec2(1.0,1.0);
        out_pos = quadPos[1];
        gl_Position = mvp*vec4(out_pos,1.0);
        EmitVertex();
        
        out_spriteTexco = vec2(0.0,0.0);
        out_pos = quadPos[2];
        gl_Position = mvp*vec4(out_pos,1.0);
        EmitVertex();
        
        out_spriteTexco = vec2(0.0,1.0);
        out_pos = quadPos[3];
        gl_Position = mvp*vec4(out_pos,1.0);
        EmitVertex();
        EndPrimitive();
    }
}

-- fs
out vec4 out_color;

in vec2 in_spriteTexco;
in vec3 in_pos;
flat in float in_moonIndex;
flat in vec3 in_sunToMoon;
flat in vec3 in_moonColor;

uniform sampler2DArray moonColorTexture;

#include regen.sky.utility.all

vec3 fakeSphereNormal(vec2 texco) {
    vec2 x = texco*2.0 - vec2(1.0);
    return vec3(x, sqrt(1.0 - dot(x,x)));
}

void main() {
    vec3 eyedir = normalize(in_pos);
    float eyeExtinction = getEyeExtinction(eyedir.xyz);
    
    vec2 texco = vec2(in_spriteTexco.x, 1.0 - in_spriteTexco.y);
    vec3 moonNormal = fakeSphereNormal(texco);
    float nDotL = dot(-moonNormal, in_sunToMoon);
    float blendFactor = nDotL;
    blendFactor *= eyeExtinction;

    vec4 moonColor = texture(moonColorTexture, vec3(texco,in_moonIndex));
    out_color.rgb = in_moonColor*moonColor.rgb*blendFactor;
    out_color.a = moonColor.a;
}

-- vs
#version 150

in vec3 in_pos;
out vec2 out_texco;
void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- fs
#version 150

in vec2 in_texco;
out vec4 output;

uniform sampler2D in_sceneTexture;

void main() {
  output = texture(in_sceneTexture, in_texco);
}


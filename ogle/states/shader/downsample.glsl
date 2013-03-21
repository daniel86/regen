
-- vs
#include sampling.vs

-- gs
#include sampling.gs

-- fs
#include sampling.fsHeader
out vec4 out_color;
void main() {
  out_color = texture(in_inputTexture, in_texco);
}


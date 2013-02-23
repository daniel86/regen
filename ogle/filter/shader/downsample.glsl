
-- vs
#include sampling.vs

-- gs
#include sampling.gs

-- fs
#include sampling.fsHeader
out vec4 output;
void main() {
  output = texture(in_inputTexture, in_texco);
}


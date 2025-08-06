
-- vs
#include regen.models.sky-box.vs
-- tcs
#include regen.models.sky-box.tcs
-- tes
#include regen.models.sky-box.tes
-- gs
#include regen.models.sky-box.gs
-- fs
out vec4 out_color;
void main(void) {
    out_color = vec4(0.0, 0.0, 0.0, 1.0);
}

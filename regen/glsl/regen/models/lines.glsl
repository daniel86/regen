
--------------------------------
--------------------------------
----- Rendering lines in 3D
--------------------------------
--------------------------------
-- vs
#include regen.models.mesh.vs
-- fs
out vec4 out_color;

const vec3 in_lineColor = vec3(1.0);

#include regen.models.mesh.writeOutput

void main() {
    out_color = vec4(in_lineColor,1.0);
}


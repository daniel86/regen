
--------------------------------
--------------------------------
----- A sprite that fakes a sphere by calculating sphere normals and
----- discarding fragments outside the sphere radius.
--------------------------------
--------------------------------
-- vs
#include regen.models.mesh.defines
#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif

in vec3 in_pos;

in float in_sphereRadius;
out float out_sphereRadius;

#define HANDLE_IO(i)

void main() {
#ifdef HAS_modelMatrix
  gl_Position = in_modelMatrix * vec4(in_pos,1.0);
#else
  gl_Position = vec4(in_pos,1.0);
#endif
  out_sphereRadius = in_sphereRadius;
  HANDLE_IO(gl_VertexID);
}

-- gs
#include regen.models.mesh.defines
#extension GL_EXT_geometry_shader4 : enable

// TODO: support layered rendering, HANDLE_IO

layout(points) in;
layout(triangle_strip, max_vertices=8) out;

#include regen.states.camera.input
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

out vec3 out_posWorld;
out vec3 out_posEye;
out vec2 out_spriteTexco;

in float in_sphereRadius[1];
#ifdef DEPTH_CORRECT
out float in_sphereRadius;
#endif

#include regen.math.computeSpritePoints

void main() {
  vec3 centerWorld = gl_PositionIn[0].xyz;
  vec4 centerEye = transformWorldToEye(centerWorld,0);
  vec3 quadPos[4] = computeSpritePoints(
      centerEye.xyz, vec2(in_sphereRadius[0]), vec3(0.0,1.0,0.0));
#ifdef DEPTH_CORRECT
  out_sphereRadius = in_sphereRadius[0];
#endif

  out_spriteTexco = vec2(1.0,0.0);
  vec4 posEye = vec4(quadPos[0],1.0);
  out_posEye = posEye.xyz;
  out_posWorld = (__VIEW_INV__ * posEye).xyz;
  gl_Position = transformEyeToScreen(posEye,0);
  EmitVertex();
    
  out_spriteTexco = vec2(1.0,1.0);
  posEye = vec4(quadPos[1],1.0);
  out_posEye = posEye.xyz;
  out_posWorld = (__VIEW_INV__ * posEye).xyz;
  gl_Position = transformEyeToScreen(posEye);
  EmitVertex();
        
  out_spriteTexco = vec2(0.0,0.0);
  posEye = vec4(quadPos[2],1.0);
  out_posEye = posEye.xyz;
  out_posWorld = (__VIEW_INV__ * posEye).xyz;
  gl_Position = transformEyeToScreen(posEye);
  EmitVertex();
        
  out_spriteTexco = vec2(0.0,1.0);
  posEye = vec4(quadPos[3],1.0);
  out_posEye = posEye.xyz;
  out_posWorld = (__VIEW_INV__ * posEye).xyz;
  gl_Position = transformEyeToScreen(posEye);
  EmitVertex();
  EndPrimitive();
}

-- fs
#include regen.models.mesh.defines
#include regen.states.textures.defines
#include regen.models.mesh.fs-outputs

in vec3 in_posWorld;
in vec3 in_posEye;
in vec2 in_spriteTexco;
#ifdef DEPTH_CORRECT
in float in_sphereRadius;
#endif

#ifdef HAS_col
uniform vec4 in_col;
#endif

#include regen.states.camera.input
#include regen.states.material.input
#include regen.states.textures.input

#ifdef DEPTH_CORRECT
#include regen.states.camera.depthCorrection
#endif
#include regen.states.textures.mapToFragment
#include regen.states.textures.mapToLight
#include regen.models.mesh.writeOutput

void main()
{
  vec2 spriteTexco = in_spriteTexco*2.0 - vec2(1.0);
  float texcoMagnitude = length(spriteTexco);
  // 0.99 because color at edges was wrong.
  if(texcoMagnitude>=0.99) discard;
  
  vec3 normal = vec3(spriteTexco, sqrt(1.0 - dot(spriteTexco,spriteTexco)));
  vec4 norWorld = normalize(__VIEW_INV__ * vec4(normal,0.0));
#ifdef DEPTH_CORRECT
  // Note that early depth test is disabled then and this can have
  // bad consequences for performance.
  depthCorrection(in_sphereRadius*(1.0-texcoMagnitude));
#endif
#ifdef HAS_col
  vec4 color = in_col;
#else
  vec4 color = vec4(1.0);
#endif
  textureMappingFragment(in_posWorld,color,norWorld.xyz);
  writeOutput(in_posWorld,norWorld.xyz,color);
}


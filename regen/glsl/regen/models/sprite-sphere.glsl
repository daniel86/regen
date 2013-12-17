
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
-- tcs
#include regen.models.mesh.tcs
-- tes
#include regen.models.mesh.tes
-- gs
#include regen.models.mesh.defines
#extension GL_EXT_geometry_shader4 : enable
#define2 __MAX_VERTICES__ ${${RENDER_LAYER}*4}

layout(points) in;
layout(triangle_strip, max_vertices=${__MAX_VERTICES__}) out;

#include regen.states.camera.input
#include regen.states.camera.transformEyeToScreen
#include regen.states.camera.transformEyeToWorld
#include regen.states.camera.transformWorldToEye

out vec3 out_posWorld;
out vec3 out_posEye;
out vec2 out_spriteTexco;

in float in_sphereRadius[1];
#ifdef DEPTH_CORRECT
out float in_sphereRadius;
#endif

#include regen.math.computeSpritePoints

#define HANDLE_IO(i)

void emitVertex(vec4 posEye, mat4 viewInv, mat4 proj) {
  out_posEye = posEye.xyz;
  out_posWorld = transformEyeToWorld(posEye,viewInv).xyz;
  gl_Position = transformEyeToScreen(posEye,proj);
  EmitVertex();
  HANDLE_IO(0);
  
  EmitVertex();
}

void emitSpriteSphere(mat4 view, mat4 viewInv, mat4 proj, int layer) {
  vec4 centerEye = transformWorldToEye(gl_PositionIn[0].xyz,view);
  vec3 quadPos[4] = computeSpritePoints(
      centerEye.xyz, vec2(in_sphereRadius[0]), vec3(0.0,1.0,0.0));

  out_spriteTexco = vec2(1.0,0.0);
  emitVertex(vec4(quadPos[0],1.0), viewInv, proj);
  out_spriteTexco = vec2(1.0,1.0);
  emitVertex(vec4(quadPos[1],1.0), viewInv, proj);  
  out_spriteTexco = vec2(0.0,0.0);
  emitVertex(vec4(quadPos[2],1.0), viewInv, proj); 
  out_spriteTexco = vec2(0.0,1.0);
  emitVertex(vec4(quadPos[3],1.0), viewInv, proj);
  EndPrimitive();
}

void main() {
  mat4 view, viewInv, proj;
#if RENDER_TARGET == CUBE
  proj = in_projectionMatrix;
#elif RENDER_TARGET == 2D_ARRAY
  viewInv = in_viewMatrixInverse;
  view = in_viewMatrix;
#endif
#ifdef DEPTH_CORRECT
  out_sphereRadius = in_sphereRadius[0];
#endif
  
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
  gl_Layer = ${LAYER};
  out_layer = ${LAYER};
#if RENDER_TARGET == CUBE
  viewInv = in_viewMatrixInverse[${LAYER}];
  view = in_viewMatrix[${LAYER}];
#elif RENDER_TARGET == 2D_ARRAY
  proj = in_projectionMatrix[${LAYER}];
#endif
  emitSpriteSphere(viewInv, proj, ${LAYER});
#endif
#endfor
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
  // Note that early depth test is disabled with DEPTH_CORRECT defined and this can have
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

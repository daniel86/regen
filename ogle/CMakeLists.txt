
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )

include_directories(/usr/include/freetype2)

# build static libraries for some components
add_subdirectory(algebra)
add_subdirectory(av)
add_subdirectory(gl-types)
add_subdirectory(utility)
add_subdirectory(font)

add_subdirectory(animations)
add_subdirectory(states)
add_subdirectory(textures)
add_subdirectory(shader)
add_subdirectory(models)
add_subdirectory(render-tree)
add_subdirectory(render-passes)

add_library(ogle SHARED
            states/state.cpp
            states/render-state.cpp
            states/attribute-state.cpp
            states/vbo-state.cpp
            states/fbo-state.cpp
            states/texture-state.cpp
            states/shader-state.cpp
            states/uniform-state.cpp
            states/light-state.cpp
            states/material-state.cpp
            states/camera-state.cpp
            states/projection-state.cpp
            states/model-transformation-state.cpp
            states/bones-state.cpp
            states/fog-state.cpp
            states/tesselation-state.cpp
            states/blit-state.cpp
            animations/animation.cpp
            animations/animation-buffer.cpp
            animations/animation-manager.cpp
            animations/camera-manipulator.cpp
            animations/vbo-animation.cpp
            animations/vbo-morph-animation.cpp
            animations/vbo-wave-animation.cpp
            animations/bone.cpp
            shader/shader-configuration.cpp
            shader/shader-function.cpp
            shader/shader-manager.cpp
            shader/shader-generator.cpp
            shader/normal-mapping.cpp
            shader/light-shader.cpp
            shader/blending-shader.cpp
            shader/filter-shader.cpp
            shader/utility-shader.cpp
            shader/debugging-shader.cpp
            shader/raycast-shader.cpp
            shader/tesselation-shader.cpp
            textures/cube-image-texture.cpp
            textures/video-texture.cpp
            textures/image-texture.cpp
            textures/fire-texture.cpp
            textures/texel-transfer.cpp
            models/quad.cpp
            models/cube.cpp
            models/sphere.cpp
            models/volume.cpp
            models/text.cpp
            models/sky-box.cpp
            models/terrain.cpp
            models/assimp-loader.cpp
            models/assimp-mesh.cpp
            render-tree/state-node.cpp
            render-tree/render-tree.cpp
            render-passes/anti-aliasing.cpp
            render-passes/debug-normal.cpp
            render-passes/depth-of-field.cpp
            render-passes/glowing.cpp
            render-passes/hdr.cpp
            render-passes/motion-blur.cpp
            render-passes/pixel-velocity.cpp
            )
TARGET_LINK_LIBRARIES( ogle  algebra )
TARGET_LINK_LIBRARIES( ogle  av )
TARGET_LINK_LIBRARIES( ogle  gl-types )
TARGET_LINK_LIBRARIES( ogle  utility )
TARGET_LINK_LIBRARIES( ogle  font )

install(
    FILES
        exceptions/io-exception.h
    DESTINATION include/ogle/exceptions
    )

install(
    TARGETS
        ogle
    LIBRARY DESTINATION lib/ogle
)

            #fluid/eulerian/fluid.cpp
            #fluid/eulerian/smoke.cpp
            #fluid/eulerian/fire.cpp
            #fluid/eulerian/liquid.cpp
            #fluid/eulerian/helper.cpp
            #fluid/eulerian/obstacles.cpp
            #fluid/eulerian/primitive.cpp
            #fluid/eulerian/stages/advect.cpp
            #fluid/eulerian/stages/buoyancy.cpp
            #fluid/eulerian/stages/diffusion.cpp
            #fluid/eulerian/stages/external-forces.cpp
            #fluid/eulerian/stages/extrapolate-velocity.cpp
            #fluid/eulerian/stages/pressure.cpp
            #fluid/eulerian/stages/redistance.cpp
            #fluid/eulerian/stages/vorticity.cpp

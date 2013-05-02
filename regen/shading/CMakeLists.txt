
set(SOURCES
        shading/ambient-occlusion.cpp
        shading/distance-fog.cpp
        shading/light-state.cpp
        shading/volumetric-fog.cpp
        shading/light-pass.cpp
        shading/shading-deferred.cpp
        shading/shading-direct.cpp
        shading/t-buffer.cpp
        shading/shadow-map.cpp
)

install(
    FILES
        ambient-occlusion.h
        distance-fog.h
        light-pass.h
        light-state.h
        shading-deferred.h
        shading-direct.h
        shadow-map.h
        t-buffer.h
        volumetric-fog.h
    DESTINATION ${HEADER_INSTALL_PATH}/shading
)
install(
    FILES
        shader/fog.glsl
        shader/shading.glsl
        shader/shadow_mapping.glsl
        shader/ssao.glsl
    DESTINATION ${SHADER_INSTALL_PATH}/shading/shader
)

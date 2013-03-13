
set(SOURCES
        shading/light-pass.cpp
        shading/ambient-occlusion.cpp
        shading/deferred.cpp
        shading/direct.cpp
        shading/post-processing.cpp
        shading/shadow-map.cpp
        shading/directional-shadow-map.cpp
        shading/spot-shadow-map.cpp
        shading/point-shadow-map.cpp
        shading/light-shafts.cpp
        shading/light-state.cpp
        shading/distance-fog.cpp
        shading/volumetric-fog.cpp
)

install(
    FILES
        light-pass.h
        ambient-occlusion.h
        deferred.h
        direct.h
        post-processing.h
        shadow-map.h
        directional-shadow-map.h
        spot-shadow-map.h
        point-shadow-map.h
        light-shafts.h
        light-state.h
        distance-fog.h
        volumetric-fog.h
    DESTINATION ${HEADER_INSTALL_PATH}/states
    )

install(
    FILES
        shader/shading.glsl
        shader/shadow-mapping.glsl
    DESTINATION ${SHADER_INSTALL_PATH}
    )


set(SOURCES
        shading/light-pass.cpp
        shading/ambient-light.cpp
        shading/ambient-occlusion.cpp
        shading/deferred.cpp
        shading/deferred-light.cpp
        shading/direct.cpp
        shading/directional-light.cpp
        shading/point-light.cpp
        shading/post-processing.cpp
        shading/spot-light.cpp
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
        ambient-light.h
        ambient-occlusion.h
        deferred.h
        deferred-light.h
        direct.h
        directional-light.h
        point-light.h
        post-processing.h
        spot-light.h
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


set(SOURCES
        shading/light-pass.cpp
        shading/ambient-occlusion.cpp
        shading/shading-deferred.cpp
        shading/shading-direct.cpp
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
        shading-deferred.h
        shading-direct.h
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

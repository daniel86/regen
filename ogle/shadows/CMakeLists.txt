
set(SOURCES
        shadows/shadow-map.cpp
        shadows/directional-shadow-map.cpp
        shadows/spot-shadow-map.cpp
        shadows/point-shadow-map.cpp
)

install(
    FILES
        shadow-map.h
        directional-shadow-map.h
        spot-shadow-map.h
        point-shadow-map.h
    DESTINATION ${HEADER_INSTALL_PATH}/shadows
    )

install(
    FILES
        shader/shadow-mapping.glsl
    DESTINATION ${SHADER_INSTALL_PATH}
    )
    
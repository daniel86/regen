
# list of all shader files in this directory
file(GLOB SHADER_FILES *.glsl)
# inst all all shader files
install(
    FILES ${SHADER_FILES}
    DESTINATION ${SHADER_INSTALL_PATH}/glsl/regen
)

add_subdirectory(filter)
add_subdirectory(gui)
add_subdirectory(models)
add_subdirectory(particles)
add_subdirectory(shading)
add_subdirectory(states)

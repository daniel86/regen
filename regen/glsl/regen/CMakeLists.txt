
# list of all shader files in this directory
file(GLOB SHADER_FILES *.glsl)
# inst all all shader files
install(
    FILES ${SHADER_FILES}
    DESTINATION ${SHADER_INSTALL_PATH}/glsl/regen
)

add_subdirectory(gui)
add_subdirectory(meshes)
add_subdirectory(particles)
add_subdirectory(post-passes)
add_subdirectory(shading)
add_subdirectory(sky)
add_subdirectory(utility)

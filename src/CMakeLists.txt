add_subdirectory(core)
add_subdirectory(graphics)
add_subdirectory(scene)
add_subdirectory(view)

# resources
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
configure_file(config.h.in "${PROJECT_SOURCE_DIR}/src/include/core/config.h")

target_include_directories(${PROJECT_NAME}
        PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/src/include"
)

cmake_minimum_required(VERSION 3.15)
project(JardimDasRunas CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(app
    main.cpp
    GLAD/src/glad.c
    stb_image/stb_image.cpp
)

target_include_directories(app PRIVATE
    GLAD/include
    glfw-3.4.bin.WIN64/include
    glm
    stb_image
)

target_link_libraries(app PRIVATE
    "${CMAKE_SOURCE_DIR}/glfw-3.4.bin.WIN64/lib-vc2022/glfw3dll.lib"
    opengl32
)

add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/glfw-3.4.bin.WIN64/lib-vc2022/glfw3.dll"
        "$<TARGET_FILE_DIR:app>/glfw3.dll"
        
)
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:app>/assets"
)

add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/_sprite_vs.glsl"
        "${CMAKE_SOURCE_DIR}/_sprite_fs.glsl"
        "${CMAKE_SOURCE_DIR}/_color_vs.glsl"
        "${CMAKE_SOURCE_DIR}/_color_fs.glsl"
        "${CMAKE_SOURCE_DIR}/map.txt"
        "$<TARGET_FILE_DIR:app>"
)

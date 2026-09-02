# ThorVG examples
if(NOT ${THORVG_ENABLE_BGFX_ENGINE})
    return()
endif()

add_executable(bgfx_basic bgfx_basic/main.cpp)
target_include_directories(bgfx_basic PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_link_libraries(bgfx_basic PRIVATE thorvg bgfx bx glfw)

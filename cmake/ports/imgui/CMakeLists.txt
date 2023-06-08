cmake_minimum_required(VERSION 3.16)
project(imgui CXX)

set(CMAKE_DEBUG_POSTFIX d)

if(APPLE)
    set(CMAKE_CXX_STANDARD 11)
    enable_language(OBJCXX)
endif()

add_library(${PROJECT_NAME} "")
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_sources(
    ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)

if(IMGUI_BUILD_DX11_BINDING)
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
endif()

if(IMGUI_BUILD_WIN32_BINDING)
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_win32.cpp)
endif()

if(IMGUI_FREETYPE)
    find_package(freetype CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC freetype)
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp)
    target_compile_definitions(${PROJECT_NAME} PUBLIC IMGUI_ENABLE_FREETYPE)
endif()

if(IMGUI_USE_WCHAR32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC IMGUI_USE_WCHAR32)
endif()

list(REMOVE_DUPLICATES BINDINGS_SOURCES)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}_target
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

foreach(BINDING_TARGET ${BINDING_TARGETS})
    install(
        TARGETS ${BINDING_TARGET}
        EXPORT ${PROJECT_NAME}_target
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endforeach()

if(NOT IMGUI_SKIP_HEADERS)
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imconfig.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_internal.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imstb_textedit.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imstb_rectpack.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imstb_truetype.h
        ${CMAKE_CURRENT_SOURCE_DIR}/misc/cpp/imgui_stdlib.h
        DESTINATION include
    )

    if(IMGUI_BUILD_DX11_BINDING)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_dx11.h DESTINATION include)
    endif()

    if(IMGUI_BUILD_WIN32_BINDING)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_win32.h DESTINATION include)
    endif()

    if(IMGUI_FREETYPE)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/misc/freetype/imgui_freetype.h DESTINATION include)
    endif()
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(imgui-config.cmake.in imgui-config.cmake INSTALL_DESTINATION share/imgui)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/imgui-config.cmake DESTINATION share/imgui)

install(
    EXPORT ${PROJECT_NAME}_target
    NAMESPACE ${PROJECT_NAME}::
    FILE ${PROJECT_NAME}-targets.cmake
    DESTINATION share/${PROJECT_NAME}
)

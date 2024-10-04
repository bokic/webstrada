cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(webstrada-tests LANGUAGES CXX)

find_package(LLVM REQUIRED)

if(BUILD_UNIT_TESTS)
    add_executable(webstrada-tests
        tests.cpp
        ../src/worker.cpp
        ../src/template_cache.cpp
    )

    if(WEBSTRADA_USE_PCH_IN_BUILD)
        target_precompile_headers(webstrada-tests PRIVATE
            ${CMAKE_SOURCE_DIR}/include/webstrada/pch.h
        )
    endif()
    target_link_libraries(webstrada-tests PRIVATE
        webstrada-core
        webstrada-compiler
        textparser
        gtest
        LLVM
        fcgi
    )
endif()

add_executable(WebStrada-cli
    WebStrada_cli.cpp
    ../src/template_cache.cpp
    ../src/worker.cpp
)

target_include_directories(WebStrada-cli PRIVATE
    ../include
)

if(WEBSTRADA_USE_PCH_IN_BUILD)
    target_precompile_headers(WebStrada-cli PRIVATE
        ${CMAKE_SOURCE_DIR}/include/webstrada/pch.h
    )
endif()

target_link_libraries(WebStrada-cli
    webstrada-core
    webstrada-compiler
    LLVM
    fcgi
)

cmake_minimum_required(VERSION 3.16)

project(summedareafont)

set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    SET(CMAKE_CXX_FLAGS "/nologo /DWIN32=1 /D_WINDOWS /W3 /GR /EHsc /MP /arch:AVX2 /GF /guard:cf- /wd4996 /wd4068 /wd5033 /w14388 /w14389 /w14101 /w14189 /std:c++17 /GS- /Gw /Qspectre- /fp:fast")
    SET(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MTd /ZI /Ob0 /Od /RTC1 /Gy")
    SET(CMAKE_C_FLAGS "/nologo /DWIN32=1 /D_WINDOWS /W3 /MP /arch:AVX2 /GF /guard:cf- /wd4996 /wd4068 /Qspectre-")
    SET(CMAKE_C_FLAGS_DEBUG "/D_DEBUG /MTd /ZI /Ob0 /Od /RTC1 /Gy")
    add_compile_options("/MP")
endif()

if (UNIX)
    add_definitions(-DUNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -fvisibility=hidden -Wno-unused-function -Wno-catch-value -Wno-sign-compare -Wno-int-in-bool-context")
	link_libraries(${CMAKE_THREAD_LIBS_INIT})
endif()

set(FILES
     main.cpp
     atlas.cpp
     atlas.h
	${CMAKE_SOURCE_DIR}/glew-2.1.0/src/glew.c
)

include_directories(${CMAKE_SOURCE_DIR}/glew-2.1.0/include ${CMAKE_SOURCE_DIR}/glfw/include)
add_definitions(-DGLEW_NO_GLU -DGLEW_STATIC)

find_package(OpenGL REQUIRED)

if(MSVC)
	add_executable(${PROJECT_NAME} WIN32 ${FILES})
	target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/glfw/lib-vc2022/glfw3_mt.lib)
	target_link_libraries(${PROJECT_NAME} ws2_32.lib iphlpapi.lib ${LIBRARIES} OpenGL::GL)
	set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
	add_executable(${PROJECT_NAME} ${FILES})
	find_package(glfw3 3.3 REQUIRED)
	target_link_libraries(${PROJECT_NAME} glfw OpenGL::GL ${CMAKE_DL_LIBS})
endif()

file(COPY fonts/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin/fonts)

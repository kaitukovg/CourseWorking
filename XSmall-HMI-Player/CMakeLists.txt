cmake_minimum_required(VERSION 3.15)
project(XSmall-HMI-Player VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Включаем FetchContent
include(FetchContent)

# ========== nlohmann/json (однозаголовочная версия) ==========
message(STATUS "Setting up nlohmann/json...")

# Скачиваем только заголовочный файл
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
    DOWNLOAD_NO_EXTRACT TRUE  # Не распаковываем архив
)

FetchContent_GetProperties(nlohmann_json)
if(NOT nlohmann_json_POPULATED)
    FetchContent_Populate(nlohmann_json)
    
    # Создаем папку include/nlohmann если её нет
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/nlohmann)
    
    # Копируем заголовочный файл
    file(COPY ${nlohmann_json_SOURCE_DIR}/json.hpp
         DESTINATION ${CMAKE_BINARY_DIR}/include/nlohmann)
    
    # Создаем интерфейсную цель для библиотеки
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE 
        ${CMAKE_BINARY_DIR}/include
    )
endif()

message(STATUS "nlohmann/json ready (single-header)")

# ========== SFML (СТАТИЧЕСКАЯ СБОРКА) ==========
message(STATUS "Downloading SFML...")

FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.1
    GIT_SHALLOW TRUE
)

# НАСТРОЙКИ ДЛЯ СТАТИЧЕСКОЙ СБОРКИ
set(SFML_STATIC_LIBRARIES ON CACHE BOOL "Build static libraries" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# Отключаем ненужные модули
set(SFML_BUILD_AUDIO OFF CACHE BOOL "Disable audio" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "Disable network" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "Disable docs" FORCE)

FetchContent_MakeAvailable(sfml)
message(STATUS "SFML downloaded (static)")

# ========== GOOGLE TEST ==========
message(STATUS "Downloading Google Test...")

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
message(STATUS "Google Test downloaded")

# ========== НАШ ПРОЕКТ ==========
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Директории с заголовками
include_directories(
    ${PROJECT_ROOT}/include
    ${CMAKE_BINARY_DIR}/include  # Добавляем для nlohmann/json
    ${sfml_SOURCE_DIR}/include
)

# Исходные файлы
set(SOURCES
    src/main.cpp
    src/VariableDatabase.cpp
    src/VisualObject.cpp
    src/Rectangle.cpp
    src/Text.cpp
    src/Line.cpp
    src/Polyline.cpp
    src/InputField.cpp
    src/Button.cpp
    src/Image.cpp
    src/HistoryGraph.cpp
    src/SceneFactory.cpp
    src/HmiPlayer.cpp
    src/JSONLoader.cpp
)

# Создаем исполняемый файл
add_executable(HMI_Player ${SOURCES})

# Добавляем определение для статической линковки
target_compile_definitions(HMI_Player PRIVATE SFML_STATIC)

# Подключаем библиотеки
target_link_libraries(HMI_Player PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    nlohmann_json  # Наша интерфейсная цель
)

# Для Windows нужно добавить системные библиотеки для статической линковки
if(WIN32)
    target_link_libraries(HMI_Player PRIVATE
        opengl32
        winmm
        gdi32
    )
endif()

# Копируем ресурсы
file(COPY ${PROJECT_ROOT}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${PROJECT_ROOT}/objects.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "Main app configured (static)")

# ========== ТЕСТЫ ==========
add_subdirectory(tests)

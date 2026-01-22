cmake_minimum_required(VERSION 3.14)
project(EntropyVisualizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- GLFW ---
add_subdirectory(external/glfw)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# ImGui
file(GLOB IMGUI_CORE_SRC
    external/imgui/*.cpp
)

list(REMOVE_ITEM IMGUI_CORE_SRC
    external/imgui/imgui_demo.cpp
)

set(IMGUI_BACKEND_SRC
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

# --- IMGUI ---
add_library(imgui STATIC ${IMGUI_CORE_SRC} ${IMGUI_BACKEND_SRC})
target_include_directories(imgui PUBLIC external/imgui)
target_link_libraries(imgui PUBLIC glfw)

# --- GLAD ---
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# --- ImGuiFileDialog ---
add_library(imguifiledialog STATIC external/ImGuiFileDialog/ImGuiFileDialog.cpp)
target_include_directories(imguifiledialog PUBLIC external/ImGuiFileDialog external/imgui)
target_link_libraries(imgui PUBLIC imguifiledialog)

# --- Core library ---
add_library(core STATIC src/core.cpp)
target_include_directories(core PUBLIC include)

# --- Cache library ---
add_library(cache STATIC src/cache.cpp)
target_include_directories(cache PUBLIC include)
target_link_libraries(cache PUBLIC core)

# --- UI library ---
add_library(ui STATIC src/ui.cpp)
target_include_directories(ui PUBLIC include external/imgui external/ImGuiFileDialog)
target_link_libraries(ui PUBLIC core imgui glad glfw imguifiledialog)

# --- App library ---
add_library(app STATIC src/app.cpp)
target_include_directories(app PUBLIC include ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(app PUBLIC core cache ui imgui glad glfw)

if(WIN32)
    add_executable(EntropyVisualizer WIN32 main.cpp)
else()
    add_executable(EntropyVisualizer main.cpp)
endif()
# --- Entropy Visualizer ---
target_include_directories(EntropyVisualizer PUBLIC include external/imgui external/ImGuiFileDialog)
target_link_libraries(EntropyVisualizer app)

if (WIN32)
    # Add icon resource
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/app_icon.rc")
    target_sources(EntropyVisualizer PRIVATE ${APP_ICON_RESOURCE})
    set_target_properties(EntropyVisualizer PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
)
endif()

# --- Platform-specific OpenGL linking for visualizer ---
if (WIN32)
    target_link_libraries(EntropyVisualizer opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(EntropyVisualizer OpenGL::GL dl pthread)
endif()

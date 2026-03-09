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

set(IMGUI_BACKEND_SRC
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

if(LINUX)
    find_package(CURL REQUIRED)
endif()

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
target_link_libraries(imguifiledialog PUBLIC imgui)

# --- ImGui_Gradient ---
add_subdirectory(external/imgui_gradient)
target_include_directories(imgui_gradient SYSTEM PRIVATE external/imgui)
target_compile_definitions(imgui_gradient PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

# --- Core library ---
add_library(core STATIC src/core.cpp)
target_include_directories(core PUBLIC include)

# --- Cache library ---
add_library(cache STATIC src/cache.cpp)
target_include_directories(cache PUBLIC include)
target_link_libraries(cache PUBLIC core)

# --- Hex Display Feature Manager library ---
add_library(hex_display_feature_manager STATIC src/hex_display_feature_manager.cpp)
target_include_directories(hex_display_feature_manager PUBLIC include external/imgui)

# --- UI library ---
add_library(ui STATIC src/ui.cpp)
target_include_directories(ui PUBLIC include external/imgui external/ImGuiFileDialog external/imgui_gradient)
target_link_libraries(ui PUBLIC core imgui glad glfw imguifiledialog imgui_gradient)

# --- App library ---
add_library(app STATIC src/app.cpp src/update.cpp)
target_include_directories(app PUBLIC include ${CMAKE_CURRENT_SOURCE_DIR})
if(LINUX)
    target_link_libraries(app PUBLIC core cache ui imgui glad glfw hex_display_feature_manager CURL::libcurl)
else()
    target_link_libraries(app PUBLIC core cache ui imgui glad glfw hex_display_feature_manager winhttp.lib)
endif()

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

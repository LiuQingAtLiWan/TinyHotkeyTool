cmake_minimum_required(VERSION 3.10)  # 设置 CMake 的最低版本
set(CMAKE_GENERATOR_PLATFORM Win32)   # 设置 win32平台

project(tinyShortCut)                  # 定义项目名称

# 设置编译器标准（可选，依据需求）
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 指定源文件
add_executable(tinyShortCut WIN32 shortcut.cpp)

# 如果需要链接 Windows API，请添加以下定义
target_link_libraries(tinyShortCut PRIVATE user32 gdi32)

cmake_minimum_required(VERSION 3.22)
project(LUNA LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui)

# add to your sources list
add_executable(luna_sama
  app/main.cpp
  ui/MainWindow.cpp ui/MainWindow.h
  ui/ModeMenu.cpp ui/ModeMenu.h
  ui/CharacterView.cpp ui/CharacterView.h
  ui/IOOverlay.cpp ui/IOOverlay.h
  core/BackendClient.h
  core/ModeManager.cpp core/ModeManager.h   # <--- NEW
)


target_link_libraries(luna_sama
  Qt6::Widgets Qt6::Gui
)

# (Optional) copy style.qss next to the binary for easy running from IDEs
add_custom_command(TARGET luna_sama POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:luna_sama>/app
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/app/style.qss
          $<TARGET_FILE_DIR:luna_sama>/app/style.qss
)

cmake_minimum_required(VERSION 3.22)
project(LUNA LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt auto-magic
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

# ---- Qt packages ----
# Add Network (for BackendClient) and Multimedia (for AudioPlayer)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Multimedia)

# ---- Sources ----
set(SOURCES
  # app
  app/main.cpp
   app/app.rc  

  # ui
  ui/MainWindow.cpp         ui/MainWindow.h
  ui/ModeMenu.cpp           ui/ModeMenu.h
  ui/CharacterView.cpp      ui/CharacterView.h
  ui/IOOverlay.cpp          ui/IOOverlay.h

  # core
  core/ModeManager.cpp      core/ModeManager.h
  core/BackendClient.cpp    core/BackendClient.h
  core/AudioPlayer.cpp      core/AudioPlayer.h
)

add_executable(luna_sama WIN32 ${SOURCES})

# On Windows, build as a GUI app (no console). Harmless elsewhere.
if (WIN32)
  set_target_properties(luna_sama PROPERTIES WIN32_EXECUTABLE YES)
endif()



# ---- Link Qt ----
target_link_libraries(luna_sama
  PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
)

# (Optional) copy style.qss next to the binary for easy running from IDEs
add_custom_command(TARGET luna_sama POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:luna_sama>/app
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/app/style.qss
          $<TARGET_FILE_DIR:luna_sama>/app/style.qss
)

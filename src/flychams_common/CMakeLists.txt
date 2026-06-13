cmake_minimum_required(VERSION 3.8)
project(flychams_common)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-variable)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(image_transport REQUIRED)
find_package(geographic_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(foxglove_msgs REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(OpenCV REQUIRED)
list(APPEND CMAKE_MODULE_PATH "/usr/share/cmake/geographiclib")
find_package(GeographicLib REQUIRED)
find_package(flychams_api REQUIRED)

# Disable OpenXLSX documentation generation (requires Doxygen)
set(OPENXLSX_CREATE_DOCS OFF CACHE BOOL "Build library documentation (requires Doxygen and Graphviz/Dot to be installed)" FORCE)

# Add external libraries
add_subdirectory(external/OpenXLSX ${CMAKE_CURRENT_BINARY_DIR}/OpenXLSX_build)
add_subdirectory(external/json ${CMAKE_CURRENT_BINARY_DIR}/json_build)
add_subdirectory(external/nlopt ${CMAKE_CURRENT_BINARY_DIR}/nlopt_build)

# Include directories
include_directories(
  include
)

# Declare the library
add_library(${PROJECT_NAME} SHARED
  # Settings
  src/settings/spreadsheet_parser.cpp
  src/settings/mission_settings_parser.cpp
  src/settings/airsim_settings_creator.cpp
  src/settings/foxglove_layout_creator.cpp
  src/settings/mission_settings_creator.cpp
  src/settings/settings_tools.cpp
  # Utils
  src/utils/math_utils.cpp
  src/utils/vision_utils.cpp
  src/utils/frame_utils.cpp
  src/utils/stream_utils.cpp
  # Base
  src/base/base_node.cpp
  src/base/base_status_node.cpp
  src/base/base_discoverer_node.cpp
  src/base/base_status_discoverer_node.cpp
  src/base/base_module.cpp
  src/base/base_status_module.cpp
  src/base/base_discoverer_module.cpp
  src/base/base_status_discoverer_module.cpp
  # External
  external/hungarian-algorithm-cpp/Hungarian.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include>
  $<INSTALL_INTERFACE:include>
  ${OpenCV_INCLUDE_DIRS}
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/hungarian-algorithm-cpp>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
    rclcpp::rclcpp
    tf2::tf2
    tf2_ros::tf2_ros
    tf2_geometry_msgs::tf2_geometry_msgs
    ${OpenCV_LIBS}
    ${GeographicLib_LIBRARIES}
    OpenXLSX::OpenXLSX
    nlopt
)

ament_target_dependencies(${PROJECT_NAME}
  rclcpp
  std_msgs
  geometry_msgs
  nav_msgs
  sensor_msgs
  image_transport
  geographic_msgs
  visualization_msgs
  foxglove_msgs
  Eigen3
  tf2
  tf2_ros
  tf2_geometry_msgs
  OpenCV
  flychams_api
)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${GeographicLib_INCLUDE_DIRS}
)

# Create executable for settings creator node
add_executable(settings_creator_node src/settings_creator_node.cpp)
target_link_libraries(settings_creator_node ${PROJECT_NAME})
ament_target_dependencies(settings_creator_node rclcpp)

# Install executable
install(TARGETS settings_creator_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

# Install include directory
install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)

install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)
install(
  DIRECTORY external/OpenXLSX/OpenXLSX/headers/
  DESTINATION include/OpenXLSX
)
install(
  DIRECTORY external/OpenXLSX/OpenXLSX/external/
  DESTINATION include/external
)
install(
  DIRECTORY external/json/single_include/
  DESTINATION include
)
install(
  DIRECTORY external/hungarian-algorithm-cpp/
  DESTINATION include
)

# Install library
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

# Install and export OpenXLSX
install(TARGETS OpenXLSX
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

# Install and export NLopt
install(TARGETS nlopt
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

# Export targets
ament_export_targets(${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(
  rclcpp
  std_msgs
  geometry_msgs
  nav_msgs
  sensor_msgs
  image_transport
  geographic_msgs
  visualization_msgs
  foxglove_msgs
  Eigen3
  tf2
  tf2_ros
  tf2_geometry_msgs
  OpenCV
  flychams_api
)
ament_export_libraries(${PROJECT_NAME})
ament_export_include_directories(include)
ament_package()
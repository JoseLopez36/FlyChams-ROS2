cmake_minimum_required(VERSION 3.8)
project(flychams_bringup)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(flychams_core REQUIRED)

# Create executable for settings creator node
add_executable(settings_creator_node src/settings_creator_node.cpp)
target_link_libraries(settings_creator_node rclcpp::rclcpp flychams_core::flychams_core)
ament_target_dependencies(settings_creator_node rclcpp flychams_core)

# Install launch and config files
install(
  DIRECTORY launch/ 
  DESTINATION share/${PROJECT_NAME}
)
install(
  DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)

# Export package
ament_export_dependencies(
  rclcpp
  flychams_core
)
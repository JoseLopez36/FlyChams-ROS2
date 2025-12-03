cmake_minimum_required(VERSION 3.8)
project(flychams_simulation)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(airsim_interfaces REQUIRED)
find_package(flychams_core REQUIRED)

# Create simulation library
set(SIMULATION_SRC
  src/gui/gui_manager.cpp
  src/metrics/metrics_factory.cpp
  src/marker/marker_factory.cpp
  src/target/target_state.cpp
  src/target/target_control.cpp
  src/tools/simulation_tools.cpp
  src/tools/airsim_tools.cpp
)
add_library(${PROJECT_NAME} SHARED ${SIMULATION_SRC})
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Export dependencies
ament_target_dependencies(${PROJECT_NAME}
  rclcpp airsim_interfaces flychams_core
)

# Link to required libraries
target_link_libraries(${PROJECT_NAME}
  rclcpp::rclcpp
)

# Create executable for GUI manager node
add_executable(gui_manager_node src/gui_manager_node.cpp)
target_link_libraries(gui_manager_node ${PROJECT_NAME})
ament_target_dependencies(gui_manager_node rclcpp airsim_interfaces flychams_core)

# Create executable for metrics factory node
add_executable(metrics_factory_node src/metrics_factory_node.cpp)
target_link_libraries(metrics_factory_node ${PROJECT_NAME})
ament_target_dependencies(metrics_factory_node rclcpp flychams_core)

# Create executable for marker factory node
add_executable(marker_factory_node src/marker_factory_node.cpp)
target_link_libraries(marker_factory_node ${PROJECT_NAME})
ament_target_dependencies(marker_factory_node rclcpp flychams_core)

# Create executable for target state node
add_executable(target_state_node src/target_state_node.cpp)
target_link_libraries(target_state_node ${PROJECT_NAME})
ament_target_dependencies(target_state_node rclcpp flychams_core)

# Create executable for target control node
add_executable(target_control_node src/target_control_node.cpp)
target_link_libraries(target_control_node ${PROJECT_NAME})
ament_target_dependencies(target_control_node rclcpp airsim_interfaces flychams_core)

# Install targets and headers
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(TARGETS gui_manager_node metrics_factory_node marker_factory_node target_state_node target_control_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

# Export package
ament_export_targets(${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(
  rclcpp
  airsim_interfaces
  flychams_core
)
ament_export_libraries(${PROJECT_NAME})
ament_export_include_directories(include)
ament_package()
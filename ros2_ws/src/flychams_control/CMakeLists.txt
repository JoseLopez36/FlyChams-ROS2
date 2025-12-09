cmake_minimum_required(VERSION 3.8)
project(flychams_control)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(mavros_msgs REQUIRED)
find_package(airsim_interfaces REQUIRED)
find_package(flychams_core REQUIRED)

# Create control library
set(CONTROL_SRC
  src/drone/drone_frames.cpp
  src/drone/drone_state.cpp
  src/drone/drone_control.cpp
  src/camera/camera_control.cpp
  src/communication/mavros_communication.cpp
  src/communication/camera_communication.cpp
)
add_library(${PROJECT_NAME} SHARED ${CONTROL_SRC})
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Export dependencies
ament_target_dependencies(${PROJECT_NAME}
  rclcpp mavros_msgs airsim_interfaces flychams_core
)

# Link to required libraries
target_link_libraries(${PROJECT_NAME}
  rclcpp::rclcpp
)

# Create executable for drone frames node
add_executable(drone_frames_node src/drone_frames_node.cpp)
target_link_libraries(drone_frames_node ${PROJECT_NAME})
ament_target_dependencies(drone_frames_node rclcpp mavros_msgs flychams_core)

# Create executable for drone state node
add_executable(drone_state_node src/drone_state_node.cpp)
target_link_libraries(drone_state_node ${PROJECT_NAME})
ament_target_dependencies(drone_state_node rclcpp mavros_msgs flychams_core)

# Create executable for drone control node
add_executable(drone_control_node src/drone_control_node.cpp)
target_link_libraries(drone_control_node ${PROJECT_NAME})
ament_target_dependencies(drone_control_node rclcpp mavros_msgs flychams_core)

# Create executable for camera control node
add_executable(camera_control_node src/camera_control_node.cpp)
target_link_libraries(camera_control_node ${PROJECT_NAME})
ament_target_dependencies(camera_control_node rclcpp airsim_interfaces flychams_core)

# Install targets and headers
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(TARGETS drone_frames_node drone_state_node drone_control_node camera_control_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

# Export package
ament_export_targets(${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(
  rclcpp
  mavros_msgs
  airsim_interfaces
  flychams_core
)
ament_export_libraries(${PROJECT_NAME})
ament_export_include_directories(include)
ament_package()
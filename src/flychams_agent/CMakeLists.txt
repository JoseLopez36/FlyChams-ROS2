cmake_minimum_required(VERSION 3.8)
project(flychams_agent)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(mavros REQUIRED)
find_package(mavros_msgs REQUIRED)
find_package(flychams_common REQUIRED)
find_package(flychams_api REQUIRED)

# Find GeographicLib
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};/usr/share/cmake/geographiclib")
find_package(GeographicLib REQUIRED)

# Find GStreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

# Create control library
set(CONTROL_SRC
  src/mavros/mavros_manager.cpp
  src/drone/drone_frames.cpp
  src/drone/drone_state.cpp
  src/drone/drone_control.cpp
  src/camera/camera_frames.cpp
  src/camera/camera_control.cpp
  src/camera/siyi_a8_mini.cpp
  src/mavros/mavros_communication.cpp
  src/mavros/mavros_utils.cpp
  src/camera/camera_communication.cpp
  src/analysis/agent_analysis.cpp
  src/positioning/agent_positioning.cpp
  src/tracking/agent_tracking.cpp
  src/stream/agent_stream.cpp
)
add_library(${PROJECT_NAME} SHARED ${CONTROL_SRC})
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${GeographicLib_INCLUDE_DIRS}
  ${GSTREAMER_INCLUDE_DIRS}
)

# Export dependencies
ament_target_dependencies(${PROJECT_NAME}
  rclcpp mavros mavros_msgs flychams_common flychams_api
)

# Link to required libraries
target_link_libraries(${PROJECT_NAME}
  rclcpp::rclcpp
  ${GeographicLib_LIBRARIES}
  ${GSTREAMER_LIBRARIES}
)

# Create executable for mavros manager node
add_executable(mavros_manager_node src/mavros_manager_node.cpp)
target_link_libraries(mavros_manager_node ${PROJECT_NAME})
ament_target_dependencies(mavros_manager_node rclcpp mavros_msgs flychams_common flychams_api)

# Create executable for drone frames node
add_executable(drone_frames_node src/drone_frames_node.cpp)
target_link_libraries(drone_frames_node ${PROJECT_NAME})
ament_target_dependencies(drone_frames_node rclcpp mavros_msgs flychams_common flychams_api)

# Create executable for drone state node
add_executable(drone_state_node src/drone_state_node.cpp)
target_link_libraries(drone_state_node ${PROJECT_NAME})
ament_target_dependencies(drone_state_node rclcpp mavros_msgs flychams_common flychams_api)

# Create executable for drone control node
add_executable(drone_control_node src/drone_control_node.cpp)
target_link_libraries(drone_control_node ${PROJECT_NAME})
ament_target_dependencies(drone_control_node rclcpp mavros_msgs flychams_common flychams_api)

# Create executable for camera frames node
add_executable(camera_frames_node src/camera_frames_node.cpp)
target_link_libraries(camera_frames_node ${PROJECT_NAME})
ament_target_dependencies(camera_frames_node rclcpp flychams_common flychams_api)

# Create executable for camera control node
add_executable(camera_control_node src/camera_control_node.cpp)
target_link_libraries(camera_control_node ${PROJECT_NAME})
ament_target_dependencies(camera_control_node rclcpp flychams_common flychams_api)

# Create executable for agent analysis node
add_executable(agent_analysis_node src/agent_analysis_node.cpp)
target_link_libraries(agent_analysis_node ${PROJECT_NAME})
ament_target_dependencies(agent_analysis_node rclcpp flychams_common flychams_api)

# Create executable for agent positioning node
add_executable(agent_positioning_node src/agent_positioning_node.cpp)
target_link_libraries(agent_positioning_node ${PROJECT_NAME})
ament_target_dependencies(agent_positioning_node rclcpp flychams_common flychams_api)

# Create executable for agent tracking node
add_executable(agent_tracking_node src/agent_tracking_node.cpp)
target_link_libraries(agent_tracking_node ${PROJECT_NAME})
ament_target_dependencies(agent_tracking_node rclcpp flychams_common flychams_api)

# Create executable for agent stream node
add_executable(agent_stream_node src/agent_stream_node.cpp)
target_link_libraries(agent_stream_node ${PROJECT_NAME})
ament_target_dependencies(agent_stream_node rclcpp flychams_common flychams_api)

# Install Python package
ament_python_install_package(${PROJECT_NAME})

# Install targets and headers
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(TARGETS drone_frames_node drone_state_node drone_control_node camera_frames_node camera_control_node mavros_manager_node agent_analysis_node agent_positioning_node agent_tracking_node agent_stream_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(PROGRAMS
  flychams_agent/target_detection_node.py
  DESTINATION lib/${PROJECT_NAME}
  RENAME target_detection_node
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)

install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

install(DIRECTORY flychams_agent/models/
  DESTINATION share/${PROJECT_NAME}/models
)

# Export package
ament_export_targets(${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(
  rclcpp
  rclpy
  mavros
  mavros_msgs
  flychams_common
  flychams_api
)
ament_export_libraries(${PROJECT_NAME})
ament_export_include_directories(include)
ament_package()
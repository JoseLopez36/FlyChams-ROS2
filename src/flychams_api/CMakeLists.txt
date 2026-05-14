cmake_minimum_required(VERSION 3.8)
project(flychams_api)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)

# Generate interfaces
rosidl_generate_interfaces(${PROJECT_NAME}
  # Base messages
  "msg/Element.msg"
  "msg/Crop.msg"
  # Coordinator messages
  "msg/Registration.msg"
  "msg/ClusterAssignment.msg"
  "msg/ClusterGeometry.msg"
  # Agent messages
  "msg/AgentStatus.msg"
  "msg/AgentAssignment.msg"
  "msg/AgentClusters.msg"
  "msg/ObservationSetpoints.msg"
  # Operator messages
  "msg/MissionMetrics.msg"
  "msg/AgentMetrics.msg"
  "msg/TargetMetrics.msg"
  "msg/ClusterMetrics.msg"
  DEPENDENCIES std_msgs geometry_msgs
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()

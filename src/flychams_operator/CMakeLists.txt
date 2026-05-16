cmake_minimum_required(VERSION 3.8)
project(flychams_operator)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-variable)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(flychams_common REQUIRED)
find_package(flychams_api REQUIRED)

# Create operator library
set(OPERATOR_SRC
  src/metrics/agent_metrics.cpp
  src/metrics/target_metrics.cpp
  src/metrics/cluster_metrics.cpp
  src/markers/agent_markers.cpp
  src/markers/target_markers.cpp
  src/markers/cluster_markers.cpp
)
add_library(${PROJECT_NAME} SHARED ${OPERATOR_SRC})
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Export dependencies
ament_target_dependencies(${PROJECT_NAME}
  rclcpp visualization_msgs flychams_common flychams_api
)

# Link to required libraries
target_link_libraries(${PROJECT_NAME}
  rclcpp::rclcpp
)

# Create executable for metrics node
add_executable(metrics_creator_node src/metrics_creator_node.cpp)
target_link_libraries(metrics_creator_node ${PROJECT_NAME})
ament_target_dependencies(metrics_creator_node rclcpp flychams_common flychams_api)

# Create executable for markers node
add_executable(markers_generator_node src/markers_generator_node.cpp)
target_link_libraries(markers_generator_node ${PROJECT_NAME})
ament_target_dependencies(markers_generator_node rclcpp visualization_msgs flychams_common flychams_api)

# Install targets and headers
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(TARGETS metrics_creator_node markers_generator_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}
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

# Export package
ament_export_targets(${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(
  rclcpp
  visualization_msgs
  flychams_common
  flychams_api
)
ament_export_libraries(${PROJECT_NAME})
ament_export_include_directories(include)
ament_package()
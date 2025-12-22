cmake_minimum_required(VERSION 3.8)
project(flychams_operator)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(flychams_core REQUIRED)

# Create operator library
set(DASHBOARD_SRC
  src/metrics/metrics_factory.cpp
  src/marker/marker_factory.cpp
)
add_library(${PROJECT_NAME} SHARED ${DASHBOARD_SRC})
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Export dependencies
ament_target_dependencies(${PROJECT_NAME}
  rclcpp flychams_core
)

# Link to required libraries
target_link_libraries(${PROJECT_NAME}
  rclcpp::rclcpp
)

# Create executable for metrics factory node
add_executable(metrics_factory_node src/metrics_factory_node.cpp)
target_link_libraries(metrics_factory_node ${PROJECT_NAME})
ament_target_dependencies(metrics_factory_node rclcpp flychams_core)

# Create executable for marker factory node
add_executable(marker_factory_node src/marker_factory_node.cpp)
target_link_libraries(marker_factory_node ${PROJECT_NAME})
ament_target_dependencies(marker_factory_node rclcpp flychams_core)

# Install operator interface node script
install(PROGRAMS
  flychams_operator/operator_interface_node.py
  DESTINATION lib/${PROJECT_NAME}
  RENAME operator_interface_node
)

# Install targets and headers
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(TARGETS metrics_factory_node marker_factory_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

# Export package
ament_export_targets(${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(
  rclcpp
  flychams_core
)
ament_export_libraries(${PROJECT_NAME})
ament_export_include_directories(include)

# Install Python package
ament_python_install_package(${PROJECT_NAME})

ament_package()
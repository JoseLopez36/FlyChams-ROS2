cmake_minimum_required(VERSION 3.8)
project(flychams_coordinator)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-variable)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(flychams_common REQUIRED)
find_package(flychams_api REQUIRED)

# Create coordinator library
set(COORD_SRC
  src/registration/agent_registration.cpp
  src/registration/target_registration.cpp
  src/registration/cluster_registration.cpp
  src/clustering/target_clustering.cpp
  src/analysis/cluster_analysis.cpp
  src/assignment/agent_assignment.cpp
)
add_library(${PROJECT_NAME} SHARED ${COORD_SRC})
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Export dependencies
ament_target_dependencies(${PROJECT_NAME}
  rclcpp flychams_common flychams_api
)

# Link to required libraries
target_link_libraries(${PROJECT_NAME}
  rclcpp::rclcpp
)

# Create executable for element registrator node
add_executable(element_registrator_node src/element_registrator_node.cpp)
target_link_libraries(element_registrator_node ${PROJECT_NAME})
ament_target_dependencies(element_registrator_node rclcpp flychams_common flychams_api)

# Create executable for target clustering node
add_executable(target_clustering_node src/target_clustering_node.cpp)
target_link_libraries(target_clustering_node ${PROJECT_NAME})
ament_target_dependencies(target_clustering_node rclcpp flychams_common flychams_api)

# Create executable for cluster analysis node
add_executable(cluster_analysis_node src/cluster_analysis_node.cpp)
target_link_libraries(cluster_analysis_node ${PROJECT_NAME})
ament_target_dependencies(cluster_analysis_node rclcpp flychams_common flychams_api)

# Create executable for agent assignment node
add_executable(agent_assignment_node src/agent_assignment_node.cpp)
target_link_libraries(agent_assignment_node ${PROJECT_NAME})
ament_target_dependencies(agent_assignment_node rclcpp flychams_common flychams_api)

# Install targets and headers
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(TARGETS agent_assignment_node target_clustering_node cluster_analysis_node element_registrator_node
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
  flychams_common
  flychams_api
)
ament_export_libraries(${PROJECT_NAME})
ament_export_include_directories(include)
ament_package()
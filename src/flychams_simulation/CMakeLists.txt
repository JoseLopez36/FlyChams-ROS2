cmake_minimum_required(VERSION 3.8)
project(flychams_simulation)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-variable)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(airsim_interfaces REQUIRED)
find_package(flychams_common REQUIRED)
find_package(flychams_api REQUIRED)

# Create simulation library
set(SIMULATION_SRC
  # Target
  src/target/target_state.cpp
  src/target/target_control.cpp
  # Bridge
  src/bridge/simulation_bridge.cpp
  src/bridge/agent_bridge.cpp
)
add_library(${PROJECT_NAME} SHARED ${SIMULATION_SRC})
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Export dependencies
ament_target_dependencies(${PROJECT_NAME}
  rclcpp airsim_interfaces flychams_common flychams_api
)

# Link to required libraries
target_link_libraries(${PROJECT_NAME}
  rclcpp::rclcpp
)

# Create executable for target state node
add_executable(target_state_node src/target_state_node.cpp)
target_link_libraries(target_state_node ${PROJECT_NAME})
ament_target_dependencies(target_state_node rclcpp flychams_common flychams_api)

# Create executable for target control node
add_executable(target_control_node src/target_control_node.cpp)
target_link_libraries(target_control_node ${PROJECT_NAME})
ament_target_dependencies(target_control_node rclcpp airsim_interfaces flychams_common flychams_api)

# Create executable for agent simulation bridge node
add_executable(agent_bridge_node src/agent_bridge_node.cpp)
target_link_libraries(agent_bridge_node ${PROJECT_NAME})
ament_target_dependencies(agent_bridge_node rclcpp airsim_interfaces flychams_common flychams_api)

# Install targets and headers
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(TARGETS target_state_node target_control_node agent_bridge_node
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
  airsim_interfaces
  flychams_common
  flychams_api
)
ament_export_libraries(${PROJECT_NAME})
ament_export_include_directories(include)
ament_package()
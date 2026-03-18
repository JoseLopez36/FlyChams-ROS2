#!/usr/bin/env python3

"""
Operator interface node for the FlyingChameleons system
Author: Jose Francisco Lopez Ruiz
Date: 2026-02-12
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import PointStamped
from flychams_interfaces.msg import Registration, ClusterGeometry, AgentGuiSetpoints
from PyQt5.QtCore import QObject, pyqtSignal
from PyQt5.QtWidgets import QApplication
import threading
import logging
import os
from typing import Dict

from flychams_operator.core import AgentData, TargetData, ClusterData, replace_id_in_topic, spin_ros_node

class OperatorInterfaceSignals(QObject):
    """Qt signals for thread-safe GUI updates"""
    agent_added = pyqtSignal(str, list)  # agent_id, stream_urls
    agent_removed = pyqtSignal(str)
    agent_position_updated = pyqtSignal(str, float, float, float)
    agent_setpoint_updated = pyqtSignal(str, float, float, float)
    target_added = pyqtSignal(str)
    target_removed = pyqtSignal(str)
    target_position_updated = pyqtSignal(str, float, float, float)
    cluster_added = pyqtSignal(str)
    cluster_removed = pyqtSignal(str)
    cluster_geometry_updated = pyqtSignal(str, float, float, float, float)

class OperatorInterface(Node):
    """Operator interface that discovers and tracks agents, targets and clusters"""

    # Element types matching C++ enum (0: agent, 1: target, 2: cluster)
    ELEMENT_TYPE_AGENT = 1
    ELEMENT_TYPE_TARGET = 2
    ELEMENT_TYPE_CLUSTER = 3

    def __init__(self, signals: OperatorInterfaceSignals):
        # Initialize node with parameter support
        super().__init__('operator_interface_node', 
                        allow_undeclared_parameters=True,
                        automatically_declare_parameters_from_overrides=True)
        
        self.signals = signals
        
        # Get parameters with defaults
        self.declare_parameter('update_rate', 10.0)
        self.update_rate = self.get_parameter('update_rate').get_parameter_value().double_value
        
        # Get topic names from parameters
        self.registration_topic = self.get_parameter('global_topics.registration').get_parameter_value().string_value
        self.agent_global_position_pattern = self.get_parameter('agent_topics.global_position').get_parameter_value().string_value
        self.agent_setpoint_pattern = self.get_parameter('agent_topics.position_setpoint').get_parameter_value().string_value
        self.target_position_pattern = self.get_parameter('target_topics.true_position').get_parameter_value().string_value
        self.cluster_geometry_pattern = self.get_parameter('cluster_topics.geometry').get_parameter_value().string_value
        
        # Data structures
        self.agents: Dict[str, AgentData] = {}
        self.targets: Dict[str, TargetData] = {}
        self.clusters: Dict[str, ClusterData] = {}
        self.elements: Dict[str, int] = {}  # id -> type
        
        # Create discovery subscriber
        self.registration_sub = self.create_subscription(
            Registration,
            self.registration_topic,
            self.on_discovery,
            10
        )
        
        self.get_logger().info('Operator interface initialized')

    def on_discovery(self, msg: Registration):
        """Handle registration messages for discovery"""
        # Track current elements in this message
        current_elements = set()
        
        for element in msg.elements:
            element_id = element.id
            element_type = element.type
            
            current_elements.add(element_id)

            # Add element if it doesn't exist already
            if element_id not in self.elements:
                self.elements[element_id] = element_type
                
                # Call corresponding add callback
                if element_type == self.ELEMENT_TYPE_AGENT:
                    self.add_agent(element_id)
                elif element_type == self.ELEMENT_TYPE_TARGET:
                    self.add_target(element_id)
                elif element_type == self.ELEMENT_TYPE_CLUSTER:
                    self.add_cluster(element_id)
        
        # Remove elements that are no longer present
        to_remove = []
        for element_id in self.elements:
            if element_id not in current_elements:
                to_remove.append(element_id)
        
        for element_id in to_remove:
            element_type = self.elements[element_id]
            if element_type == self.ELEMENT_TYPE_AGENT:
                self.remove_agent(element_id)
            elif element_type == self.ELEMENT_TYPE_TARGET:
                self.remove_target(element_id)
            elif element_type == self.ELEMENT_TYPE_CLUSTER:
                self.remove_cluster(element_id)
            
            del self.elements[element_id]

    def add_agent(self, agent_id: str):
        """Add an agent and create its subscribers"""
        self.get_logger().info(f'Adding agent: {agent_id}')
        
        # Create and add agent
        self.agents[agent_id] = AgentData()
        
        # Create agent position subscriber
        global_position_topic = replace_id_in_topic(self.agent_global_position_pattern, 'AGENTID', agent_id)
        
        self.agents[agent_id].position_sub = self.create_subscription(
            PointStamped,
            global_position_topic,
            lambda msg, aid=agent_id: self.agent_position_callback(aid, msg),
            10
        )
        
        # Create agent position setpoint subscriber
        setpoint_topic = replace_id_in_topic(self.agent_setpoint_pattern, 'AGENTID', agent_id)
        
        self.agents[agent_id].position_setpoint_sub = self.create_subscription(
            PointStamped,
            setpoint_topic,
            lambda msg, aid=agent_id: self.agent_position_setpoint_callback(aid, msg),
            10
        )

        # Get stream URLs from agent configuration
        stream_urls = []
        # Get list of multi_camera IDs for this agent
        multi_cameras_ids_param = f'agents.{agent_id}.tracking.multi_cameras.ids'
        multi_camera_ids = self.get_parameter(multi_cameras_ids_param).get_parameter_value().string_array_value
        multi_windows_ids_param = f'agents.{agent_id}.tracking.multi_windows.ids'
        multi_window_ids = self.get_parameter(multi_windows_ids_param).get_parameter_value().string_array_value
        
        # Get stream_url for each multi_camera
        for multi_camera_id in multi_camera_ids:
            stream_url_param = f'agents.{agent_id}.tracking.multi_cameras.{multi_camera_id}.interface_stream_url'
            try:
                stream_url = self.get_parameter(stream_url_param).get_parameter_value().string_value
                stream_urls.append(stream_url)
            except Exception as e:
                self.get_logger().warn(f'Could not get stream_url for {agent_id}/{multi_camera_id}: {e}')

        # Get stream_url for each multi_window
        for multi_window_id in multi_window_ids:
            stream_url_param = f'agents.{agent_id}.tracking.multi_windows.{multi_window_id}.interface_stream_url'
            try:
                stream_url = self.get_parameter(stream_url_param).get_parameter_value().string_value
                stream_urls.append(stream_url)
            except Exception as e:
                self.get_logger().warn(f'Could not get stream_url for {agent_id}/{multi_window_id}: {e}')
                    
        # Emit signal for GUI
        self.signals.agent_added.emit(agent_id, stream_urls)

    def remove_agent(self, agent_id: str):
        """Remove an agent and clean up its subscribers"""
        self.get_logger().info(f'Removing agent: {agent_id}')
        
        if agent_id in self.agents:
            # Subscribers will be automatically cleaned up when node is destroyed
            # Just remove from map
            del self.agents[agent_id]
            
            # Emit signal for GUI
            self.signals.agent_removed.emit(agent_id)

    def add_target(self, target_id: str):
        """Add a target and create its subscribers"""
        self.get_logger().info(f'Adding target: {target_id}')
        
        # Create and add target
        self.targets[target_id] = TargetData()
        
        # Create target position subscriber
        true_position_topic = replace_id_in_topic(self.target_position_pattern, 'TARGETID', target_id)
        
        self.targets[target_id].position_sub = self.create_subscription(
            PointStamped,
            true_position_topic,
            lambda msg, tid=target_id: self.target_position_callback(tid, msg),
            10
        )
        
        # Emit signal for GUI
        self.signals.target_added.emit(target_id)

    def remove_target(self, target_id: str):
        """Remove a target and clean up its subscribers"""
        self.get_logger().info(f'Removing target: {target_id}')
        
        if target_id in self.targets:
            # Subscribers will be automatically cleaned up when node is destroyed
            # Just remove from map
            del self.targets[target_id]
            
            # Emit signal for GUI
            self.signals.target_removed.emit(target_id)

    def add_cluster(self, cluster_id: str):
        """Add a cluster and create its subscribers"""
        self.get_logger().info(f'Adding cluster: {cluster_id}')
        
        # Create and add cluster
        self.clusters[cluster_id] = ClusterData()
        
        # Create cluster geometry subscriber
        geometry_topic = replace_id_in_topic(self.cluster_geometry_pattern, 'CLUSTERID', cluster_id)
        
        self.clusters[cluster_id].geometry_sub = self.create_subscription(
            ClusterGeometry,
            geometry_topic,
            lambda msg, cid=cluster_id: self.cluster_geometry_callback(cid, msg),
            10
        )
        
        # Emit signal for GUI
        self.signals.cluster_added.emit(cluster_id)

    def remove_cluster(self, cluster_id: str):
        """Remove a cluster and clean up its subscribers"""
        self.get_logger().info(f'Removing cluster: {cluster_id}')
        
        if cluster_id in self.clusters:
            # Subscribers will be automatically cleaned up when node is destroyed
            # Just remove from map
            del self.clusters[cluster_id]
            
            # Emit signal for GUI
            self.signals.cluster_removed.emit(cluster_id)

    def agent_position_callback(self, agent_id: str, msg: PointStamped):
        """Callback for agent position updates"""
        if agent_id in self.agents:
            # Emit signal for GUI update
            self.signals.agent_position_updated.emit(
                agent_id,
                msg.point.x,
                msg.point.y,
                msg.point.z
            )

    def agent_position_setpoint_callback(self, agent_id: str, msg: PointStamped):
        """Callback for agent position setpoint updates"""
        if agent_id in self.agents:
            # Emit signal for GUI update
            self.signals.agent_setpoint_updated.emit(
                agent_id,
                msg.point.x,
                msg.point.y,
                msg.point.z
            )

    def target_position_callback(self, target_id: str, msg: PointStamped):
        """Callback for target position updates"""
        if target_id in self.targets:
            # Emit signal for GUI update
            self.signals.target_position_updated.emit(
                target_id,
                msg.point.x,
                msg.point.y,
                msg.point.z
            )

    def cluster_geometry_callback(self, cluster_id: str, msg: ClusterGeometry):
        """Callback for cluster geometry updates"""
        if cluster_id in self.clusters:
            # Emit signal for GUI update
            self.signals.cluster_geometry_updated.emit(
                cluster_id,
                msg.center.x,
                msg.center.y,
                msg.center.z,
                msg.radius
            )

def main(args=None):
    """Main entry point for the operator interface node"""
    # Configure Python logging for camera panel and other operator modules
    log_level_str = os.environ.get('PYTHON_LOG_LEVEL', 'INFO').upper()
    log_level = getattr(logging, log_level_str, logging.INFO)
    
    # Configure root logger
    logging.basicConfig(
        level=log_level,
        format='%(asctime)s [%(levelname)s] [%(name)s] %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )
    
    # Get modules to log (default to monitoring_panel if not specified)
    log_modules = os.environ.get('PYTHON_LOG_MODULES', 'flychams_operator.interface.monitoring_panel').split(',')
    for module_name in log_modules:
        module_name = module_name.strip()
        if module_name:
            logger = logging.getLogger(module_name)
            logger.setLevel(log_level)
    
    rclpy.init(args=args)
    
    # Create Qt application
    app = QApplication([])
    
    # Create signals object
    signals = OperatorInterfaceSignals()
    
    # Create operator interface node
    node = OperatorInterface(signals)

    # Get is_sim parameter
    is_sim = node.get_parameter('is_sim').value
    
    # Import GUI components here to avoid circular imports
    from flychams_operator.interface.main_window import MainWindow
    
    # Create main window
    main_window = MainWindow(signals, is_sim=is_sim)
    main_window.show()
    
    # Create ROS2 executor for separate thread
    executor = rclpy.executors.SingleThreadedExecutor()
    
    # Start ROS2 executor in separate thread
    ros_thread = threading.Thread(target=spin_ros_node, args=(node, executor), daemon=True)
    ros_thread.start()
    
    # Run Qt event loop
    try:
        app.exec()
    except KeyboardInterrupt:
        pass
    finally:
        # Cleanup
        node.get_logger().info('Shutting down operator interface node')
        main_window.close()
        executor.shutdown()
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()


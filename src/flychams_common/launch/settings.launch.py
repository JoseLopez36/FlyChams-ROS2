#!/usr/bin/env python3
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    # Declare launch arguments
    config_spreadsheet_path = DeclareLaunchArgument(
        'config_spreadsheet_path',
        default_value='',
        description='Path to configuration spreadsheet file'
    )

    mission_settings_path = DeclareLaunchArgument(
        'mission_settings_path',
        default_value='',
        description='Path to output mission settings file'
    )

    airsim_settings_path = DeclareLaunchArgument(
        'airsim_settings_path',
        default_value='',
        description='Path to output AirSim settings file'
    )

    # Settings creator node
    settings_creator_node = Node(
        package='flychams_common',
        executable='settings_creator_node',
        name='settings_creator_node',
        parameters=[
            {'path.config_spreadsheet_path': LaunchConfiguration('config_spreadsheet_path')},
            {'path.mission_settings_path': LaunchConfiguration('mission_settings_path')},
            {'path.airsim_settings_path': LaunchConfiguration('airsim_settings_path')}
        ],
        output='screen'
    )

    return LaunchDescription([
        config_spreadsheet_path,
        mission_settings_path,
        airsim_settings_path,
        settings_creator_node,
    ])
"""
Target detection module for the FlyingChameleons system
Author: Jose Francisco Lopez Ruiz
Date: 2026-03-02
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import PointStamped
from tf2_ros import TransformException
from tf2_ros.buffer import Buffer
from tf2_ros.transform_listener import TransformListener

# Import tracker modules
from flychams_agent.detection.yolo_tracker import YoloTracker
from flychams_agent.detection.vision_utils import VisionUtils

class TargetDetection:
    def __init__(self, node: Node):
        self.node = node
        self.logger = node.get_logger()
        self.vision_utils = VisionUtils()
        
        # --- Parameters ---
        self.inference_rate = self.node.get_parameter('inference_rate').get_parameter_value().double_value
        self.detector_model = self.node.get_parameter('detector_model').get_parameter_value().string_value
        self.target_class = self.node.get_parameter('target_class').get_parameter_value().string_value
        self.width = self.node.get_parameter('width').get_parameter_value().integer_value
        self.height = self.node.get_parameter('height').get_parameter_value().integer_value
        self.z_plane = self.node.get_parameter('z_plane').get_parameter_value().double_value

        # Get agent ID from parameters
        self.agent_id = self.node.get_parameter('agent_id').get_parameter_value().string_value

        # Get camera parameters from parameters
        self.camera_ids = self.node.get_parameter('agents.' + self.agent_id + '.tracking.multi_cameras.ids').get_parameter_value().string_array_value
        self.camera_id = self.camera_ids[0]
        self.focal = self.node.get_parameter('agents.' + self.agent_id + '.tracking.multi_cameras.' + self.camera_id + '.ref_focal').get_parameter_value().double_value
        self.sensor_width = self.node.get_parameter('agents.' + self.agent_id + '.tracking.multi_cameras.' + self.camera_id + '.camera.sensor_size.width').get_parameter_value().double_value
        self.sensor_height = self.node.get_parameter('agents.' + self.agent_id + '.tracking.multi_cameras.' + self.camera_id + '.camera.sensor_size.height').get_parameter_value().double_value
        self.rho_x = self.sensor_width / self.width
        self.rho_y = self.sensor_height / self.height
        self.K = self.vision_utils.build_K(self.width, self.height, self.focal, self.rho_x, self.rho_y)

        # Get source URL from parameters
        self.source = self.node.get_parameter('agents.' + self.agent_id + '.inference_stream_url').get_parameter_value().string_value

        # Get target estimated position topic from parameters
        self.target_est_position_topic = self.node.get_parameter('target_topics.est_position').get_parameter_value().string_value

        # Get relevant frames from parameters
        self.source_frame = self.node.get_parameter('global_frames.world').get_parameter_value().string_value
        self.target_frame_template = self.node.get_parameter('agent_frames.camera_optical').get_parameter_value().string_value
        self.target_frame = self.target_frame_template.replace('AGENTID', self.agent_id).replace('HEADID', self.camera_id)

        # TF Listener
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self.node)
        self.camera_pose = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0] # x, y, z, yaw, pitch, roll
        
        # --- Publishers ---
        self.target_publishers = {}
        
        # --- Tracker Initialization ---
        self.tracker = YoloTracker(
            model_name=self.detector_model,
            target_class=self.target_class, 
            source=self.source, 
            logger=self.logger
        )
        
        # --- Processing Timer ---
        timer_period = 1.0 / self.inference_rate
        self.timer = self.node.create_timer(timer_period, self.update)
        
        self.logger.info(f"TargetDetection module initialized")

    def update(self):
        # Update tracker
        try:
            tracked_objects = self.tracker.update()
            self.logger.info(f"Number of tracked objects: {len(tracked_objects)}")
        except Exception as e:
            self.logger.error(f"Tracker update failed: {e}")
            return

        if not tracked_objects or len(tracked_objects) == 0:
            return

        # Get camera pose
        try:
            t = self.tf_buffer.lookup_transform(
                self.source_frame,
                self.target_frame,
                rclpy.time.Time())
            
            tx = t.transform.translation.x
            ty = t.transform.translation.y
            tz = t.transform.translation.z
            
            qx = t.transform.rotation.x
            qy = t.transform.rotation.y
            qz = t.transform.rotation.z
            qw = t.transform.rotation.w
            
            roll, pitch, yaw = self.euler_from_quaternion(qx, qy, qz, qw)
            
            self.camera_pose = [tx, ty, tz, yaw, pitch, roll]
            
        except TransformException as ex:
            self.logger.warn(f'Could not transform {self.source_frame} to {self.target_frame}: {ex}')
            return
            
        # Calculate 3D positions
        for obj in tracked_objects:
            tid, x1, y1, x2, y2, score, cls_id = obj
            
            # Calculate center (u, v) - using bottom center for ground projection
            u = 0.5 * (x1 + x2)
            v = y2
            
            # Project to 3D
            calculated_pos = self.vision_utils.calculate_3d_position(
                u, v, self.K, self.camera_pose, self.z_plane
            )
            
            if calculated_pos is not None:
                tx, ty = calculated_pos
                
                # Construct target_id string
                target_id = f"TARGET{tid}"
                
                # Check if publisher exists for this target_id
                if target_id not in self.target_publishers:
                    topic_name = self.target_est_position_topic.replace('TARGETID', target_id)
                    self.target_publishers[target_id] = self.node.create_publisher(
                        PointStamped, topic_name, 10
                    )
                    self.logger.info(f"Created publisher for {target_id} on {topic_name}")

                # Create PointStamped message
                msg = PointStamped()
                msg.header.stamp = self.node.get_clock().now().to_msg()
                msg.header.frame_id = self.source_frame
                msg.point.x = tx
                msg.point.y = ty
                msg.point.z = self.z_plane
                
                # Publish the message
                self.target_publishers[target_id].publish(msg)

    def shutdown(self) -> None:
        self.logger.info("TargetDetection module shutting down")
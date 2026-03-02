"""
3D Position calculation logic for detected targets
Author: Jose Francisco Lopez Ruiz
Date: 2026-03-02
"""

import numpy as np

class VisionUtils:
    """Encapsulates the logic for calculating 3D positions of targets from 2D image coordinates"""

    def __init__(self):
        pass

    @staticmethod
    def build_k_from_hfov(w, h, fov_h_deg):
        """
        Build camera intrinsic matrix K from horizontal FOV
        
        Args:
            w: Image width in pixels
            h: Image height in pixels
            fov_h_deg: Horizontal Field of View in degrees
            
        Returns:
            3x3 Camera intrinsic matrix K
        """
        fov = np.deg2rad(fov_h_deg)
        fx = (w / 2.0) / np.tan(fov / 2.0)
        fy = fx
        cx, cy = w / 2.0, h / 2.0
        return np.array([[fx, 0.0, cx],
                         [0.0, fy, cy],
                         [0.0, 0.0, 1.0]], dtype=np.float64)

    @staticmethod
    def rot_from_ypr(yaw_deg, pitch_deg, roll_deg):
        """
        Create a rotation matrix from Yaw, Pitch, Roll (in degrees)
        
        Args:
            yaw_deg: Yaw in degrees
            pitch_deg: Pitch in degrees
            roll_deg: Roll in degrees
            
        Returns:
            3x3 Rotation matrix
        """
        yaw, pitch, roll = map(np.deg2rad, [yaw_deg, pitch_deg, roll_deg])
        cy, sy = np.cos(yaw), np.sin(yaw)
        cp, sp = np.cos(pitch), np.sin(pitch)
        cr, sr = np.cos(roll), np.sin(roll)
        
        # Rz * Ry * Rx
        Rz = np.array([[cy, -sy, 0], [sy, cy, 0], [0, 0, 1]], dtype=np.float64)
        Ry = np.array([[cp, 0, sp], [0, 1, 0], [-sp, 0, cp]], dtype=np.float64)
        Rx = np.array([[1, 0, 0], [0, cr, -sr], [0, sr, cr]], dtype=np.float64)
        
        return Rz @ Ry @ Rx

    @staticmethod
    def ray_from_pixel(u, v, K):
        """
        Calculate a normalized ray in the camera coordinate system from pixel coordinates
        
        Args:
            u: Pixel x-coordinate
            v: Pixel y-coordinate
            K: 3x3 Camera intrinsic matrix
            
        Returns:
            Normalized 3D ray vector [x, y, z] in camera frame
        """
        fx, fy = K[0, 0], K[1, 1]
        cx, cy = K[0, 2], K[1, 2]
        
        # Convert pixel to normalized camera coordinates
        # Note: The original logic used [1.0, x_cv, -y_cv] which suggests a specific 
        # camera coordinate system convention (X-forward, Y-right, Z-up).
        x_cv = (u - cx) / fx
        y_cv = (v - cy) / fy
        
        # Original logic from MULTIPLATANO_ROS2.py:
        # ray = np.array([1.0, x_cv, -y_cv], dtype=np.float64)
        ray = np.array([1.0, x_cv, -y_cv], dtype=np.float64)
        return ray / np.linalg.norm(ray)

    @staticmethod
    def intersect_plane_z_world(ray_w, cam_pos_w, z_plane_w = 0.0):
        """
        Intersect a ray with a horizontal plane at a given Z altitude in world coordinates
        
        Args:
            ray_w: Normalized ray vector in world coordinates
            cam_pos_w: Camera position [x, y, z] in world coordinates
            z_plane_w: Z-altitude of the plane to intersect with
            
        Returns:
            3D intersection point [x, y, z] or None if no intersection
        """
        # Check if ray is parallel to the plane
        if abs(ray_w[2]) < 1e-9:
            return None
            
        t = (z_plane_w - cam_pos_w[2]) / ray_w[2]
        
        # Check if intersection is in front of the camera
        if t <= 0:
            return None
            
        return cam_pos_w + t * ray_w

    def calculate_3d_position(self, u, v, K, drone_pose, z_plane = 0.0):
        """
        Full pipeline to calculate 2D ground coordinates (x, y) from a pixel
        
        Args:
            u: Pixel x-coordinate
            v: Pixel y-coordinate
            K: Camera intrinsics
            drone_pose: [x, y, z, yaw, pitch, roll] in degrees
            z_plane: Ground altitude
            
        Returns:
            (x, y) ground coordinates or None
        """
        x, y, z, yaw, pitch, roll = drone_pose
        cam_pos = np.array([x, y, z], dtype=np.float64)
        
        # 1. Get rotation matrix
        Rwc = self.rot_from_ypr(yaw, pitch, roll)
        
        # 2. Get ray in camera frame
        ray_c = self.ray_from_pixel(u, v, K)
        
        # 3. Transform ray to world frame
        ray_w = Rwc @ ray_c
        
        # 4. Intersect with ground plane
        P = self.intersect_plane_z_world(ray_w, cam_pos, z_plane)
        
        if P is not None:
            return (float(-P[1]), float(P[0]))
            
        return None
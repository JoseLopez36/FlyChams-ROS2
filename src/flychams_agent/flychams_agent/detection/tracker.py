"""
Base Tracker interface for the FlyChams system
Author: Jose Francisco Lopez Ruiz
Date: 2026-02-12
"""

from abc import ABC, abstractmethod
from typing import List, Any
import numpy as np

class Tracker(ABC):
    """Abstract base class for all tracker implementations"""

    @abstractmethod
    def detect(self, image: np.ndarray) -> List[Any]:
        """
        Perform detection on the given image
        
        Args:
            image: OpenCV image (numpy array)
            
        Returns:
            A list of detections
        """
        pass

    @abstractmethod
    def update(self, image: np.ndarray) -> List[Any]:
        """
        Update the tracker with a new frame
        
        Args:
            image: OpenCV image (numpy array)
            
        Returns:
            A list of tracked objects
        """
        pass
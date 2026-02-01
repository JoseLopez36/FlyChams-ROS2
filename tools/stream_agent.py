import sys
import time
import socket
import json
import threading
import argparse
import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib

class AgentStream:
    def __init__(self, host="127.0.0.1", source_port=5000, destination_port=6000, control_port=7000, gpu_type="auto"):
        """
        Initialize the AgentStream class
        """
        self.host = host
        self.source_port = source_port
        self.destination_port = destination_port
        self.control_port = control_port
        
        Gst.init(None)
        
        if gpu_type == "auto":
            self.gpu_type = self.detect_gpu_type()
            print(f"Auto-detected GPU type: {self.gpu_type}")
        else:
            self.gpu_type = gpu_type.lower()
            
        self.pipeline = None
        self.loop = None
        self.crops = []
        self.tracking_thread = None
        self.running = False

    def detect_gpu_type(self):
        """
        Detects available GPU hardware encoding/decoding support.
        Checks for NVIDIA elements first, then falls back to AMD/VAAPI.
        """
        registry = Gst.Registry.get()

        # Check for VAAPI elements (AMD/Intel)
        va_enc = registry.find_plugin('vaapi')
        if va_enc:
            return "amd"
        
        # Check for NVIDIA elements
        nv_enc = registry.find_plugin('nvcodec')
        if nv_enc:
            return "nvidia"
            
        print("Warning: No hardware acceleration found. Defaulting to AMD (VAAPI) pipeline which might fail")
        return "amd"

    def create_pipeline_str(self):
        if self.gpu_type == "nvidia":
            return self.create_nvidia_pipeline_str()
        else:
            return self.create_amd_pipeline_str()

    def create_amd_pipeline_str(self):
        """
        Constructs the GStreamer pipeline string with 1 full res (scaled) and 4 crops for AMD
        """
        # 0. Base source: Listening on source_port
        pipeline_str = (
            f"udpsrc port={self.source_port} buffer-size=5242880 caps=\"application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H265, payload=(int)96\" ! "
            "rtph265depay ! h265parse ! vah265dec ! queue max-size-buffers=1 ! tee name=t "
        )
        
        # 1. Full image branch: Rescale to 1080p, output to destination_port
        full_res_branch = (
            "t. ! queue leaky=downstream max-size-buffers=10 ! "
            "vapostproc ! "
            "video/x-raw,width=1920,height=1080 ! "
            "vah265enc bitrate=8000 rate-control=cbr ! "
            "h265parse ! "
            "mpegtsmux ! "
            f"udpsink host={self.host} port={self.destination_port} sync=false buffer-size=52428800 "
        )
        pipeline_str += full_res_branch
        
        # 2. Crop branches: Output to destination_port + i
        for i in range(1, 5):
            port = self.destination_port + i
            name = f"crop_{i}"
            crop_branch = (
                "t. ! queue leaky=downstream max-size-buffers=10 ! "
                f"videocrop name={name} ! "
                "vapostproc ! "
                "video/x-raw,width=1280,height=720 ! "
                "vah265enc bitrate=3000 rate-control=cbr ! "
                "h265parse ! "
                "mpegtsmux ! "
                f"udpsink host={self.host} port={port} sync=false buffer-size=52428800 "
            )
            pipeline_str += crop_branch
            
        return pipeline_str

    def create_nvidia_pipeline_str(self):
        """
        Constructs the GStreamer pipeline string with 1 full res (scaled) and 4 crops for Nvidia
        """
        # 0. Base source: Listening on source_port
        pipeline_str = (
            f"udpsrc port={self.source_port} buffer-size=5242880 caps=\"application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H265, payload=(int)96\" ! "
            "rtph265depay ! h265parse ! nvh265dec ! queue max-size-buffers=1 ! tee name=t "
        )
        
        # 1. Full image branch: Rescale to 1080p, output to destination_port
        full_res_branch = (
            "t. ! queue leaky=downstream max-size-buffers=10 ! "
            "videoscale ! "
            "video/x-raw,width=1920,height=1080 ! "
            "nvh265enc bitrate=8000 rc-mode=cbr ! "
            "h265parse ! "
            "mpegtsmux ! "
            f"udpsink host={self.host} port={self.destination_port} sync=false buffer-size=52428800 "
        )
        pipeline_str += full_res_branch
        
        # 2. Crop branches: Output to destination_port + i
        for i in range(1, 5):
            port = self.destination_port + i
            name = f"crop_{i}"
            crop_branch = (
                "t. ! queue leaky=downstream max-size-buffers=10 ! "
                f"videocrop name={name} ! "
                "videoscale ! "
                "video/x-raw,width=1280,height=720 ! "
                "nvh265enc bitrate=3000 rc-mode=cbr ! "
                "h265parse ! "
                "mpegtsmux ! "
                f"udpsink host={self.host} port={port} sync=false buffer-size=52428800 "
            )
            pipeline_str += crop_branch
            
        return pipeline_str

    def tracking_loop(self):
        """
        Listens for UDP packets with JSON data to update crop properties.
        Expected JSON: {"id": 1, "top": 10, "left": 10, "right": 10, "bottom": 10}
        """
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(("0.0.0.0", self.control_port))
        sock.setblocking(False)

        print(f"Listening for crop data on UDP {self.control_port}...")

        while self.running:
            try:
                data, addr = sock.recvfrom(1024)
                crop_data = json.loads(data.decode('utf-8'))
                
                crop_id = crop_data.get("id")
                if crop_id is not None and crop_id in self.crops:
                    cropper = self.crops[crop_id]
                    # Safely apply updates, clamping to 0 to avoid GStreamer warnings
                    if "left" in crop_data: cropper.set_property("left", max(0, int(crop_data["left"])))
                    if "right" in crop_data: cropper.set_property("right", max(0, int(crop_data["right"])))
                    if "top" in crop_data: cropper.set_property("top", max(0, int(crop_data["top"])))
                    if "bottom" in crop_data: cropper.set_property("bottom", max(0, int(crop_data["bottom"])))
                else:
                    print(f"Received crop data with invalid or missing id: {crop_data}")

            except BlockingIOError:
                # No data waiting, sleep briefly to save CPU
                time.sleep(0.01)
            except json.JSONDecodeError:
                print("Received invalid JSON")
            except Exception as e:
                if self.running:
                    print(f"Error in tracking loop: {e}")

        sock.close()

    def start(self):
        """
        Starts the GStreamer pipeline and the tracking thread
        """
        pipeline_str = self.create_pipeline_str()
        print(f"Launching pipeline: {pipeline_str}")
        self.pipeline = Gst.parse_launch(pipeline_str)
        
        # Retrieve crop elements for dynamic control
        self.crops = {}
        for i in range(1, 5):
            name = f"crop_{i}"
            cropper = self.pipeline.get_by_name(name)
            if cropper:
                self.crops[i] = cropper
            else:
                print(f"Error: Could not find element named '{name}'")
                sys.exit(1)

        # Start the pipeline
        self.pipeline.set_state(Gst.State.PLAYING)
        self.running = True
        print("Pipeline started")

        # Start the tracking thread
        self.tracking_thread = threading.Thread(target=self.tracking_loop)
        self.tracking_thread.daemon = True
        self.tracking_thread.start()

        # Run the main GLib loop
        self.loop = GLib.MainLoop()
        try:
            self.loop.run()
        except KeyboardInterrupt:
            self.stop()

    def stop(self):
        """
        Stops the pipeline and cleanup.
        """
        print("Stopping pipeline...")
        self.running = False
        if self.pipeline:
            self.pipeline.set_state(Gst.State.NULL)
            self.pipeline = None
        if self.loop:
            self.loop.quit()
        print("Pipeline stopped")

def main():
    parser = argparse.ArgumentParser(description="Agent Stream")
    parser.add_argument("--host", default="127.0.0.1", help="Destination host")
    parser.add_argument("--source-port", type=int, default=5000, help="Source port")
    parser.add_argument("--destination-port", type=int, default=6000, help="Destination port start")
    parser.add_argument("--control-port", type=int, default=7000, help="Control port")
    parser.add_argument("--gpu-type", default="auto", choices=["auto", "amd", "nvidia"], help="GPU type (auto, amd or nvidia)")
    
    args = parser.parse_args()
    
    agent = AgentStream(
        host=args.host, 
        source_port=args.source_port, 
        destination_port=args.destination_port, 
        control_port=args.control_port,
        gpu_type=args.gpu_type
    )
    agent.start()

if __name__ == '__main__':
    main()

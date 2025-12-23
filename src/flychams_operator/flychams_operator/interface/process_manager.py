"""Process manager for launching and tracking subprocesses with Qt signals"""

import subprocess
import threading
import queue
import re
import logging
from PyQt5.QtCore import QObject, pyqtSignal, QTimer

logger = logging.getLogger(__name__)

def strip_ansi_codes(text: str) -> str:
    """Remove ANSI escape codes from text"""
    ansi_escape = re.compile(r'\x1b\[[0-9;]*m')
    return ansi_escape.sub('', text)

class ProcessManager(QObject):
    """Manages background processes and routes their output to signals"""
    
    # Signal emitted when a new line of output is received: (component_name, line_text)
    line_received = pyqtSignal(str, str)
    
    def __init__(self):
        super().__init__()
        self.processes = {}  # component -> {'proc': process, 'stop_cmd': stop_cmd}
        self.log_queue = queue.Queue()
        
        # Timer to process log queue in the main thread
        self.log_timer = QTimer()
        self.log_timer.timeout.connect(self.process_log_queue)
        self.log_timer.start(50)  # Check every 50ms for responsiveness

    def start_process(self, component_name: str, cmd: list, stop_cmd: list = None):
        """Start a process if not already running"""
        if component_name in self.processes:
            proc = self.processes[component_name]['proc']
            if proc.poll() is None:
                self.log_queue.put((component_name, f"Process {component_name} is already running."))
                return

        self.log_queue.put((component_name, f"Launching {component_name}: {' '.join(cmd)}"))
        
        try:
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                bufsize=1,
                universal_newlines=False
            )
            self.processes[component_name] = {
                'proc': process,
                'stop_cmd': stop_cmd
            }
            
            # Start threads to read stdout and stderr
            threading.Thread(target=self.read_stream, args=(process.stdout, component_name, False), daemon=True).start()
            threading.Thread(target=self.read_stream, args=(process.stderr, component_name, True), daemon=True).start()
            
            self.log_queue.put((component_name, f"Started {component_name} (PID: {process.pid})"))
        except Exception as e:
            logger.error(f"Failed to start {component_name}: {e}")
            self.log_queue.put((component_name, f"ERROR: Failed to start: {e}"))

    def stop_process(self, component_name: str):
        """Stop a specific process using its stop command or terminate it"""
        if component_name not in self.processes:
            return

        process_info = self.processes[component_name]
        proc = process_info['proc']
        stop_cmd = process_info['stop_cmd']

        if proc.poll() is None:
            if stop_cmd:
                self.log_queue.put((component_name, f"Stopping {component_name} with command: {' '.join(stop_cmd)}"))
                try:
                    subprocess.run(stop_cmd, check=False)
                except Exception as e:
                    self.log_queue.put((component_name, f"ERROR running stop command: {e}"))
                    proc.terminate()
            else:
                self.log_queue.put((component_name, f"Stopping {component_name}..."))
                proc.terminate()
        
        # Remove from tracking after attempting to stop
        del self.processes[component_name]

    def stop_all(self):
        """Stop all managed processes in parallel"""
        threads = []
        # Create a list of names to avoid dictionary size change during iteration
        for name in list(self.processes.keys()):
            if not name.startswith("Operator") and not name.startswith("UE5"):
                t = threading.Thread(target=self.stop_process, args=(name,), daemon=True)
                t.start()
                threads.append(t)
        
        # Wait for all stop threads to finish to ensure they are all stopped
        for t in threads:
            t.join()
            
        self.processes.clear()

    def read_stream(self, stream, component_name, is_stderr):
        """Background thread worker to read stream lines"""
        try:
            for line in iter(stream.readline, b''):
                if line:
                    line_str = line.decode('utf-8', errors='replace').rstrip()
                    line_str = strip_ansi_codes(line_str)
                    prefix = "[STDERR] " if is_stderr else ""
                    self.log_queue.put((component_name, f"{prefix}{line_str}"))
        except Exception as e:
            self.log_queue.put((component_name, f"ERROR reading stream: {e}"))
        finally:
            stream.close()

    def process_log_queue(self):
        """Flush the queue and emit signals on the main thread"""
        while not self.log_queue.empty():
            try:
                component, message = self.log_queue.get_nowait()
                self.line_received.emit(component, message)
            except queue.Empty:
                break


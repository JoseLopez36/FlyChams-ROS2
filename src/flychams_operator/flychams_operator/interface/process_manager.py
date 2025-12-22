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
        self.processes = {}  # component -> process
        self.log_queue = queue.Queue()
        
        # Timer to process log queue in the main thread
        self.log_timer = QTimer()
        self.log_timer.timeout.connect(self._process_log_queue)
        self.log_timer.start(50)  # Check every 50ms for responsiveness

    def start_process(self, component_name: str, cmd: list):
        """Start a process if not already running"""
        if component_name in self.processes and self.processes[component_name].poll() is None:
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
            self.processes[component_name] = process
            
            # Start threads to read stdout and stderr
            threading.Thread(target=self._read_stream, args=(process.stdout, component_name, False), daemon=True).start()
            threading.Thread(target=self._read_stream, args=(process.stderr, component_name, True), daemon=True).start()
            
            self.log_queue.put((component_name, f"Started {component_name} (PID: {process.pid})"))
        except Exception as e:
            logger.error(f"Failed to start {component_name}: {e}")
            self.log_queue.put((component_name, f"ERROR: Failed to start: {e}"))

    def stop_all(self):
        """Stop all managed processes"""
        for name, proc in self.processes.items():
            if proc.poll() is None:
                self.log_queue.put((name, f"Stopping {name}..."))
                proc.terminate()
        self.processes.clear()

    def _read_stream(self, stream, component_name, is_stderr):
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

    def _process_log_queue(self):
        """Flush the queue and emit signals on the main thread"""
        while not self.log_queue.empty():
            try:
                component, message = self.log_queue.get_nowait()
                self.line_received.emit(component, message)
            except queue.Empty:
                break


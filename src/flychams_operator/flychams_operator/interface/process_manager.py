"""Process manager for launching and tracking processes"""

import logging
import re
from typing import Dict, List, Optional

from PyQt5.QtCore import QObject, pyqtSignal, QProcess

logger = logging.getLogger(__name__)

def strip_ansi_codes(text: str) -> str:
    """Remove ANSI escape codes from text"""
    ansi_escape = re.compile(r'\x1b\[[0-9;]*m')
    return ansi_escape.sub('', text)

class ProcessManager(QObject):
    """Manages background processes and routes their output to signals using QProcess"""
    
    # Signal emitted when a new line of output is received: (component_name, line_text)
    line_received = pyqtSignal(str, str)
    
    def __init__(self):
        super().__init__()
        # component -> {'proc': QProcess, 'stop_cmd': list, 'stdout_buf': str, 'stderr_buf': str}
        self.processes: Dict[str, Dict] = {}

    def start_process(self, component_name: str, cmd: List[str], stop_cmd: Optional[List[str]] = None):
        """Start a process if not already running"""
        if component_name in self.processes:
            proc = self.processes[component_name]['proc']
            if proc.state() != QProcess.NotRunning:
                self.line_received.emit(component_name, f"Process {component_name} is already running.")
                return
            else:
                # Cleanup dead process entry if it exists but isn't running
                self._cleanup_process(component_name)

        self.line_received.emit(component_name, f"Launching {component_name}: {' '.join(cmd)}")
        
        process = QProcess(self)
        process.setProcessChannelMode(QProcess.SeparateChannels)
        
        # Store process info
        self.processes[component_name] = {
            'proc': process,
            'stop_cmd': stop_cmd,
            'stdout_buf': "",
            'stderr_buf': ""
        }
        
        # Connect signals
        # Use default values in lambdas to capture current component_name
        process.readyReadStandardOutput.connect(
            lambda name=component_name: self._handle_stdout(name)
        )
        process.readyReadStandardError.connect(
            lambda name=component_name: self._handle_stderr(name)
        )
        process.finished.connect(
            lambda exit_code, exit_status, name=component_name: self._handle_finished(name, exit_code, exit_status)
        )
        process.errorOccurred.connect(
            lambda error, name=component_name: self._handle_error(name, error)
        )

        # Start the process
        program = cmd[0]
        arguments = cmd[1:]
        process.start(program, arguments)
        
        # Wait briefly to check if it failed to start immediately
        if process.waitForStarted(1000):
             self.line_received.emit(component_name, f"Started {component_name} (PID: {process.processId()})")
        else:
             # If it failed to start, errorOccurred might have already fired, but we ensure cleanup
             if component_name in self.processes:
                 self.line_received.emit(component_name, f"Failed to start {component_name}: {process.errorString()}")
                 self._cleanup_process(component_name)

    def stop_process(self, component_name: str):
        """Stop a specific process using its stop command or terminate it"""
        if component_name not in self.processes:
            return

        process_info = self.processes[component_name]
        proc = process_info['proc']
        stop_cmd = process_info['stop_cmd']

        if proc.state() != QProcess.NotRunning:
            if stop_cmd:
                self.line_received.emit(component_name, f"Stopping {component_name} with command: {' '.join(stop_cmd)}")
                # Execute stop command asynchronously
                stop_proc = QProcess(self)
                stop_proc.start(stop_cmd[0], stop_cmd[1:])
                # We let the stop command run. If it succeeds, the main process should exit,
                # triggering _handle_finished which cleans up.
                # If the stop command fails or doesn't kill the process, the process remains running.
                # This differs slightly from original which force-terminated on exception,
                # but is more robust for async operations.
            else:
                self.line_received.emit(component_name, f"Stopping {component_name}...")
                proc.terminate()
        else:
            self._cleanup_process(component_name)

    def stop_all(self):
        """Stop all managed processes"""
        # Create a list of names to avoid dictionary size change during iteration
        for name in list(self.processes.keys()):
            if not name.startswith("Operator") and not name.startswith("UE5"):
                self.stop_process(name)

    def _handle_stdout(self, component_name):
        if component_name not in self.processes:
            return
        
        proc = self.processes[component_name]['proc']
        data = proc.readAllStandardOutput().data()
        self._process_stream_data(component_name, data, is_stderr=False)

    def _handle_stderr(self, component_name):
        if component_name not in self.processes:
            return
        
        proc = self.processes[component_name]['proc']
        data = proc.readAllStandardError().data()
        self._process_stream_data(component_name, data, is_stderr=True)

    def _process_stream_data(self, component_name, data, is_stderr):
        # Decode current chunk
        text = data.decode('utf-8', errors='replace')
        
        # Append to buffer
        key = 'stderr_buf' if is_stderr else 'stdout_buf'
        full_text = self.processes[component_name][key] + text
        
        # Split lines
        lines = full_text.split('\n')
        
        # The last element is the partial line (or empty string if text ended with \n)
        self.processes[component_name][key] = lines[-1]
        
        # Process complete lines
        for line in lines[:-1]:
            clean_line = strip_ansi_codes(line.rstrip())
            prefix = "[STDERR] " if is_stderr else ""
            self.line_received.emit(component_name, f"{prefix}{clean_line}")

    def _handle_finished(self, component_name, exit_code, exit_status):
        """Handle process termination"""
        status_str = "Normal Exit" if exit_status == QProcess.NormalExit else "Crashed"
        self.line_received.emit(component_name, f"Process finished: {status_str} (Code: {exit_code})")
        self._cleanup_process(component_name)

    def _handle_error(self, component_name, error):
        """Handle process error"""
        # Only report if we haven't cleaned it up yet
        if component_name in self.processes:
            proc = self.processes[component_name]['proc']
            self.line_received.emit(component_name, f"Process error: {proc.errorString()}")

    def _cleanup_process(self, component_name):
        """Remove process from tracking"""
        if component_name in self.processes:
            del self.processes[component_name]

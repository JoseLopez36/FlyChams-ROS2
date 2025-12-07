#!/usr/bin/env python3

import libtmux
import subprocess
import threading

def stop_containers(filter_name):
    subprocess.run(f"docker ps -q --filter name={filter_name} | xargs -r docker stop", shell=True)

def main():
    session_name = "flychams"
    server = libtmux.Server()
    
    # Kill Tmux Session
    try:
        if hasattr(server, "sessions"):
            sessions = server.sessions
        else:
            sessions = server.list_sessions()
            
        for s in sessions:
            name = getattr(s, 'session_name', None) or s.get('session_name')
            if name == session_name:
                s.kill()
                print(f"Tmux session '{session_name}' killed.")
                break
        else:
            print(f"Tmux session '{session_name}' not found.")
    except Exception as e:
        print(f"Error managing tmux session: {e}")

    # Stop Docker Containers
    print("Stopping FlyChams containers...")
    try:
        # Stop all FlyChams containers in parallel
        threads = [
            threading.Thread(target=stop_containers, args=("flychams-global",)),
            threading.Thread(target=stop_containers, args=("flychams-AGENT*",)),
        ]

        for t in threads:
            t.start()
        for t in threads:
            t.join()
        
        print("FlyChams containers stopped successfully")
    except Exception as e:
        print(f"Error stopping containers: {e}")

if __name__ == "__main__":
    main()
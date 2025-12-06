#!/usr/bin/env python3

import libtmux
import subprocess

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
        # Stop all FlyChams containers
        subprocess.run("docker ps -q --filter name=flychams-* | xargs -r docker stop", shell=True)
        
        print("FlyChams containers stopped successfully")
    except Exception as e:
        print(f"Error stopping containers: {e}")

if __name__ == "__main__":
    main()
#!/usr/bin/env python3
"""
Sample script to communicate with Wwise Authoring API (WAAPI)
running on Wine on Linux.
"""
import sys
import json

try:
    from waapi import WaapiClient, CannotConnectToWaapiException
except ImportError:
    print("waapi-client not installed. Install with: pip install waapi-client")
    sys.exit(1)

def main():
    url = "ws://127.0.0.1:8080/waapi"
    print(f"Connecting to Wwise Authoring API at {url}...")
    try:
        with WaapiClient(url=url) as client:
            print("Successfully connected to WAAPI!")
            
            # Query Wwise version and info
            info = client.call("ak.wwise.core.getInfo")
            print("\nWwise Info:")
            print(json.dumps(info, indent=2))
            
            # Query Project Info
            project_info = client.call("ak.wwise.core.getProjectInfo")
            print("\nProject Info:")
            print(json.dumps(project_info, indent=2))

    except CannotConnectToWaapiException:
        print("Could not connect to WAAPI server.")
        print("Make sure Wwise or WwiseConsole waapi-server is running.")
        sys.exit(1)

if __name__ == "__main__":
    main()

# Wwise Authoring API (WAAPI) on Linux

Wwise features WAAPI, a rich RPC/pub-sub API over WebSockets and HTTP allowing external scripts, game engine editor plugins (Unreal, Unity, Godot), and CI/CD pipelines to manipulate projects and query sound engine data.

## Running WAAPI on Linux

WAAPI runs seamlessly inside the Wine prefix. Because Wine exposes the Windows TCP stack to `127.0.0.1`, native Linux tools (Python, Node.js, curl) can directly talk to WAAPI without proxies!

### 1. Start the WAAPI Server

You can run WAAPI either alongside the Wwise GUI or completely headlessly:

```bash
# Headless WAAPI server:
wwise waapi path/to/MyProject.wproj

# Or simply start the GUI and enable WAAPI in User Preferences:
wwise gui path/to/MyProject.wproj
```

### 2. Test Connection from Python

```bash
pip install -r requirements.txt
python waapi_client.py
```

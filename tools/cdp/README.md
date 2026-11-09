# Headless Launcher Automation via Chrome DevTools Protocol (CDP)

Because the Wwise Launcher is an Electron application, it can be launched with remote debugging enabled (`--remote-debugging-port=9222`) and automated via CDP without an active display session.

## Included Tools

- **`cdp.py`**: Connects via WebSocket to the launcher's renderer and evaluates JavaScript expressions in the context of the page:
  ```bash
  python3 cdp.py 'document.title'
  ```
- **`drive.js`**: Automation script to navigate the Wwise version picker and initiate unattended package downloads.

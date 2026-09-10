# Talisman bridge

Host-side BLE diagnostics for the Talisman prototype.

From this directory:

```console
uv run main.py scan
uv run main.py inspect
uv run main.py signal
uv run main.py watch --seconds 30
uv run main.py demo --seconds 15
```

- `scan` discovers the device advertised as `Talisman-V1`.
- `inspect` connects and reads the custom service's JSON `info` and `state`
  characteristics.
- `signal` writes the semantic JSON attention command. The firmware maps that
  intent to the current physical expression.
- `watch` reads the initial state and latest event, then prints live state and
  semantic-event notifications.
- `demo` runs the complete Experiment 11 path in one connection: discover,
  inspect, subscribe, send the attention signal, and display notifications.

macOS may ask for Bluetooth permission the first time the bridge runs. The
board must be powered and not connected to another BLE client.

## Experiment 09 UUIDs

| Purpose | UUID |
| --- | --- |
| Service | `7a110001-6c8d-4f4b-9f3a-45dcd0a6b001` |
| Info | `7a110002-6c8d-4f4b-9f3a-45dcd0a6b001` |
| State | `7a110003-6c8d-4f4b-9f3a-45dcd0a6b001` |
| Command | `7a110004-6c8d-4f4b-9f3a-45dcd0a6b001` |
| Events | `7a110005-6c8d-4f4b-9f3a-45dcd0a6b001` |
| Config | `7a110006-6c8d-4f4b-9f3a-45dcd0a6b001` |

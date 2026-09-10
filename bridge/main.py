import argparse
import asyncio
import json

from bleak import BleakClient, BleakScanner


DEVICE_NAME = "Talisman-V1"
SERVICE_UUID = "7a110001-6c8d-4f4b-9f3a-45dcd0a6b001"
INFO_UUID = "7a110002-6c8d-4f4b-9f3a-45dcd0a6b001"
STATE_UUID = "7a110003-6c8d-4f4b-9f3a-45dcd0a6b001"
COMMAND_UUID = "7a110004-6c8d-4f4b-9f3a-45dcd0a6b001"
ATTENTION_COMMAND = {"action": "signal", "pattern": "attention"}


async def find_talisman(timeout: float):
    print(f"Scanning for {DEVICE_NAME} for up to {timeout:.0f} seconds...")
    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=timeout)
    if device is None:
        raise RuntimeError(f"{DEVICE_NAME} was not discovered")
    print(f"Discovered {device.name} ({device.address})")
    return device


async def inspect_talisman(timeout: float) -> None:
    device = await find_talisman(timeout)
    async with BleakClient(device) as client:
        print("Connected")
        service_uuids = {service.uuid.lower() for service in client.services}
        if SERVICE_UUID not in service_uuids:
            raise RuntimeError("Talisman service was not exposed")
        info = json.loads((await client.read_gatt_char(INFO_UUID)).decode())
        state = json.loads((await client.read_gatt_char(STATE_UUID)).decode())
        print(f"Info: {json.dumps(info, sort_keys=True)}")
        print(f"State: {json.dumps(state, sort_keys=True)}")


async def signal_talisman(timeout: float) -> None:
    device = await find_talisman(timeout)
    payload = json.dumps(ATTENTION_COMMAND, separators=(",", ":")).encode()
    async with BleakClient(device) as client:
        print("Connected")
        await client.write_gatt_char(COMMAND_UUID, payload, response=True)
        print(f"Command sent: {payload.decode()}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Talisman BLE diagnostic bridge")
    parser.add_argument("command", choices=("scan", "inspect", "signal"))
    parser.add_argument("--timeout", type=float, default=10.0)
    return parser.parse_args()


async def run() -> None:
    args = parse_args()
    if args.command == "scan":
        await find_talisman(args.timeout)
    elif args.command == "inspect":
        await inspect_talisman(args.timeout)
    else:
        await signal_talisman(args.timeout)


def main() -> None:
    try:
        asyncio.run(run())
    except (RuntimeError, OSError) as error:
        raise SystemExit(f"Error: {error}") from error


if __name__ == "__main__":
    main()

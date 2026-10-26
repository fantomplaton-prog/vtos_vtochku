__all__ = ['list_devices', 'ble_find_indy_address']

from serial.tools import list_ports
import pandas as pd

def list_devices() -> pd.DataFrame:
    """
    Lists all connected devices.

    Returns:
        pd.DataFrame: A DataFrame containing details of all connected devices.
    """
    df_ports = pd.DataFrame([c.__dict__ for c in list_ports.comports()])
    return df_ports

from bleak import BleakScanner
async def ble_find_indy_address() -> str:
    """
    Asynchronously finds the address of an Indy device using BLE.

    Returns:
        str: The address of the found Indy device, or None if no device is found.

    Example usage:
    
    import asyncio
    
    async def main():
        address = await ble_find_indy_address()
        if address:
            print(f"Indy device found at address: {address}")
        else:
            print("No Indy device found.")
    
    asyncio.run(main())
        """
    device = await BleakScanner.find_device_by_name('INDY')
    if device is None:
        print('Found no Indy devices')
    else:
        print(f'Found Indy device {device.address}')
    return device.address if device else None

async def ble_find_fest_address() -> list:
    """
    Asynchronously finds the addresses of FEST devices using BLE.

    Returns:
        list: A list of tuples, each containing the UUID and the device object for each FEST device found.

    Example usage:
    
    import asyncio
    
    async def main():
        fest_devices = await ble_find_fest_address()
        if fest_devices:
            for uuid, device in fest_devices:
                print(f"FEST device found: {device.name} at UUID: {uuid}")
        else:
            print("No FEST devices found.")
    
    asyncio.run(main())
    """
    from bleak import BleakScanner
    devices = await BleakScanner.discover(
        return_adv=True
    )
    
    return [(uuid, dev) 
            for uuid, (dev, adv) in devices.items() 
            if dev.name and dev.name.startswith("FEST")]

import threading
from bleak import BleakClient
import asyncio

class BleakClientThread:
    """
    A class to manage asynchronous BLE operations using Bleak and threading.
    see ref at https://github.com/hbldh/bleak

    Attributes:
        address (str): The BLE device address.
        loop (asyncio.new_event_loop): The event loop for asynchronous operations.
        client (BleakClient): The Bleak BLE client.
        thread (threading.Thread): The thread running the event loop.
        connected (bool): Connection status of the BLE client.

    Methods:
        run_loop(loop): Runs the given event loop.
        connect(): Asynchronously connects to the BLE device.
        disconnect(): Asynchronously disconnects from the BLE device.
        read_characteristic(char_uuid): Reads data from a specified characteristic.
        write_characteristic(char_uuid, data): Writes data to a specified characteristic.
        start(): Starts the thread and the event loop.
        stop(): Stops the event loop and joins the thread.
        perform_action(coro): Schedules a coroutine to be run in the event loop thread.
        prot_open(): Placeholder for opening protocol.
        prot_close(): Placeholder for closing protocol.
        prot_stop(): Placeholder for stopping protocol.
    """
    def __init__(self, address):
        """
        Initializes the BleakClientThread with a given BLE device address.

        Parameters:
            address (str): The BLE device address.
        """

        self.address = address
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)  # Set the event loop for this thread.
        self.client = BleakClient(address, loop=self.loop)  # Ensure BleakClient uses this loop.
        self.thread = threading.Thread(target=self.loop.run_forever)
        self.thread.daemon = True
        self.connected = False

    def run_loop(self, loop):
        """
        Runs the given event loop.

        Parameters:
            loop (asyncio.new_event_loop): The event loop to run.
        """
        asyncio.set_event_loop(loop)
        loop.run_forever()

    async def connect(self):
        """
        Asynchronously connects to the BLE device.

        Returns:
            bool: True if the connection was successful, False otherwise.
        """
        self.connected = await self.client.connect()
        return self.connected

    async def disconnect(self):
        """
        Asynchronously disconnects from the BLE device.
        """
        await self.client.disconnect()
        self.connected = False

    async def read_characteristic(self, char_uuid):
        """
        Reads data from a specified characteristic.

        Parameters:
            char_uuid (str): The UUID of the characteristic to read from.

        Returns:
            The data read from the characteristic.
        """
        return await self.client.read_gatt_char(char_uuid)

    async def write_characteristic(self, char_uuid, data):
        """
        Writes data to a specified characteristic.

        Parameters:
            char_uuid (str): The UUID of the characteristic to write to.
            data: The data to write.
        """
        await self.client.write_gatt_char(char_uuid, data)

    def start(self):
        """
        Starts the thread and the event loop.
        """
        self.thread.start()

    def stop(self):
        """
        Stops the event loop and joins the thread.
        """
        self.loop.call_soon_threadsafe(self.loop.stop)
        self.thread.join()

    def perform_action(self, coro):
        """
        Schedules a coroutine to be run in the event loop thread.

        Parameters:
            coro: The coroutine to schedule.

        Returns:
            The result of the coroutine, or None if an error occurred.
        """
        fut = asyncio.run_coroutine_threadsafe(coro, self.loop)
        try:
            result = fut.result()
            return result
        except Exception as e:
            print(f"An error occurred: {e}")
            return None

    def prot_open(self):
        """
        Placeholder for opening protocol. Not implemented yet.
        """
        raise NotImplementedError("prot_open function not implemented yet")

    def prot_close(self):
        """
        Placeholder for closing protocol. Not implemented yet.
        """
        raise NotImplementedError("prot_close function not implemented yet")

    def prot_stop(self):
        """
        Placeholder for stopping protocol. Not implemented yet.
        """
        raise NotImplementedError("prot_stop function not implemented yet")

# Example usage
if __name__ == "__main__":
    device_address = "Device_Address_Here"
    characteristic_uuid = "Characteristic_UUID_Here"
    data_to_write = bytearray([0x00, 0x01])

    ble_client = BleakClientThread(device_address)
    ble_client.start()

    try:
        # Connect to the device
        connected = ble_client.perform_action(ble_client.connect())
        if connected:
            print("Connected to the device.")
            ble_client.connected = True

            # Perform read
            data = ble_client.perform_action(ble_client.read_characteristic(characteristic_uuid))
            print(f"Read data: {data}")

            # Perform write
            ble_client.perform_action(ble_client.write_characteristic(characteristic_uuid, data_to_write))
            print(f"Wrote data: {data_to_write}")

            # Disconnect
            ble_client.perform_action(ble_client.disconnect())
            print("Disconnected.")
    except Exception as e:
        print(f"Failed to perform BLE operations: {e}")
    finally:
        ble_client.stop()
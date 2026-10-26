from .motorica_prot_ble_interface import BleakClientThread

class IndyClientThread(BleakClientThread):
    """
    A class to manage specific BLE operations for Indy devices using the BleakClientThread as a base.

    Attributes:
        OPEN_MOTOR_HDLE (str): UUID for opening the motor characteristic.
        CLOSE_MOTOR_HDLE (str): UUID for closing the motor characteristic.
        data_go (bytearray): Data to send to initiate an action.
        data_nogo (bytearray): Data to send to stop an action.
    """
    def __init__(self, address):
        """
        Initializes the IndyClientThread with a given BLE device address.

        Parameters:
            address (str): The BLE device address.
        """
        super().__init__(address)
        self.OPEN_MOTOR_HDLE = "0000fe48-8e22-4541-9d4c-21edae82ed19"
        self.CLOSE_MOTOR_HDLE = "0000fe49-8e22-4541-9d4c-21edae82ed19"
        self.data_go = bytearray([0x01, 0x00])
        self.data_nogo = bytearray([0x00, 0x00])
        
    def prot_close(self):
        """
        Initiates the protocol to close the motor by writing to the CLOSE_MOTOR_HDLE characteristic.
        """
        self.perform_action(self.write_characteristic(self.CLOSE_MOTOR_HDLE, self.data_go))

    def prot_open(self):
        """
        Initiates the protocol to open the motor by writing to the OPEN_MOTOR_HDLE characteristic.
        """
        self.perform_action(self.write_characteristic(self.OPEN_MOTOR_HDLE, self.data_go))

    def prot_stop(self):
        """
        Stops any ongoing motor action by writing to both the OPEN_MOTOR_HDLE and CLOSE_MOTOR_HDLE characteristics.
        """
        self.perform_action(self.write_characteristic(self.CLOSE_MOTOR_HDLE, self.data_nogo))
        self.perform_action(self.write_characteristic(self.OPEN_MOTOR_HDLE, self.data_nogo))

# Example usage
if __name__ == "__main__":
    device_address = "Indy_Device_Address_Here"
    
    indy_client = IndyClientThread(device_address)
    indy_client.start()

    try:
        # Connect to the Indy device
        connected = indy_client.perform_action(indy_client.connect())
        if connected:
            print("Connected to the Indy device.")
            indy_client.connected = True

            # Open the motor
            indy_client.perform_action(indy_client.prot_open())
            print("Motor opened.")

            # Close the motor
            indy_client.perform_action(indy_client.prot_close())
            print("Motor closed.")

            # Stop any motor action
            indy_client.perform_action(indy_client.prot_stop())
            print("Motor action stopped.")

            # Disconnect
            indy_client.perform_action(indy_client.disconnect())
            print("Disconnected from the Indy device.")
    except Exception as e:
        print(f"Failed to perform Indy BLE operations: {e}")
    finally:
        indy_client.stop()

import MotoricaInterface as mi
from MotoricaInterface.ubi4 import ubi4_enum_description

print()
def test_version():
    print("\r\n")
    assert mi.__version__ == '0.0.8' # should be updated each time changes occur
    print("mi.__version__", mi.__version__)

def test_crc():
    print("\r\n")
    crc_test = mi.get_crc([0x01, 0x02, 0x03, 0x04], 4)
    print(crc_test)
    assert crc_test == 244
    
def test_have_omg():
    print("\r\n")
    
    ubi4_stack = mi.ubi4.Ubi4Stack('/dev/cu.usbmodem1234561')
    ubi4_stack.initialize_stack()
    print(ubi4_stack)
    for sub_device in ubi4_stack.sub_devices_array:
        print('--------------SUBDEVICE------------------')
        print(sub_device)
        
    assert "DCE_OMG_MODULE" in [ubi4_enum_description.device_codes_enum(sub_device.DeviceCode).name 
                                for sub_device in ubi4_stack.sub_devices_array], "DCE_OMG_MODULE not found"
    
    print("omg device found")
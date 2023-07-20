import argparse
import subprocess
from serial import Serial
import serial.tools.list_ports as list_ports
import time

def read(port):
    output = port.readline()
    return output.decode("utf-8")

def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("--hex", help="Path to the application hex file prepared for nrf53 crash reproduction", type=str, default="app.hex")
    parser.add_argument("--snr", type=int, help="JLink serial number of the connected nRF53 device", required=True)
    parser.add_argument("--flash", action="store_true", help="Flash the device before testing")
    parser.add_argument("-i", "--iterations", help="Interaction no. to reproduce crash. If crash cannot be reproduced increase this value", required=True, type=int)
    args = parser.parse_args()

    # Flash device
    if args.flash:
        print(f"\nClearing and flashing nRF53 DK: {args.snr} file: {args.hex}\n")
        cmd = ["nrfjprog", "-f", "nrf53", "--snr", f"{args.snr}", "--program", f"{args.hex}", "--recover", "--verify", "--reset"]
        result = subprocess.run(cmd, stderr = subprocess.PIPE)
        if(result.stderr):
            print("Cannot flash the device")

    # Find comport
    port = ""
    for comport in list_ports.comports():
        try:
            if int(comport.serial_number) == args.snr and int(comport.location[-1]) == 2:
                port = comport.device
        except ValueError:
            continue
        except TypeError:
            continue
    
    print("Start testing...\n")
    with Serial(port, 115200, timeout=1) as device:
        # Reset device until crash
        for i in range(0, args.iterations):
            print(f"\n Iteration no. {i}")
            subprocess.call(["nrfjprog", "--snr", f"{args.snr}", "--pinreset"])
            time.sleep(6)
            device.readlines()
            device.write("ot state\n".encode("utf-8"))
            time.sleep(4)
            lines_read = device.readlines()
            for line in lines_read:
                if line.decode("utf-8").find("No response within timeout") != -1:
                    print("\n CRASH DETECTED!")
                    return
            device.flush()
            print("\n NO CRASH")

if __name__ == '__main__':
    main()

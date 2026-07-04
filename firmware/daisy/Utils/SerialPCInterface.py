import serial
import time
import sys


def drain_response(ser, timeout=0.75, idle_timeout=0.15):
    deadline = time.time() + timeout
    idle_deadline = time.time() + idle_timeout
    chunks = []

    while time.time() < deadline:
        waiting = ser.in_waiting
        if waiting > 0:
            chunks.append(ser.read(waiting))
            idle_deadline = time.time() + idle_timeout
        elif chunks and time.time() >= idle_deadline:
            break
        else:
            time.sleep(0.01)

    if chunks:
        print(b''.join(chunks).decode('utf-8', errors='replace'), end='')

# Open the serial port
# Adjust 'COM9' or 115200 to match your device setup
try:
    ser = serial.Serial('COM9', 115200, timeout=0.1) # Shorter timeout for faster reading
    print("--- Connected to COM9. Type your commands below. ---")
    print("--- Type 'exit' or press Ctrl+C to stop. ---\n")
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    sys.exit()

try:
    while True:
        # 1. Look for and read any incoming data from the device
        if ser.in_waiting > 0:
            # Read all available bytes in the buffer
            incoming_data = ser.read(ser.in_waiting)
            # Decode bytes to string and print (errors='ignore' prevents crashes on weird bytes)
            print(incoming_data.decode('utf-8', errors='ignore'), end='')

        # 2. Get live input from you
        # We use a tiny sleep so it doesn't max out your CPU while idling
        time.sleep(0.05) 
        
        # Check if you've typed anything
        # Note: Standard input() blocks the loop, so the script will read 
        # incoming data *right after* you send a command.
        command = input("Send command -> ")
        
        if command.strip().lower() == 'exit':
            break
            
        if command:
            # Ensure the command ends with a newline, convert to bytes, and send
            if not command.endswith('\n'):
                command += '\n'
            ser.write(command.encode('utf-8'))
            drain_response(ser)

except KeyboardInterrupt:
    print("\nStopping live terminal...")

finally:
    # Always clean up and close the port safely
    ser.close()
    print("Serial port closed.")
import serial
import asyncio
from azure.iot.device.aio import IoTHubDeviceClient

async def main():
    device_client = IoTHubDeviceClient.create_from_connection_string("")
    await device_client.connect()
    s = serial.Serial("/dev/ttyACM0", 9600)

    try:
        while True:
            message_from_arduino = s.readline().decode("utf-8")
            print("Received message from arduino: " + message_from_arduino)
            
            print("Sending message to IoT Hub...")
            await device_client.send_message(message_from_arduino)
            print("Message successfully sent!")
    except KeyboardInterrupt:
        await device_client.shutdown()
        s.close()

if __name__ == "__main__":
    asyncio.run(main())


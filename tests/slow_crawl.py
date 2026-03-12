#!/usr/bin/env python3
import socket
import threading
import time

TARGET_IP = "127.0.0.1"  # Replace with your server IP
TARGET_PORT = 8080         # Replace with your server port
NUM_CONNECTIONS = 100     # Number of slow connections to open
DELAY = 21               # Delay between sending chunks (seconds)
TIMEOUT = 30             # Max time to wait for server response (seconds)

def slow_connect():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(TIMEOUT)  # Set socket timeout
        s.connect((TARGET_IP, TARGET_PORT))

        # Send partial request slowly
        s.send(b"GET /index.html HTTP/1.1\r\n")
        time.sleep(DELAY)
        s.send(b"Host: example.com\r\n")
        time.sleep(DELAY)
        s.send(b"Cookie: uuid=sdjfsdfjsdf\r\n")
        time.sleep(DELAY)
        s.send(b"\r\n")

        # Try to read the server's response
        response = s.recv(4096).decode()
        if "408" in response:
            print("✅ Success: Server responded with 408 (Request Timeout)")
        else:
            print(f"❌ Unexpected response: {response[:100]}...")

    except socket.timeout:
        print("⏳ Connection timed out (server did not respond in time)")
    except BrokenPipeError:
        print("🔧 Broken pipe (server closed the connection)")
    except Exception as e:
        print(f"⚠️ Error: {e}")
    finally:
        s.close()

# Launch threads
for i in range(NUM_CONNECTIONS):
    threading.Thread(target=slow_connect).start()
    time.sleep(0.1)  # Stagger thread starts

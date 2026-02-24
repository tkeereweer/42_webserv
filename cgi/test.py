import sys
import os

print("CGI START")

print("--- ENV ---")
for key, value in os.environ.items():
    print(f"{key}={value}")

print("--- BODY ---")
body = sys.stdin.read()
print(body)

print("CGI END")

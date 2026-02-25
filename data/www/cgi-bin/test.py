import sys
import os

print("Status: 200 OK") #Status: header to be parsed by server to ensure CGI doesn't have error codes of its own
print("Content-Type: text/plain") #Content-Type always handled by CGI
print() #newline that separates headers and body
print("--- ENV ---")
for key, value in os.environ.items():
    print(f"{key}={value}")
print("This is the standard text response.")
print("The server will add the 'HTTP/1.1 200 OK' for me!")

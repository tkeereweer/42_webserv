import sys
import os


print("content-type: text/plain") #Content-Type always handled by CGI
print("status: 200")
print() #newline that separates headers and body
print("here start the env vars")
for key, value in os.environ.items():
    print(f"{key}={value}")
print("This is the standard text response.")
print("The server will add the 'HTTP/1.1 200 OK' for me!")

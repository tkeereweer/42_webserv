import sys
import os
from urllib.parse import parse_qs

# Configuration
LOG_FILE_PATH = "/home/mkeerewe/42/rank05/webserv_perso/data/upload/customer_list.log"
HEADERS = [
    "First Name", "Last Name", "Sex", "Email Address", 
    "Marketing", "Annoying Status", "About You"
]

def handle_post():
    # 1. Ensure the directory exists
    os.makedirs(os.path.dirname(LOG_FILE_PATH), exist_ok=True)

    # 2. Check if file exists to determine if we need headers
    file_exists = os.path.isfile(LOG_FILE_PATH)

    # 3. Read the body from stdin
    # CGI POST data is passed through stdin
    try:
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
        body = sys.stdin.read(content_length)
    except (ValueError, EOFError):
        body = ""

    # Parse the form data
    parsed_data = parse_qs(body)

    # Helper to get first value or default
    get_val = lambda key, default="": parsed_data.get(key, [default])[0]

    # 4. Extract and clean data
    first_name = get_val("firstname")
    last_name = get_val("lastname")
    sex = get_val("sex")
    email = get_val("email")
    about_you = get_val("about_you")[:200]  # Hard limit to 200 chars
    
    # Checkboxes only appear in the POST body if they are checked
    marketing = "Yes" if "marketing" in parsed_data else "No"
    annoying = "True" if "annoying_status" in parsed_data else "False"

    # 5. Write to the file
    with open(LOG_FILE_PATH, "a", encoding="utf-8") as f:
        # If it's a new file, write the header row
        if not file_exists:
            f.write("\t".join(HEADERS) + "\n")
        
        # Prepare the entry
        entry = [
            first_name, last_name, sex, email, 
            marketing, annoying, about_you.replace("\n", " ") # Keep it on one line
        ]
        f.write("\t".join(entry) + "\n")

if __name__ == "__main__":
    handle_post()
    # Basic CGI response header
    print("Content-Type: text/html\n")
    print("<html><body><h2>Submission received.</h2></body></html>")

print("POST FORM HANDLE END")
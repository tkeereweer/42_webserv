import os
import sys
from urllib.parse import parse_qs

# print("GET FORM HANDLE START")

# print("--- ENV ---")
# for key, value in os.environ.items():
#     print(f"{key}={value}")

# print("--- BODY ---")

# Configuration
LOG_FILE_PATH = "/workspace/data/upload/customer_list.log"
# Defining the columns exactly as requested
HEADERS = [
    "First Name",
    "Last Name",
    "Sex",
    "Email Address",
    "marketing",
    "annoying_status",
    "about you",
]


def handle_get_request():
    # 1. Ensure the directory exists
    os.makedirs(os.path.dirname(LOG_FILE_PATH), exist_ok=True)

    # 2. Check if file exists (to write headers if it's new)
    file_is_new = not os.path.exists(LOG_FILE_PATH)

    # 3. Grab data from the environment variable
    query_string = os.environ.get("QUERY_STRING", "")
    parsed_data = parse_qs(query_string)

    # Helper to extract values (parse_qs returns lists)
    get_val = lambda key, default="": parsed_data.get(key, [default])[0]

    # 4. Extract and format fields
    first_name = get_val("firstname")
    last_name = get_val("lastname")
    sex = get_val("sex")
    email = get_val("email")
    about_you = get_val("about_you")[:200]  # Enforce 200 character limit

    # Checkbox logic: if it's in the query string, it was checked
    marketing = "yes" if "marketing" in parsed_data else "no"
    annoying = "true" if "annoying_status" in parsed_data else "false"

    # 5. Write to the ASCII log (using Tabs to maintain "columns")
    with open(LOG_FILE_PATH, "a", encoding="utf-8") as f:
        if file_is_new:
            f.write("\t".join(HEADERS) + "\n")

        # We replace newlines in 'about_you' to keep the ASCII row structure intact
        clean_about = about_you.replace("\n", " ").replace("\r", " ")

        row = [first_name, last_name, sex, email, marketing, annoying, clean_about]
        f.write("\t".join(row) + "\n")


if __name__ == "__main__":
    # Standard CGI response
    print("Content-Type: text/html\n")

    try:
        handle_get_request()
        print("<html><body><h1>Data Logged Successfully (GET)</h1></body></html>")
    except Exception as e:
        print(f"<html><body><h1>Error</h1><p>{str(e)}</p></body></html>")

print("GET FORM HANDLE END")

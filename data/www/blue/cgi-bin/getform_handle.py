import os
import sys
from urllib.parse import parse_qs

# print("GET FORM HANDLE START")

# print("--- ENV ---")
# for key, value in os.environ.items():
#     print(f"{key}={value}")

# print("--- BODY ---")

# Configuration
LOG_FILE_PATH = "./data/upload/customer_list.log" #MAKE ENV VAR !!! SUCH THAT WE DONT NEED TO WRITE PATH AGAIN
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


PAGE_TEMPLATE = """<!doctype html>
<html lang="en">
    <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>User Submission (GET) - At your Webservice</title>
        <link rel="stylesheet" href="../css/style.css" />
    </head>
    <body>
        <header>
            <span class="header-title">
                <a href="/index.html">At your Webservice</a>
            </span>
        </header>
        <main>
            <section class="form-container">
                <h1>Submit via GET</h1>
                <p class="message {status_class}">{message}</p>
                <form action="/cgi-bin/getform_handle.py" method="GET">
                    <div class="form-group">
                        <label for="fname">First Name:</label>
                        <input type="text" id="fname" name="firstname" required />
                    </div>
                    <div class="form-group">
                        <label for="lname">Last Name:</label>
                        <input type="text" id="lname" name="lastname" required />
                    </div>
                    <div class="form-group">
                        <label for="sex">Sex:</label>
                        <select id="sex" name="sex">
                            <option value="male">Male</option>
                            <option value="female">Female</option>
                            <option value="other">Other</option>
                            <option value="prefer-not-to-say">Prefer not to say</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label for="email">Email Address:</label>
                        <input type="email" id="email" name="email" required />
                    </div>
                    <div class="form-group">
                        <label for="about">About You (Optional):</label><br />
                        <textarea id="about" name="about_you" rows="4" cols="50" placeholder="Tell us something... (not too much as query parameters are limited in length)"></textarea>
                    </div>
                    <div class="checkbox-group">
                        <input type="checkbox" id="marketing" name="marketing" value="yes" />
                        <label for="marketing">Receive marketing emails</label>
                    </div>
                    <div class="checkbox-group">
                        <input type="checkbox" id="annoying" name="annoying_status" value="true" />
                        <label for="annoying">Identify me as an annoying customer (30% longer response time from customer service)</label>
                    </div>
                    <div class="form-actions">
                        <button type="submit" class="submit-btn">Send</button>
                    </div>
                </form>
            </section>
            <p><a href="/index.html">← Back to Home</a></p>
        </main>
    </body>
</html>"""

if __name__ == "__main__":
    print("Content-Type: text/html\n")

    try:
        handle_get_request()
        print(PAGE_TEMPLATE.format(status_class="success", message="Data logged successfully!"))
    except Exception as e:
        print(PAGE_TEMPLATE.format(status_class="error", message="Error: " + str(e)))

import os
import sys
from urllib.parse import parse_qs

# Configuration
LOG_FILE_PATH = "/home/mkeerewe/42/rank05/webserv_perso/data/upload/customer_list.log"
HEADERS = [
    "First Name",
    "Last Name",
    "Sex",
    "Email Address",
    "Marketing",
    "Annoying Status",
    "About You",
]


def handle_post():
    # 1. Ensure the directory exists
    os.makedirs(os.path.dirname(LOG_FILE_PATH), exist_ok=True)

    # 2. Check if file exists to determine if we need headers
    file_exists = os.path.isfile(LOG_FILE_PATH)

    # 3. Read the body from stdin
    # CGI POST data is passed through stdin
    try:
        content_length = int(os.environ.get("CONTENT_LENGTH", 0))
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
            first_name,
            last_name,
            sex,
            email,
            marketing,
            annoying,
            about_you.replace("\n", " "),  # Keep it on one line
        ]
        f.write("\t".join(entry) + "\n")


PAGE_TEMPLATE = """<!doctype html>
<html lang="en">
    <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>User Submission - At your Webservice</title>
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
                <h1>Submit Your Details</h1>
                <p class="message {status_class}">{message}</p>
                <form action="../cgi-bin/postform_handle.py" method="POST">
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
                        <textarea id="about" name="about_you" rows="4" cols="50" placeholder="Tell us something..."></textarea>
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
        handle_post()
        print(PAGE_TEMPLATE.format(status_class="success", message="Submission received!"))
    except Exception as e:
        print(PAGE_TEMPLATE.format(status_class="error", message="Error: " + str(e)))

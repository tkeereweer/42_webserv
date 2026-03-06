import sys
import os

UPLOAD_DIR = "data/upload"

def parse_multipart(data):
    # The first line is --<boundary>
    first_end = data.find(b'\r\n')
    if first_end == -1:
        return None, None
    boundary = data[:first_end]  # e.g. b'--someboundary'

    parts = data.split(boundary)
    for part in parts[1:]:
        if part in (b'--\r\n', b'--'):
            continue
        if part.startswith(b'\r\n'):
            part = part[2:]
        header_end = part.find(b'\r\n\r\n')
        if header_end == -1:
            continue
        headers_raw = part[:header_end].decode('utf-8', errors='replace')
        content = part[header_end + 4:]
        for suffix in (b'\r\n--', b'\r\n'):
            if content.endswith(suffix):
                content = content[:-len(suffix)]
                break

        filename = None
        for line in headers_raw.split('\r\n'):
            if 'Content-Disposition' in line:
                for item in line.split(';'):
                    item = item.strip()
                    if item.startswith('filename='):
                        filename = item[9:].strip('"')
        if filename:
            return filename, content

    return None, None

def handle_upload():
    try:
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
        raw = sys.stdin.buffer.read(content_length)
    except (ValueError, EOFError):
        raw = b''

    filename, content = parse_multipart(raw)

    if not filename or content is None:
        print("Content-Type: text/html\n")
        print("""<!doctype html>
<html lang="en">
    <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>At your Webservice — Files</title>
        <link rel="stylesheet" href="../css/style.css" />
    </head>
    <body>
        <header>
            <span class="header-title"><a href="/index.html">At your Webservice</a></span>
        </header>
        <main>
            <h1 class="page-title">FILES</h1>
            <div style="display:flex;flex-direction:column;gap:24px;width:100%;max-width:600px;">
                <p class="message error">Error: no file received.</p>
                <form class="upload-form" action="/cgi-bin/upload_handle.py" method="POST" enctype="multipart/form-data">
                    <label class="file-label" for="myfile" id="file-label">Choose file</label>
                    <input type="file" id="myfile" name="myfile" />
                    <button type="submit" class="upload-btn" id="upload-btn" disabled>Upload</button>
                </form>
            </div>
        </main>
        <script>
            document.getElementById('myfile').addEventListener('change', function() {
                var hasFile = this.files.length > 0;
                document.getElementById('file-label').textContent = hasFile ? this.files[0].name : 'Choose file';
                document.getElementById('upload-btn').disabled = !hasFile;
            });
        </script>
    </body>
</html>""")
        return

    os.makedirs(UPLOAD_DIR, exist_ok=True)
    # Sanitize filename
    filename = os.path.basename(filename)
    dest = os.path.join(UPLOAD_DIR, filename)
    with open(dest, 'wb') as f:
        f.write(content)

    print("Content-Type: text/html\n")
    print("""<!doctype html>
<html lang="en">
    <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>At your Webservice — Files</title>
        <link rel="stylesheet" href="../css/style.css" />
    </head>
    <body>
        <header>
            <span class="header-title"><a href="/index.html">At your Webservice</a></span>
        </header>
        <main>
            <h1 class="page-title">FILES</h1>
            <div style="display:flex;flex-direction:column;gap:24px;width:100%;max-width:600px;">
                <p class="message success">Saved: {filename} ({size} bytes)</p>
                <form class="upload-form" action="/cgi-bin/upload_handle.py" method="POST" enctype="multipart/form-data">
                    <label class="file-label" for="myfile" id="file-label">Choose file</label>
                    <input type="file" id="myfile" name="myfile" />
                    <button type="submit" class="upload-btn" id="upload-btn" disabled>Upload</button>
                </form>
            </div>
        </main>
        <script>
            document.getElementById('myfile').addEventListener('change', function() {{
                var hasFile = this.files.length > 0;
                document.getElementById('file-label').textContent = hasFile ? this.files[0].name : 'Choose file';
                document.getElementById('upload-btn').disabled = !hasFile;
            }});
        </script>
    </body>
</html>""".format(filename=filename, size=len(content)))

if __name__ == "__main__":
    handle_upload()

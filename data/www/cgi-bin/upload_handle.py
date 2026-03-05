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
        if content.endswith(b'\r\n'):
            content = content[:-2]

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
        print("<html><body><h2>Error: no file received.</h2><a href='../pages/files.html'>Back</a></body></html>")
        return

    os.makedirs(UPLOAD_DIR, exist_ok=True)
    # Sanitize filename
    filename = os.path.basename(filename)
    dest = os.path.join(UPLOAD_DIR, filename)
    with open(dest, 'wb') as f:
        f.write(content)

    print("Content-Type: text/html\n")
    print("<html><body>")
    print("<h2>Upload successful!</h2>")
    print("<p>Saved: <strong>{}</strong> ({} bytes)</p>".format(filename, len(content)))
    print("<a href='../pages/files.html'>Upload another</a>")
    print("</body></html>")

if __name__ == "__main__":
    handle_upload()

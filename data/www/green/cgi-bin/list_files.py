import os
from urllib.parse import parse_qs

UPLOAD_DIR = "data/upload"

def list_files():
    # os.makedirs(UPLOAD_DIR, exist_ok=True)
    files = sorted(os.listdir(UPLOAD_DIR))

    qs = parse_qs(os.environ.get("QUERY_STRING", ""))
    message = ""
    if "success" in qs:
        message = '<p class="message success">File uploaded successfully!</p>'
    elif "error" in qs:
        message = '<p class="message error">Error: {}</p>'.format(qs["error"][0])

    print("Status: 200")
    print("Content-Type: text/html\n")
    print("""<!doctype html>
<html lang="en">
<head>
    <meta charset="UTF-8"/>
    <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
    <title>At your Webservice — Files</title>
    <link rel="stylesheet" href="../css/style.css"/>
    <style>
        .files-container { display: flex; flex-direction: column; gap: 32px; width: 100%; max-width: 600px; }
        .file-list { list-style: none; padding: 0; width: 100%; }
        .file-item {
            display: flex; align-items: center; justify-content: space-between;
            padding: 14px 0; border-bottom: 1px solid #2a3d26;
        }
        .file-item:first-child { border-top: 1px solid #2a3d26; }
        .file-link { color: #c8ddb8; text-decoration: none; font-size: 0.95rem; transition: color 0.12s; }
        .file-link:hover { color: #87c67a; }
        .delete-btn {
            cursor: pointer; color: #567055; background: none; border: none;
            font-family: inherit; font-size: 0.9rem; font-weight: 600;
            letter-spacing: 0.05em; padding: 4px 8px; transition: color 0.12s;
        }
        .delete-btn:hover { color: #e06c75; }
        .empty-msg { color: #567055; font-size: 0.95rem; }
    </style>
</head>
<body>
    <header>
        <span class="header-title"><a href="/index.html">At your Webservice</a></span>
    </header>
    <main>
        <h1 class="page-title">FILES</h1>
        <div class="files-container">""")

    if message:
        print("            " + message)

    print("""            <form class="upload-form" action="/cgi-bin/upload_handle.py" method="POST" enctype="multipart/form-data">
                <label class="file-label" for="myfile" id="file-label">Choose file</label>
                <input type="file" id="myfile" name="myfile" />
                <button type="submit" class="upload-btn" id="upload-btn" disabled>Upload</button>
            </form>""")

    if not files:
        print('            <p class="empty-msg">No files uploaded yet.</p>')
    else:
        print('            <ul class="file-list">')
        for f in files:
            safe = f.replace("'", "\\'")
            print('                <li class="file-item">')
            print('                    <a class="file-link" href="/upload/{}" download>{}</a>'.format(f, f))
            print('                    <button class="delete-btn" onclick="deleteFile(\'{}\')">[ delete ]</button>'.format(safe))
            print('                </li>')
        print('            </ul>')

    print("""        </div>
        <a href="/index.html" class="back-home">← Home</a>
    </main>
    <script>
        document.getElementById('myfile').addEventListener('change', function() {
            var hasFile = this.files.length > 0;
            document.getElementById('file-label').textContent = hasFile ? this.files[0].name : 'Choose file';
            document.getElementById('upload-btn').disabled = !hasFile;
        });
        function deleteFile(filename) {
            fetch('/upload/' + encodeURIComponent(filename), { method: 'DELETE' })
                .then(function(r) {
                    if (r.ok || r.status === 204 || r.status === 200)
                        location.reload();
                    else
                        alert('Delete failed: ' + r.status);
                })
                .catch(function(e) { alert('Error: ' + e); });
        }
    </script>
</body>
</html>""")

if __name__ == "__main__":
    list_files()

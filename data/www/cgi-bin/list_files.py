import os

UPLOAD_DIR = "data/upload"

def list_files():
    # os.makedirs(UPLOAD_DIR, exist_ok=True)
    files = sorted(os.listdir(UPLOAD_DIR))

    print("Content-Type: text/html\n")
    print("""<!doctype html>
<html lang="en">
<head>
    <meta charset="UTF-8"/>
    <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
    <title>At your Webservice \xe2\x80\x94 Uploaded Files</title>
    <link rel="stylesheet" href="../css/style.css"/>
    <style>
        .file-list { list-style: none; padding: 0; }
        .file-item { display: flex; align-items: center; gap: 1rem; padding: 0.5rem 0; border-bottom: 1px solid #ccc; }
        .delete-btn { cursor: pointer; color: red; background: none; border: none; font-size: 1.1rem; font-weight: bold; padding: 0 0.4rem; }
        .delete-btn:hover { opacity: 0.6; }
    </style>
</head>
<body>
    <header>
        <span class="header-title"><a href="/index.html">At your Webservice</a></span>
    </header>
    <main>
        <h1 class="page-title">UPLOADED FILES</h1>""")

    if not files:
        print("        <p>No files uploaded yet.</p>")
    else:
        print('        <ul class="file-list">')
        for f in files:
            safe = f.replace("'", "\\'")
            print('            <li class="file-item">')
            print('                <span>{}</span>'.format(f))
            print('                <button class="delete-btn" onclick="deleteFile(\'{}\')">&#x2715;</button>'.format(safe))
            print('            </li>')
        print('        </ul>')

    print("""        <p><a href="/files.html">Upload a file</a></p>
    </main>
    <script>
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

<?php

$upload_dir = "data/upload/";

// Ensure upload directory exists
if (!is_dir($upload_dir)) {
    mkdir($upload_dir, 0755, true);
}

// Check if a file was submitted
if (!isset($_FILES['myfile']) || $_FILES['myfile']['error'] !== UPLOAD_ERR_OK) {
    echo "Status: 200\n";
    echo "Content-Type: text/html\n\n";
    echo "<html><body><h2>Error: no file received.</h2>";
    echo "<a href='../cgi-bin/list_files.py'>Back</a></body></html>";
    exit;
}

$filename = basename($_FILES['myfile']['name']);
$dest     = $upload_dir . $filename;
$tmp      = $_FILES['myfile']['tmp_name'];

if (move_uploaded_file($tmp, $dest)) {
    // Redirect back to the file list (avoids "resubmit form?" on refresh)
    echo "Status: 302 Found\n";
    echo "Location: /cgi-bin/list_files.py\n\n";
} else {
    echo "Status: 200\n";
    echo "Content-Type: text/html\n\n";
    echo "<html><body><h2>Error: could not save file.</h2>";
    echo "<a href='../cgi-bin/list_files.py'>Back</a></body></html>";
}
?>

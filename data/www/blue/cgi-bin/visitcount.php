#!/usr/bin/env php
<?php
// CGI script - all output goes to stdout, including headers printed manually

// ── Config ────────────────────────────────────────────────────────────────────
define('CSV_FILE',    __DIR__ . '/../session_management/visitcount.csv');
define('EXPIRY_SECS', 24 * 60 * 60);   // 24 hours in seconds

// ── Helpers ───────────────────────────────────────────────────────────────────

/**
 * Read CSV into an associative array keyed by UUID.
 * Each row: [ 'uuid' => string, 'expiry' => int (unix ts), 'count' => int ]
 */
function read_csv(string $file): array {
    $rows = [];
    if (!file_exists($file)) {
        return $rows;
    }
    $fh = fopen($file, 'r');
    if (!$fh) return $rows;
    while (($line = fgetcsv($fh)) !== false) {
        if (count($line) < 3) continue;
        [$uuid, $expiry, $count] = $line;
        $rows[$uuid] = [
            'uuid'   => $uuid,
            'expiry' => (int) $expiry,
            'count'  => (int) $count,
        ];
    }
    fclose($fh);
    return $rows;
}

/**
 * Write associative array back to CSV (overwrites file).
 */
function write_csv(string $file, array $rows): void {
    $fh = fopen($file, 'w');
    if (!$fh) {
        echo "Status: 500 Internal Server Error\n\nERROR: Cannot write to CSV file.";
        exit(1);
    }
    foreach ($rows as $row) {
        fputcsv($fh, [$row['uuid'], $row['expiry'], $row['count']]);
    }
    fclose($fh);
}

/**
 * Get UUID from the COOKIE environment variable.
 * Parses the cookie string to find the 'uuid' key.
 */
function get_uuid_from_env(): ?string {
    $cookie = trim((string) getenv('COOKIE'));
    if ($cookie === '') return null;
    foreach (explode(';', $cookie) as $part) {
        $pair = explode('=', trim($part), 2);
        if (count($pair) === 2 && trim($pair[0]) === 'uuid') {
            $val = trim($pair[1]);
            return $val !== '' ? $val : null;
        }
    }
    return null;
}

/**
 * Generate a UUID v4.
 */
function generate_uuid(): string {
    $data    = random_bytes(16);
    $data[6] = chr((ord($data[6]) & 0x0f) | 0x40); // version 4
    $data[8] = chr((ord($data[8]) & 0x3f) | 0x80); // variant bits
    return vsprintf('%s%s-%s-%s-%s-%s%s%s', str_split(bin2hex($data), 4));
}

// ── Main logic ────────────────────────────────────────────────────────────────

// 1. Read existing data (creates file implicitly on first write)
$rows = read_csv(CSV_FILE);
$now  = time();

// 2. Purge rows whose expiry is older than 24 hours
foreach (array_keys($rows) as $uuid) {
    if (($now - $rows[$uuid]['expiry']) > EXPIRY_SECS) {
        unset($rows[$uuid]);
    }
}

// 3. Get UUID from COOKIE env var (always present, may be empty)
$uuid     = get_uuid_from_env();
$is_new   = ($uuid === null || !isset($rows[$uuid]));

$set_cookie_header = null;
if ($uuid === null) {
    // No cookie: generate a new UUID, prepare Set-Cookie header for output
    $uuid    = generate_uuid();
    $expires = gmdate('D, d M Y H:i:s T', $now + EXPIRY_SECS);
    $set_cookie_header = "Set-Cookie: uuid={$uuid}; Expires={$expires}; Path=/; HttpOnly; SameSite=Lax";
}

if ($is_new) {
    // First visit for this UUID: insert new row with count 1
    $rows[$uuid] = [
        'uuid'   => $uuid,
        'expiry' => $now,
        'count'  => 1,
    ];
} else {
    // Returning visitor: increment count and refresh expiry
    $rows[$uuid]['count']++;
    $rows[$uuid]['expiry'] = $now;
}

// 4. Persist updated data
write_csv(CSV_FILE, $rows);

// 5. Build response body and output everything to stdout
$body = (string) $rows[$uuid]['count'];

echo "Content-Type: text/plain\n";
echo "Status: 200\n";
echo "Content-Length: " . strlen($body) . "\n";
if ($set_cookie_header !== null) {
    echo $set_cookie_header . "\n";
}
echo "\n"; // blank line separates headers from body
echo $body;
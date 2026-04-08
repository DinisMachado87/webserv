<?php
$lengthRaw = $_SERVER['CONTENT_LENGTH'] ?? '0';
$bodySize = (int)$lengthRaw;

$body = '';
if ($bodySize > 0) {
    $body = file_get_contents('php://stdin', false, null, 0, $bodySize);
    if ($body === false) {
        $body = '';
    }
}

$query = $_SERVER['QUERY_STRING'] ?? null;

header('Content-Type: text/html; charset=UTF-8');

echo "<html>\n<body>\n";
echo "<h2>Hello world!</h2>\n";
echo "<p>Your php CGI is set up properly</p>\n";

if ($query !== null) {
    echo "<p>Here is your query string data: " .
         htmlspecialchars($query, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8') .
         "</p>\n";
}

echo "<p>Here is your body string data: " .
     htmlspecialchars($body, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8') .
     "</p>\n";

echo "</body>\n</html>\n";
?>

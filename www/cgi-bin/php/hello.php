#!/usr/bin/php
<?php
echo("Content-Type: text/plain");
echo("\r\n\r\n");

echo "PHP CGI \"Hello World!\"\n";
echo "REQUEST_METHOD=" . $_SERVER["REQUEST_METHOD"] . "\n";
echo "QUERY_STRING=" . $_SERVER["QUERY_STRING"] . "\n";

// Only read body if there's content (POST/PUT requests)
$body = "";
if (isset($_SERVER["CONTENT_LENGTH"]) && $_SERVER["CONTENT_LENGTH"] > 0) {
    $body = file_get_contents("php://stdin");
}
echo "BODY=" . $body . "\n";
?>
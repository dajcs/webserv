Before starting please follow the next few steps (files content can be anything and will be shown to you by the test):

- Download the cgi_test executable on the host
- Create a directory YoupiBanane with:
  - a file name youpi.bad_extension
  - a file name youpi.bla
  - a sub directory called nop
    - a file name youpi.bad_extension in nop
    - a file name other.pouic in nop
  - a sub directory called Yeah
    - a file name not_happy.bad_extension in Yeah

press enter to continue

Setup the configuration file as follow:
- / must answer to GET request ONLY
- /put_test/* must answer to PUT request and save files to a directory of your choice
- any file with .bla as extension must answer to POST request by calling the cgi_test executable
- /post_body must answer anything to POST request with a maxBody of 100
- /directory/ must answer to GET request and the root of it would be the repository YoupiBanane and if no file are requested, it should search for youpi.bad_extension files

press enter to continue

Before starting please verify that the server is launched
press enter to continue


Test GET http://localhost:8080/
content returned: <!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Welcome to Webserv</title>
</head>
<body>
    <div class="container">
        <h1>🚀 Welcome to Webserv!</h1>
        
        <div class="status">
            <strong>✅ Server is running!</strong>
            <p>Your HTTP/1.1 web server is successfully serving static files.</p>
        </div>
</body>
</html>

Test POST http://localhost:8080/ with a size of 0

FATAL ERROR ON LAST TEST: Post "http://localhost:8080/": EOF

import os
import sys
import html

length_raw = os.getenv("CONTENT_LENGTH", "0")
try:
    body_size = int(length_raw)
except ValueError:
    body_size = 0

body = sys.stdin.read(max(body_size, 0))
query = os.getenv("QUERY_STRING")

# CGI scripts should emit headers first
# print("Content-Type: text/html")
print()

print("<html>")
print("<body>")
print("<h2>Hello world!</h2>")
print("<p>Your python CGI is set up properly</p>")

if query is not None:
    print(f"<p>Here is your query string data: {html.escape(query)}</p>")

print(f"<p>Here is your body string data: {html.escape(body)}</p>")
print("</body>")
print("</html>")

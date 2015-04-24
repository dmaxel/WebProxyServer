# WebProxyServer
A simple web proxy server made in C for a networks class.

Danny Stieben, Aaron Tabor, Ahmed
Group 12
CSCE 3530 - UNT
April 23, 2015

To compile, do:
gcc -pthread -o server proxy_server.c

To run, do:
./server

Check the port number for connection with browser. Default is 60000.

To use, open browser and type:
<proxy_ip>:<port>/<address>

For example, on CSE05, you can use:
129.120.151.98:60000/www.wikipedia.org

To test bad word filter, you can try:
129.120.151.98:60000/www.dannystieben.com/lang.html

You can add bad words to the badwords.txt file. You can add more domains to block in blacklist.txt file. Make sure that each line has a newline character at the end.

For blocked sites, you can edit the response that the proxy server sends to the browser instead of the site.

If multiple requests fail midway through, it's possible that the proxy server crashed even though the last request was delivered. Simply start the server again to continue using it.
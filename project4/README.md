# Gunrock Web Server
This web server is about multi-threaded programming and operating systems. This version of the server can only handle one client at a time and simply serves static files. Also, it will close each connection after reading the request and responding, but generally is still HTTP 1.1 compliant.

This server was written by Sam King from UC Davis and is actively maintained by Sam as well. The `http_parse.c` file was written by [Ryan Dahl](https://github.com/ry) and is licensed under the BSD license by Ryan. This programming assignment is from the [OSTEP](http://ostep.org) textbook (tip of the hat to the authors for writing an amazing textbook).

# Overview

In this assignment, I developed a concurrent web server. I was provided with the code for a non-concurrent
(but working) web server. This basic web server operates with only a single
thread; it would be my job to make the web server multi-threaded so that it
can handle multiple requests at the same time.

In this project, I would be adding one key piece of functionality to the
basic web server: I would make the web server multi-threaded. 
I would also be modifying how the web server is invoked so
that it can handle new input parameters (e.g., the number of threads to
create).

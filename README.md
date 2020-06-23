# httpServer

Not finished and not really working.. the clientProgram was lifted from elsewhere and is not needed to use.

Run the server then connect by trying to reach the following page:
  http://localhost:8080/test.html

An example request is:
  GET /test.html HTTP/1.1
  Host: localhost:8080
  User-Agent: Mozilla/5.0 (X11; Fedora; Linux x86_64; rv:77.0) Gecko/20100101 Firefox/77.0
  Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8
  Accept-Language: en-GB,en;q=0.5
  Accept-Encoding: gzip, deflate
  Connection: keep-alive
  Upgrade-Insecure-Requests: 1
  
Worked once upon a time, however at the moment trying to manipulate the recieved client request causes SEG_FAULTS. I assume this is bc the input string is not in the correct format.

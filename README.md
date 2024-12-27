Start (windows):

gcc server.c dynamicparams.c clean.c sha1.c base64.c -o server -lws2_32 

If you change port, make sure you change the websocket port declared in /www/scripts/index.js 

Note: You're totally welcome to try and overflow the server, I'm sure there are tons of vulnerabilities beyond my understanding, and considering I did this in c...

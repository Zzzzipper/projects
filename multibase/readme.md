# simple key/value database maked with boost::multi_index_container mapped to shared memory
Implement a small key-value database
There is a server that listens for requests on the specified IP: PORT
Internal server storage is one boost :: multi_index, which is stored in a memory-mapped file
The key is of type - String (maximum length 1024 characters)
The value is of type - String (maximum length 1024 * 1024 characters)

Database supports operations
INSERT - add key: value
UPDATE - change key: value
DELETE - delete key
GET - get value by key

 
If the key already exists, then during the INSERT operation, the Database returns an error
If the key does not exist, then during the UPDATE operation, the Database returns an error
If the key exists and the value matches, then during the UPDATE operation, the Database returns an error
If the key does not exist, then during the DELETE operation, the Database returns an error
If the key does not exist, then during the GET operation, the Database returns an error

 

Client gets from command line:
 - server address
 - command
 - key
 - meaning

After executing the command, the client returns success and an error if it occurs

 
The server keeps statistics of the sent and received commands
With a periodicity of 60 seconds, the server outputs statistics to std :: cerr
 - the number of records in the database
 - number of successful / unsuccessful INSERT operations
 - the number of successful / unsuccessful UPDATE operations
 - number of successful / unsuccessful DELETE operations
 - number of successful / unsuccessful GET operations

 

Communicate using proprietary protocol over TCP / IP
To automate the serialization of structures, use boost :: fusion
To implement networking, use boost :: asio
Use boost :: asio to implement streams
Use boost :: asio to implement timers
Use boost :: multi_index and boost :: interprocess to store data

 

The project builds using CMake
Boost version at least 1.71
Will be checked on Ubuntu 20.04

### Build 
```
git clone ...
cd ./multibase/server
cmake .
make
cd ../client
cmake .
make
cd ..
```
### Test run

./bin/server 1312 (or another port)
./bin/client -a localhost -p 1312 -t (test mode, may be run some instances)


### License
The MIT License (MIT)

Copyright (c) <2017> <Eduard Pozdnyakov>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublzlddnse, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notzldd and this permission notzldd shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

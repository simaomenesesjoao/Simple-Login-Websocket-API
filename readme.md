# Simple Login + Websocket API

This program connects to the API and returns the list of distinct competition names. Summary:
1. Login to the requested API with an http request to get a token
2. Connect a websocket to the API Stream using this token
3. Reads messages from the websocket until a "sync" message is found and collects the competition names from the json output
4. Disconnects from websocket
5. Prints out the distinct competition names obtained, in alphabetical order

# Instructions on how to run
All the code required to run this program is in the included Dockerfile. To build and run the dockerfile, run
```
sudo docker build -t task .
sudo docker run task
```
The code can also be compiled directly with CMake after installing the dependencies 
- libcurl4-openssl-dev 
- libwebsocketpp-dev 
- libboost-all-dev
- libssl-dev
- nlohmann-json3-dev
- cmake
```
mkdir -p build
cd build
cmake ..
make
./task
```
Sample output:

``` 
-- Simple Login + Websocket API --
[Info] Login successful.
[Info] Connected to websocket.
[Info] Websocket closed.
[Info] Printing competition names:
AFC Asian Champions League
AFC Cup
ATP Challenger Bogota

     ...
Wexford
Women's Olympics Golf 2024
Yarmouth
``` 

# Code explanation
The structure of the code reflects the structure of the task. 
## `Authenticator` class
A class called `Authenticator` takes care of the login to the API and upon successful connection, returns an authentication token. This class uses the `libcurl` library to perform the HTTP requests and the `nlohmann/json` library to parse the json output. Even though `libcurl` is written in C, it has a pretty simple and clear interface which can easily be applied to this task. `nlohmann/json` is a modern, flexible and extensive C++ library that has become a standard for C++ json parsing.

## `Websocket` class
The `Websocket` class provides a wrapper around the `websocketpp` library that implements the websocket protocol. It is responsible for connecting to the MollyAPI websocket using SSL, reading and parsing the messages and closing the websocket once `sync` has been received. Each message is parsed to find the field `competition_name`, which then gets added to a set to ensure uniqueness of the name. The function that reads and parses the messages is called `get_messages_til_sync` and it returns the set `competition_names` containing all the unique competition names.

## Code flow
The main function sets up the credentials, calls `Authenticator` to get a token and passes it to `Websocket`, which then runs `get_messages_til_sync` to get the `competition_names` which are finally printed out to the console.


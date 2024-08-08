FROM ubuntu:24.10

RUN apt-get update && apt-get install -y g++ libcurl4-openssl-dev libwebsocketpp-dev libboost-all-dev libssl-dev nlohmann-json3-dev cmake

COPY src/main.cpp src/main.cpp
COPY CMakeLists.txt .
RUN mkdir -p build
WORKDIR build
RUN cmake ..
RUN make

CMD ["./task"]

FROM ubuntu:24.10

ENV HOME /root

SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get install -y g++ 
#RUN apt-get update && apt-get install -y g++ libcurl4-gnutls-dev libwebsocketpp-dev libboost-all-dev libssl-dev nlohmann-json3-dev

#COPY task_mollybet.cpp ./
COPY main.cpp ./
#RUN g++ -std=c++20 -Wall task_mollybet.cpp -lcurl -lssl -lcrypto -o task
RUN g++ -std=c++20 -Wall main.cpp -o main
CMD ["./main"]

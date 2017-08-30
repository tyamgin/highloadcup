#FROM centos:7
FROM ubuntu

# Install compiler

RUN apt-get update
RUN apt-get install -y g++
RUN apt install -y cmake
RUN apt-get install unzip
RUN apt-get install sudo

# Copy project
RUN mkdir /home/tyamgin
RUN mkdir /home/tyamgin/asdasdHACK
RUN mkdir /home/tyamgin/CLionProjects
RUN mkdir /home/tyamgin/CLionProjects/highload

WORKDIR /home/tyamgin/CLionProjects/highload

ADD . .

#remove previously executable file
RUN rm highload

EXPOSE 80

# http://derekmolloy.ie/hello-world-introductions-to-cmake/

CMD sudo bash ./run.sh

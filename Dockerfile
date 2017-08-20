FROM ubuntu

# Install compiler
RUN apt-get update
RUN apt-get install -y g++
RUN apt install -y cmake
RUN apt-get install unzip

# Copy project
RUN mkdir /home/tyamgin
RUN mkdir /home/tyamgin/CLionProjects
RUN mkdir /home/tyamgin/CLionProjects/highload

WORKDIR /home/tyamgin/CLionProjects/highload

ADD . .

#remove previously executable file
RUN rm highload

EXPOSE 80

# http://derekmolloy.ie/hello-world-introductions-to-cmake/

CMD     echo 'cmake...' &&\
	cmake -DCMAKE_BUILD_TYPE=Release . &&\
	echo 'make...' &&\
	make &&\
	echo 'Done.' &&\
	./highload

#FROM centos:7
FROM ubuntu

# Install apt-get for centos
#RUN wget http://packages.sw.be/rpmforge-release/rpmforge-release-0.5.2-2.el5.rf.i386.rpm
#RUN rpm -i rpmforge-release-0.5.*.rpm
#RUN yum install apt

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
	cmake -DCMAKE_BUILD_TYPE=Release . > /dev/null &&\
	echo 'make...' &&\
	make > /dev/null &&\
	echo 'Done.' &&\
	./highload

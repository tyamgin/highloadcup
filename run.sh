echo 'cmake...' &&\
cmake -DCMAKE_BUILD_TYPE=Release . > /dev/null &&\
echo 'make...' &&\
make > /dev/null &&\
echo 'running...' &&\
./highload

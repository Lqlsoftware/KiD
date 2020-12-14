rm -rf ./test

g++ -std=c++11 -o test -g -I.. test.cpp -L../lib -lengine -lpthread -lrt -lz -lpmem

rm -rf ./tmp

./test

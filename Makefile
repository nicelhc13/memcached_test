intensive_cas: intensive_putget.cpp
	g++ -o intensive_putget intensive_putget.cpp -lmemcached

test: test.cpp
	g++ -o test test.cpp -lmemcached

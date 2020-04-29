all:
	g++ -std=c++17 -o pssched pssched.cpp kernel.cpp -O3 -lrt
debug:
	g++ -std=c++17 -o pssched pssched.cpp kernel.cpp -O3 -lrt -D DEBUG
showunit:
	g++ -std=c++17 -o pssched pssched.cpp kernel.cpp -O3 -lrt -D SHOWUNIT
run:
	sudo ./pssched
gdb:
	g++ -std=c++17 -o pssched pssched.cpp kernel.cpp -g -lrt -D DEBUG

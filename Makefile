default: mycls.exe test.exe

mycls.exe: mycls.cpp processLSPrequest.cpp
	g++ -std=c++20 -g .\mycls.cpp .\processLSPrequest.cpp -o .\mycls.exe

test.exe: test.cpp
	g++ -std=c++20 .\test.cpp -o test.exe

clean:
	del *exe

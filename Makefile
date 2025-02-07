# Головна ціль
default: mycls docs

# Збірка головного файлу
mycls: src/mycls.o src/textProcessor.o src/processLSPrequest.o
	g++ -std=c++20 -g src/mycls.o src/textProcessor.o src/processLSPrequest.o -o mycls.exe

# Окремі об'єктні файли
src/mycls.o: src/mycls.cpp
	g++ -std=c++20 -c src/mycls.cpp -o src/mycls.o

src/textProcessor.o: src/textProcessor.cpp
	g++ -std=c++20 -c src/textProcessor.cpp -o src/textProcessor.o
	
src/processLSPrequest.o: src/processLSPrequest.cpp
	g++ -std=c++20 -c src/processLSPrequest.cpp -o src/processLSPrequest.o

# Генерація документації
.PHONY: docs
docs:
	doxygen Doxyfile

# Очищення з урахуванням Windows/Linux
clean:
	del /Q mycls.exe src\*.o 2>nul || rm -f mycls.exe src/*.o

CC = g++
EXE = scsa

all:
	$(CC) src/*.cpp -o $(EXE) -std=c++17 -Wall -Wextra -Werror

debug:
	$(CC) src/*.cpp -o $(EXE) -std=c++17 -Wall -Wextra -Werror -g

format:
	clang-format -i src/*.cpp src/*.hpp

format-check:
	clang-format --dry-run --Werror src/*.cpp src/*.hpp

clean:
	rm -f $(EXE)

.PHONY: all format format-check debug clean

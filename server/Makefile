all: build build/pacman_server

build:
	mkdir -p build

# Arquivos próprios do server

build/main.o: main.c
	gcc -Wall -Wextra -c main.c -o build/main.o

build/map.o: map.c
	gcc -Wall -Wextra -c map.c -o build/map.o

build/logger.o: logger.c
	gcc -Wall -Wextra -c logger.c -o build/logger.o

build/game.o: game.c
	gcc -Wall -Wextra -c game.c -o build/game.o

build/movements.o: movements.c
	gcc -Wall -Wextra -c movements.c -o build/movements.o

# Arquivos compartilhados (../shared)

build/socket.o: ../shared/socket.c
	gcc -Wall -Wextra -c ../shared/socket.c -o build/socket.o

build/protocol.o: ../shared/protocol.c
	gcc -Wall -Wextra -c ../shared/protocol.c -o build/protocol.o

build/crc.o: ../shared/crc.c
	gcc -Wall -Wextra -c ../shared/crc.c -o build/crc.o

build/screen.o: ../shared/screen.c
	gcc -Wall -Wextra -c ../shared/screen.c -o build/screen.o

# Linka os .o no executável

build/pacman_server: build/main.o build/map.o build/logger.o \
                     build/game.o build/movements.o build/socket.o \
                     build/protocol.o build/crc.o build/screen.o
	gcc build/main.o build/map.o build/logger.o build/game.o build/movements.o build/socket.o build/protocol.o build/crc.o build/screen.o -o build/pacman_server

# Limpeza

clean:
	rm -rf build

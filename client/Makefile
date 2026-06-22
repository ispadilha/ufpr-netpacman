all: build build/pacman_client

build:
	mkdir -p build

# Arquivos próprios do client

build/main.o: main.c
	gcc -Wall -Wextra -c main.c -o build/main.o

build/view.o: view.c
	gcc -Wall -Wextra -c view.c -o build/view.o

build/input.o: input.c
	gcc -Wall -Wextra -c input.c -o build/input.o

# Arquivos compartilhados (../shared)

build/socket.o: ../shared/socket.c
	gcc -Wall -Wextra -c ../shared/socket.c -o build/socket.o

build/protocol.o: ../shared/protocol.c
	gcc -Wall -Wextra -c ../shared/protocol.c -o build/protocol.o

build/crc.o: ../shared/crc.c
	gcc -Wall -Wextra -c ../shared/crc.c -o build/crc.o

build/screen.o: ../shared/screen.c
	gcc -Wall -Wextra -c ../shared/screen.c -o build/screen.o

build/transfer.o: ../shared/transfer.c
	gcc -Wall -Wextra -c ../shared/transfer.c -o build/transfer.o

# Linka os .o no executável

build/pacman_client: build/main.o build/view.o build/input.o \
                     build/socket.o build/protocol.o build/crc.o \
                     build/screen.o build/transfer.o
	gcc build/main.o build/view.o build/input.o build/socket.o build/protocol.o build/crc.o build/screen.o build/transfer.o -o build/pacman_client

# Limpeza

clean:
	rm -rf build

game: game.o message.o
	g++ game.o message.o -o game

game.o: ./source/game.cpp
	g++ -c game.cpp -o game.o

message.o: message.cpp
	g++ -c message.cpp -o message.o

clean:
	rm -rf *.o game

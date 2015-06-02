TARGET = game

CC = g++


OBJS = game.o message.o

game:${OBJS}
	${CC} -o $@ ${OBJS}
	@echo ">>> Linking Complete!"

clean:
	-rm -f *.o core *.core
	@echo ">>> Cleanup complete!"

.cpp.o:
	${CC} -c $<
	@echo ">>> Compiled" $< "successfully!"

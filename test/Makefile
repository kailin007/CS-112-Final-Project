INC=-I/home/kali/CS-112-Final-Project/test/include
LIB=-L/home/kali/CS-112-Final-Project/test/lib

all: client.c test_fun.c run_test.c
	gcc $^ -lpthread -o out $(INC) $(LIB) -lcunit -lcurses -static
clean:
	rm -rf out
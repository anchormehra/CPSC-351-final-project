.default: all

all: multi_threaded_consumer producer
 
multi_threaded_consumer: multi_threaded_consumer.cpp
	g++ multi_threaded_consumer.cpp -o multi_threaded_consumer -lpthread

producer: producer.cpp
	g++ producer.cpp -o producer

val1:
	valgrind --leak-check=full --tool=memcheck ./multi_threaded_consumer 3@csu.fullerton.edu 2

val2:
	valgrind --leak-check=full --tool=memcheck ./producer Zoomreport.txt

clean:
	$(RM) *~ *.o *.gch *.out multi_threaded_consumer producer

test: tests/test_config.cpp ip_utils.cpp ip_utils.h
	g++ tests/test_config.cpp -o test_runner
	./test_runner

clean:
	rm -f test_runner

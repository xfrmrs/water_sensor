.PHONY: test clean

test: tests/extracted_sut.cpp
	g++ -std=c++11 -I./tests/mocks tests/test_validateConfig.cpp -o test_config
	./test_config
	rm test_config

tests/extracted_sut.cpp: config.ino
	# Extract parseIpAddressString, isSafePinValue, arePinsUnique, isSupportedBaud, validateConfig
	awk '/bool parseIpAddressString\(const String &value, IPAddress &parsedValue\)/ { flag=1 } /^bool loadConfigFromFs\(\)/ { flag=0 } flag { print }' config.ino > tests/extracted_sut.cpp

clean:
	rm -f test_config tests/extracted_sut.cpp
# Mock Makefile to compile test

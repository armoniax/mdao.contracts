clear:
	rm -rf ./build/*

compile:
	eosio-cpp -abigen ./src/stake.cpp -o ./build/hello.wasm -I ./include/

do:
	echo "compile"
	make clear
	make compile

clear-test:
	rm -rf ./build/*

compile-test:
	eosio-cpp -abigen ./hello.cpp -o ./build/hello.wasm -I ./include/

do-test:
	echo "compile"
	make clear-test
	make compile-test
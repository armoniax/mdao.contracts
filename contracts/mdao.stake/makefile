clear:
	rm -rf ./build/*

compile:
	eosio-cpp -abigen ./src/stake.cpp -o ./build/mdaostake.wasm -I ./include/

do:
	echo "compile"
	make clear
	make compile

clear-test:
	rm -rf ./build/*

compile-test:
	eosio-cpp -abigen ./test/test.cpp -o ./build/multi_index_large.wasm -I ./include/

do-test:
	echo "compile"
	make clear-test
	make compile-test
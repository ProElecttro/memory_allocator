interactive platform : https://proelecttro.github.io/memory_allocator/

run allocator : 
```
clang++ -std=c++17 \
  -I"$(brew --prefix googletest)/include" \
  -L"$(brew --prefix googletest)/lib" \
  contiguous_memory_allocator_google_test.cpp \
  -lgtest -lgtest_main -pthread \
  -o allocator_tests
```

then 
```
./allocator_tests
```

to run randomized tests : 
```
./allocator_tests --gtest_filter='*Randomized*'
```

To list every test without executing it:
```
./allocator_tests --gtest_list_tests
```

To run with memory-safety instrumentation:
```
clang++ -std=c++17 -g -O1 \
  -fsanitize=address,undefined \
  -fno-omit-frame-pointer \
  -I"$(brew --prefix googletest)/include" \
  -L"$(brew --prefix googletest)/lib" \
  contiguous_memory_allocator_google_test.cpp \
  -lgtest -lgtest_main -pthread \
  -o allocator_tests_asan

./allocator_tests_asan
```

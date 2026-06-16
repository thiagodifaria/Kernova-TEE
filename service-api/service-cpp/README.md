# Kernova C++ Service

Native Kernova-TEE backend implemented in C++20 and x86-64 NASM.

```bash
cmake -S service-api/service-cpp -B service-api/service-cpp/build
cmake --build service-api/service-cpp/build --parallel
ctest --test-dir service-api/service-cpp/build --output-on-failure
```

All native source, headers, tests and linker configuration are owned by this
service.

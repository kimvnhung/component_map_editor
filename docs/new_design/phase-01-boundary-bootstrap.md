# Phase A: Boundary và test harness

## Mục tiêu

Tạo runtime library/process độc lập và test harness để chứng minh runtime không phụ thuộc UI.

## Phạm vi

- target `component_map_runtime`;
- executable `component_map_runtime_process`;
- test fixture chạy trong process;
- runtime descriptor và lifecycle tối thiểu;
- fake handler registry;
- graph fixture Producer -> Processor -> Sink.

## Công việc

1. Bảo đảm runtime target không link QML hoặc editor target.
2. Giữ `RuntimeService` lifecycle deterministic.
3. Định nghĩa runtime event, component identity và failure result tối thiểu.
4. Tạo fake Producer, Processor và Sink handler.
5. Tạo fixture có hai instance cùng type nhưng state riêng.
6. Tạo smoke test chạy runtime core không cần UI.

## Đầu ra

- runtime library build độc lập;
- process skeleton;
- fake graph fixture;
- test harness cho các phase sau.

## Exit criteria

- Runtime library và test build thành công.
- Fake graph được khởi tạo từ deployment fixture.
- Không có API runtime nhận `GraphModel*`, `QObject*` hoặc QML type.
- Test chứng minh hai instance cùng type không chia sẻ state.
- Có hướng dẫn chạy runtime test.

## Quyết định phạm vi

Phase E dùng gRPC TCP plaintext trên `127.0.0.1`. Phase A chỉ giữ boundary transport-neutral.

## Cách build và chạy smoke test

Sau khi cài dependency bằng Conan và cấu hình Qt6, bật hai option của Phase 1:

```bash
conan install . -of build/phase1_runtime/deps --build=missing -s build_type=Debug
cmake -S . -B build/phase1_runtime \
  -DCMAKE_PREFIX_PATH="<conan-prefix>;<qt-prefix>" \
  -DCME_BUILD_TESTS=ON \
  -DCME_BUILD_RUNTIME=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/phase1_runtime \
  --target tst_RuntimeServicePhase1 component_map_runtime_process
ctest --test-dir build/phase1_runtime \
  -R '^tst_RuntimeServicePhase1$' --output-on-failure
build/phase1_runtime/component_map_runtime/component_map_runtime_process
```

Test `tst_RuntimeServicePhase1` chạy độc lập với editor và QML. Fixture trong test đăng ký fake Producer, Processor và Sink, sau đó kiểm tra route tuần tự và state riêng của từng instance.

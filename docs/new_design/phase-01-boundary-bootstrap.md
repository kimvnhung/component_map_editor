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

# Phase C: Runtime core và vertical slice

## Mục tiêu

Chạy được graph Producer -> Processor -> Sink trong test process với actor, mailbox, routing, state và failure logging.

## Thành phần MVP

- `RuntimeEngine`;
- `EventRouter` cho direct route;
- `ActorRegistry`;
- `ActorScheduler`;
- bounded FIFO `Mailbox`;
- handler factory/registry;
- opaque `ComponentStateStore` in-memory;
- structured failure logger;
- idempotency store in-memory.

## Actor semantics

- Mỗi component instance có mailbox và state riêng.
- Mặc định `max_concurrency = 1`.
- Actor khác nhau có thể chạy song song.
- Public status: `idle`, `busy`, `failed`, `stopped`.
- Event chỉ complete sau khi handler result được quyết định.
- Handler không tự tạo thread và không tự truy cập mailbox.

## Routing MVP

- direct route;
- nhiều direct routes cho fan-out cơ bản;
- enabled/disabled route;
- target port và message type;
- delivery failure reporting.

Không làm filter, ordering key, delivery mode hoặc advanced backpressure.

## Backpressure

Mailbox bounded. Khi đầy, runtime reject event và ghi rejection record gồm:

- event id;
- idempotency key;
- target component;
- route;
- timestamp;
- reason code;
- mailbox depth.

Runtime không block producer, không spill-to-disk và không automatic retry.

## Failure contract

Mọi failure phải:

- trả machine-readable failure code;
- ghi structured log;
- ghi event history hoặc rejection record;
- không bị silently dropped.

Các failure tối thiểu:

- validation failure;
- handler failure;
- delivery failure;
- mailbox-full rejection;
- idempotency conflict.

Runtime không có retry loop, backoff scheduler hoặc dead-letter queue. Client tự quyết định resubmit.

## Idempotency

- Client tạo idempotency key.
- Cùng key và cùng request fingerprint trả lại kết quả đã biết.
- Cùng key nhưng fingerprint khác trả conflict.
- Key đang xử lý không được dispatch đồng thời lần hai.
- Runtime không tự chạy lại event sau failure.

## Công việc

1. Tách actor/mailbox primitive khỏi execution engine cũ.
2. Định nghĩa runtime event và handler result.
3. Viết fake Producer, Processor và Sink.
4. Resolve route table từ deployment document.
5. Implement scheduler shutdown.
6. Implement in-memory state, idempotency và rejection store.
7. Implement structured failure logging.
8. Thêm status counters cơ bản.

## Test matrix

- Producer -> Processor -> Sink;
- nhiều instance cùng type có state riêng;
- actor FIFO và `max_concurrency = 1`;
- actor khác nhau chạy song song;
- mailbox đầy tạo rejection;
- handler failure có log và failure code;
- delivery failure không automatic retry;
- idempotency duplicate và conflict;
- graceful stop không nhận event mới.

## Exit criteria

- Vertical slice chạy hoàn chỉnh trong test process.
- Không cần process hoặc QML để chạy runtime core test.
- Không có data race trên state của cùng actor.
- Failure và rejection không bị silently dropped.
- Không có retry scheduler hoặc dead-letter dependency.
- Compatibility engine cũ vẫn build và test pass.

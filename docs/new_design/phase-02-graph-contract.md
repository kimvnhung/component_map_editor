# Phase B: Deployment graph contract tối thiểu

## Mục tiêu

Tạo deployment document nhỏ nhất để runtime dựng graph Producer -> Processor -> Sink mà không cần đọc editor QObject.

## Phạm vi

- graph version immutable;
- một active graph version;
- component type và instance id;
- component properties;
- input/output port;
- direct route;
- message type;
- state schema version;
- validation cơ bản.

## Mô hình tối thiểu

### Component instance

- `instance_id` duy nhất trong graph;
- `component_type`;
- configured properties;
- initial state schema version.

### Route

- `connection_id`;
- source instance và output port;
- target instance và input port;
- message type;
- enabled.

Chỉ hỗ trợ direct route trong MVP. Fan-out có thể dùng nhiều direct routes nhưng không cần filter hoặc delivery mode nâng cao.

## Validation bắt buộc

- duplicate instance hoặc connection id;
- port không tồn tại;
- sai direction;
- message type không tương thích;
- route trỏ tới component không tồn tại;
- graph version thiếu hoặc trùng;
- component type không có handler.

Validation result có code ổn định, severity và field path.

## Activation policy

- Runtime phải stopped trước khi activate graph version mới.
- Chỉ một graph version active tại một runtime instance.
- Không migrate state giữa graph versions.
- Activate graph mới reset component state, pending inbox và pending work.
- Event pending của graph cũ không chuyển sang graph mới.

Event history và runtime logs giữ lại theo retention một ngày, trừ khi người dùng yêu cầu reset database toàn bộ.

## Proto

Tạo `proto/runtime/deployment.proto` và dùng package `cme.runtime.v1`. Không thay đổi compatibility proto hiện có.

## Exit criteria

- Graph fixture serialize và deserialize được.
- Runtime nhận deployment document mà không link editor model.
- Clone instance giữ type chung nhưng state/config riêng.
- Route hợp lệ resolve được source port, target port và message type.
- Graph invalid trả validation code ổn định.
- Activate graph mới bị từ chối nếu runtime chưa stopped.

# Speckit guideline for developer (not for ai)
## Hướng dẫn sử dụng skills
    - /speckit-constitution + Mô tả: Tạo / cập nhật rule tổng quan cho dự án. Thiết kế spec chính cho phần mềm
    - /speckit-specify + Mô tả tính năng nhỏ cần thêm/ cập nhật : tạo spec branch để phân tích vấn đề
    - /speckit.plan : Tạo plan từ spec
    - /speckit-tasks : để tạo danh sách task triển khai từ plan và các contract đã hoàn tất.
    - /speckit.analyze : để kiểm tra tính nhất quán giữa spec.md, plan.md và tasks.md
        - /speckit-clarify: từ kết quả analyze, cần thông tin thêm để làm rõ công việc, conflict thông tin, ...
        - /speckit-plan: chạy sau clarify để cập nhật plan
        - /speckit-tasks:
        - /speckit-analyze: 
        - ... Lặp lại cho đến khi hết vấn đề cần clarify (xác định từ người dùng)
    - /speckit.implement: Thực hiện các tasks
    
    /speckit.checklist - Generate quality checklists
    /speckit.converge - Assess codebase against artifacts and append remaining tasks
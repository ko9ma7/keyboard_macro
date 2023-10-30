import hid

# 장치 검색
devices = hid.enumerate(0x1d6b, 0x0104)  # Linux Foundation의 VID와 Multifunction Composite Gadget의 PID
# devices = hid.enumerate(0x0483, 0x522C)  # Linux Foundation의 VID와 Multifunction Composite Gadget의 PID

for device in devices:
    path = device['path']
    print(path)
    h = hid.device()
    h.open_path(device['path'])

    # HID report 데이터 작성 (예제 데이터)
    report_data = [0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]

    # data = [0x00] * 8  # 처음 8바이트는 0x00으로 채웁니다. (예시)
    # data.extend([0x10, 0x20, 0x30, 0x40, 0x50, 0x60])  # 추가 6바이트

    # HID report 전송
    # print(h.write(data))
    print(h.write(report_data))

    h.close()
        

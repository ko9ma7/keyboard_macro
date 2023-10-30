import usb.core
import usb.util

# USB 장치의 Vendor ID와 Product ID를 기반으로 디바이스를 찾습니다.
dev = usb.core.find(idVendor=0x1d6b, idProduct=0x0104)  # 여기서는 예시 ID를 사용했습니다.

# 만약 디바이스를 찾을 수 없으면, 에러 메시지를 출력합니다.
if dev is None:
    raise ValueError('Device not found')

# 디바이스의 첫 번째 configuration을 선택합니다.
dev.set_configuration()

# HID report 데이터 구성
# 64비트 (8바이트) 입력, 6바이트 추가 입력으로 총 14바이트의 데이터를 보내야 합니다.
data = [0x00] * 8  # 처음 8바이트는 0x00으로 채웁니다. (예시)
data.extend([0x10, 0x20, 0x30, 0x40, 0x50, 0x60])  # 추가 6바이트

# 데이터 전송
# 첫 번째 인자는 endpoint, 두 번째 인자는 데이터, 세 번째 인자는 timeout (밀리초) 입니다.
# 여기서는 예시로 endpoint 1을 사용하였습니다. 실제 endpoint 번호는 환경에 따라 다를 수 있습니다.
a = dev.write(1, data, 100000000000000)
# print(a)

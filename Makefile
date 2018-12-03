.PHONY: compile st serial espas ota terminals terminal mqtt test

compile:
	pio run

# eg make st port=1
st: compile serial terminal

serial:
	@./handle_serial.py --port=$$port
	@#esptool.py --port `python -m serial.tools.list_ports 2>/dev/null|grep "usb"` erase_flash
	@#platformio run --target upload

flashinfo:
	@esptool.py --port `python -m serial.tools.list_ports 2>/dev/null|grep "usb"` flash_id

# available esp nodes. Their name must beginn with "esp-xy"
# @ suppresses output of command run
# nmap solution: https://stackoverflow.com/questions/503171/send-a-ping-to-each-ip-on-a-subnet
esps:
	@nmap -sn 192.168.1.*|grep -i esp_|perl -pe 's/.* (esp_[^ ]*) \(([^)]*)\)/http:\/\/\1.local: \2/i'
	@#@nmap -n -sP 192.168.1.0/24
	@#@arp -a|grep esp|sed 's/\([^ ]*\) (\(.*\)).*/\1: \2/'

# call make ota ip=192.168.1.6 or w/o argument
ota:
	@./ota.sh $$ip

terminals:
	@./handle_serial.py --list --port=$$port
	@#platformio device list

terminal:
	@./handle_serial.py --monitor --port=$$port
	@#platformio device monitor

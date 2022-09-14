import serial

def str2list(s):
		return [int(x, 16) for x in s.strip().split(' ')]

kayo = serial.Serial('/dev/tty.usbserial-1420', 115200)
print(kayo.name)

resetAndHome   = str2list('EE BB 05 00 00 02 00 55')
toTestPosition = str2list('AA 4E 84 4E 84 00 11 55') # to 20100, 20100
toZeroPosition = str2list('AA 00 00 00 00 00 11 55') # to 0, 0
openFeeder1    = str2list('BB 00 01 00 00 00 00 55') # byte 3 = feeder
openFeeder10   = str2list('BB 00 0A 00 00 00 00 55') # byte 3 = feeder
closeFeeders   = str2list('BB 00 00 00 00 00 00 55') # No feeder specified

ConveyorRun    = str2list('EE CC 01 00 00 00 00 55') #conveyor runs
ConveyorStop   = str2list('EE CC 00 07 00 00 00 55') #pin drops and conveyor stops


menu = {}
menu[0]="Exit"
menu[1]="Reset and Home"
menu[2]="Go to test position"
menu[3]="Go to zero position"
menu[4]="Open feeder 1"
menu[5]="Open feeder 10"
menu[6]="Close feeders"
menu[7]="Conveyor Run"
menu[8]="Conveyor Stop"


while True:
	options=menu.keys()
	for entry in options:
		print (entry, menu[entry])

	selection = input("Please select:")
	if selection == 0:
		print("Exit")
		exit()
	if selection == 1:
		print("Reset and Home")
		b = bytearray(resetAndHome)
		kayo.write(b)
	elif selection == 2:
		print("Go to test position")
		b = bytearray(toTestPosition)
		kayo.write(b)
	elif selection == 3:
		print("Go to zero position")
		b = bytearray(toZeroPosition)
		kayo.write(b)
	elif selection == 4:
		print("Open feeder 1")
		b = bytearray(openFeeder1)
		kayo.write(b)
	elif selection == 5:
		print("Open feeder 10")
		b = bytearray(openFeeder10)
		kayo.write(b)
	elif selection == 6:
		print("Close feeders")
		b = bytearray(closeFeeders)
		kayo.write(b)
	elif selection == 7:
		print("Conveyor Run")
		b = bytearray(ConveyorRun)
		kayo.write(b)
	elif selection == 8:
		print("Conveyor Stop")
		b = bytearray(ConveyorStop)
		kayo.write(b)
#		break
	else:
		print("Unknown command")

#b = bytearray(resetAndHome)
#b = bytearray(toTestPosition)
#kayo.write(b)

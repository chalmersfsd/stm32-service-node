import serial as sr
import timeit
from time import sleep
ser = sr.Serial('COM3', 9600, timeout=1, parity=sr.PARITY_EVEN, rtscts=1)

sleep(1)
on = True
i = 0
reads = 0
writes = 0

def setLightMessage(light, intensity):
	return 'set|assi_' + light + '|' + str(intensity)
	
def getStatusMessage():
	return 'get|status'
	
def sendMessage(msg):
	global reads, writes
	msgOut = str(len(msg)) + ':' + msg + ';'
	# print('sending', msgOut)
	ser.write(str.encode(msgOut))
	ser.flush()
	writes+=1
	msgIn = str(ser.read_until(str.encode(';')))
	# print('response is ', msgIn, ' for ', msg)
	if "ACK" in msgIn and msg in msgIn:
		reads+=1
		return True, msgIn
	else:
		return False, msgIn

def testLight():
	global i
	global on
	if ser.writable() and ser.out_waiting == 0:
		i+=10
		if on:
			on = False
			if not sendMessage(setLightMessage('blue', i)):
				print("Error")
		else:
			on = True
			if not sendMessage(setLightMessage('blue', 0)):
				print("Error")
	else:
		print("Not writable, nothing waiting")
		
def testStatus():
	if ser.writable() and ser.out_waiting == 0:
		result, reading = sendMessage(getStatusMessage())
		print('status request ', result, '---', reading)
	else:
		print("Not writable, nothing waiting")
		
start = timeit.default_timer ()
while(True):
	testStatus()
	testLight()
end = timeit.default_timer ()
elapsed = end-start
print(reads, 'reads and ', writes, 'writes in ',elapsed, 'seconds', round(reads/elapsed), 'Hz communication')

ser.close()
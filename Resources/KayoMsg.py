import math
class CKayoMsg(object):
	KSF = 100.0  # XY scale factor



	
	def __init__(self):
		
		self.FeederStateBitwise_0_63 = 0
		self.FeederStateBitwise_64_127 = 0
		
		self.resetAndHome = self.str2list('EE BB 05 00 00 02 00 55')
		self.resetButton = self.str2list('EE BB 04 00 00 02 00 55')
		self.homeZ = self.str2list('EE BB 02 00 00 00 00 55')
		self.blankEEBB = self.str2list('EE BB 00 00 00 00 00 55')
		self.trackreset = self.str2list('EE BB 06 00 00 02 00 55')
		self.clipPCB = self.str2list('EE CC 00 01 00 00 00 55')
		self.releasePCB = self.str2list('EE CC 00 00 00 00 00 55')
		self.moveXYabsBlank = self.str2list('AA 00 00 00 00 00 00 55')
		
		self.moveZabsBlank = self.str2list('CC 00 00 00 00 00 00 55')
		self.CannedPnPTemplate = self.str2list('CC 00 00 00 00 00 00 55')
		
		
		self.TrackReset = self.str2list('EE BB 06 00 00 02 00 55')
		self.TrackSetWidth = self.str2list('EE DD 00 00 00 00 00 55')
		self.CheckPCBOnConveyorStatus = self.str2list('FF 00 00 00 00 00 00 55')
		self.LightsTemplate = self.str2list('BB 00 00 00 00 00 00 55')
		self.redlightON = self.str2list('BB 09 00 00 00 00 00 55')
		self.ABSMOVETYPES = {'NORMAL': 0, 'T1': 1 , 'T2' : 2,  'T3': 3,'THROW': 4}

		self.ConveyorClear = self.str2list('EE CC 00 00 00 00 00 55')

		self.ConveyorStep1 = self.str2list('EE CC 00 02 00 00 00 55')	#activates some4 solenoid
		self.ConveyorStep2 = self.str2list('EE CC 00 03 00 00 00 55')	#deactivates some solenoid
		self.ConveyorStopPinEngage = self.str2list('EE CC 00 06 00 00 00 55')	#STOP PIN ENGAGE !
		self.ConveyorRun = self.str2list('EE CC 01 00 00 00 00 55')		#conveyor runs
		self.ConveyorStop  = self.str2list('EE CC 00 07 00 00 00 55')	#pin drops and conveyor stops
		self.ConveyorClipPCB = self.str2list('EE CC 00 01 00 00 00 55')
		self.ConveyorUnclipAndMove  = self.str2list('EE CC 02 00 00 00 00 55')	#canned- waits till pcb on output and stops
		self.Oddball = self.str2list('AA 17 70 A4 B4 00 06 55')
		self.Oddball = self.str2list('AA 17 70 A4 B4 00 06 55')
		self.NozzRotation = self.str2list('DD BB 00 00 00 00 00 55')	#nozzle rotation template
		self.RotationMulti = self.str2list('DD B2 00 00 00 00 00 55')	#RotationMulti
		self.TubeFeederTemplate = self.str2list('BB 00 00 00 00 00 00 55')
		self.XYjogTemplate = self.str2list('EE EE 00 00 00 00 00 55')
		self.ControlFeeder = self.str2list('BB 00 00 00 00 00 00 55')
		self.OpenSuppliment = self.str2list('EE CC 00 08 00 00 00 55')
		self.CloseSuppliment = self.str2list('EE CC 00 09 00 00 00 55')
		self.CloseVacOpenBlow = self.str2list('DD CC 02 00 00 00 00 55')
		self.CloseVacBlowAll = self.str2list('DD CC 03 00 00 00 00 55')
		self.OpenBlowOnZn = self.str2list('DD CC 04 00 00 00 00 55')
		self.CloseBlowOnZn = self.str2list('DD CC 05 00 00 00 00 55')
		self.OpenVacOnZn = self.str2list('DD CC 06 00 00 00 00 55')
		self.CloseVacOnZn = self.str2list('DD CC 07 00 00 00 00 55')
	
	def str2list(self, s):
		return [int(x, 16) for x in s.strip().split(' ')]
	
	def makeSuppliment(self,openIt):
		if openIt :
			return self.OpenSuppliment
		else :
			return self.CloseSuppliment
		
	def makeOpenVac(self, Zn : int):

		b = self.OpenVacOnZn.copy()
		assert (Zn != 0)
		b[6] = Zn
		return b
	
	def makeCloseVac(self, Zn : int):
		b = self.CloseVacOnZn.copy()

		assert(Zn !=0)
		b[6] = Zn
		return b
	
	def makeOpenBlow(self, Zn : int):

		b = self.OpenBlowOnZn.copy()
		assert (Zn != 0)
		b[6] = Zn
		return b

	def makeCloseBlow(self, Zn : int):

		b = self.CloseBlowOnZn.copy()
		assert (Zn != 0)
		b[6] = Zn
		return b
	
	def makeCloseVacOpenBlow(self, Zn : int):

		b = self.CloseVacOpenBlow.copy()
		assert (Zn != 0)
		b[6] = Zn
		return b
	
	def makeFeederControlMsg(self,feederNumber : int , action : bool):	# True == open
		b = self.ControlFeeder.copy()
		if action == True :
			if (feederNumber <=63)  :
				self.FeederStateBitwise_0_63 |= (1<<feederNumber)
			elif (feederNumber >=64)  :
				self.FeederStateBitwise_64_127 |= (1<<(feederNumber-64))
			b[2] = feederNumber
			
		else :
			if (feederNumber <=63)  :
				self.FeederStateBitwise_0_63 &= (~(1<<feederNumber))
			elif (feederNumber >=64)  :
				self.FeederStateBitwise_64_127 &= (~(1<<(feederNumber-64)))
			
		
		return b
	
	def makeXYabsMsg(self, xy: tuple , speed: int, movetype='NORMAL'):
		b = self.moveXYabsBlank.copy()
		#transpose request to machine coordinates as A4 is transposed -X, -Y
		xy = (g.md.XYrealhome.X - xy[0], g.md.XYrealhome.Y - xy[1])
		
		xb = int((xy[0] * self.KSF)+0.5).to_bytes(length=2, byteorder='big')
		yb = int((xy[1] * self.KSF)+0.5).to_bytes(length=2, byteorder='big')
		b[1:3] = xb
		b[3:5] = yb
		b[6] = (self.ABSMOVETYPES[movetype.upper()] << 4) | speed
		return b
	
	def makeXYjogMsg(self, isX : int, isY : int, speed : int):	# isX -1 for going -, +1 for +
		b = self.XYjogTemplate.copy()

		#transpose request to machine coordinates as A4 is transposed -X, -Y
		isX = -isX	#transpose to machine direction
		isY = -isY	#treanspose to machine direction


		if speed != 0 :
			b[2] = 0x1		#motion yes  or no
			b[6] = speed  # use speed as a stop, start
			
		if isX > 0 :
			b[3] = 0x1
		elif isX < 0 :
			b[3] = 0x2
			
		if isY > 0 :
			b[4] = 0x1
		elif isY < 0 :
			b[4] = 0x2
		return b
	
	def ZaxisConvertZdepth(self, toolname: str, Z: float):
		# parameter Z is the height above the deck of the nozzle holder
		# for KayoA4, we need to convert that to a top is zero, fully lowered is some position number down...
		
		zaxisKayo = g.md.headZNameDict[toolname][0]
		zaxisIndex = g.md.headZNameDict[toolname][1]
		Zzero = g.md.headZMaxList[zaxisIndex]
		Zmin = g.md.headZMinList[zaxisIndex]
		if Z > Zzero:
			logger.fatal(' attempt to go Z {:3f} > Zmax {:3f} ! '.format(Z,Zzero))
			Z = Zzero
		elif Z < Zmin:
			logger.fatal(' attempt to go Z {:3f}< Zmin {:3f}! '.format(Z,Zmin))
			Z = Zmin
		Znew = Zzero - Z  # subtract desired height from homed height
		return (Znew, zaxisKayo)
	
	def makeZdepthconversion(self, Zconv : float) :
		# cam is a sine rule...
		MOTORSTEPS = 1.8
		MICROSTEPS = g.md.ZAXISMICROSTEPS
		
		depthDeg90 = 10.54
		depthDeg135 = 18.6
		# CAM WHEEL IS 16MM DIAMETER, MOTOR SHAFT center TO CAM WHEEL CENTRE IS 12MM
		if Zconv <= depthDeg90:
			angle = math.asin(Zconv / depthDeg90)
		else:
			Zconv -= depthDeg90
			angle = math.pi * 0.5
			angle += math.asin(Zconv / ((depthDeg135 - depthDeg90) * 1.414))
		
		fullSteps = math.degrees(angle) / MOTORSTEPS
		microSteps = int((fullSteps * MICROSTEPS) + 0.5)
		
		MaxZAxisMicrostepsValue = g.md.MAXZAxisMicrostepsValue
		if microSteps > MaxZAxisMicrostepsValue:
			microSteps = MaxZAxisMicrostepsValue
			logger.info('microSteps > MaxZAxisMicrostepsValue !')
			
		elif microSteps < 0 :
			microSteps = 0
			logger.info('microSteps < 0 !')
		return microSteps
	
	def makeZabsMsg(self, toolname: str, Z: float, speed: int):
		b = self.moveZabsBlank.copy()
		Zconv, zaxisKayo = self.ZaxisConvertZdepth(toolname, Z)
		
		microSteps = self.makeZdepthconversion(Zconv)
		
		zb = microSteps.to_bytes(length=2, byteorder='big')
		b[1:3] = zb
		b[6] = (int(zaxisKayo) << 4) | speed
		return b
	
	def makeZtestMsg(self,toolname : str, val:int):
		#1600  value is exactly 90 degrees on the Z axis cam,so must be 32 microstep setting.
		#2490 appears to be maximum - 140.0625 degrees
		#cam length is
		b = self.moveZabsBlank.copy()
		_, zaxisKayo = self.ZaxisConvertZdepth(toolname, 0)
		zb =val.to_bytes(length=2, byteorder='big')
		b[1:3] = zb
		b[6] = (int(zaxisKayo) << 4) | 0x1
		return b
	
	def makeTrackWidthMsg(self, width):
		b = self.TrackSetWidth.copy()
		W = (width - g.md.KayoA4TrackMinimumWidth) * g.md.TRACKScaleFactor
		zb = int(W+0.5).to_bytes(length=2, byteorder='big')
		b[2:4] = zb
		return b
	
	def makeLightControl(self, lightnumber, brightness):
		b = self.LightsTemplate.copy()
		b[3] = brightness
		b[6] = lightnumber
		return b

	def makeErrorLightControl(self, lightname : str ):
		
		b = self.redlightON.copy()
		lightname = lightname.upper()
		if lightname == 'RED' : b[1] = 9
		elif lightname == 'YELLOW' : b[1] = 8
		elif lightname == 'OFF' : b[1] = 7
		return b
	
	def makedCannedPnP(self, depth : float , feedernumber : int , speed:  int , delay : int ,nozzlename : str , typecanned ):
		b = self.CannedPnPTemplate.copy()
		Zconv, zaxisKayo = self.ZaxisConvertZdepth(nozzlename, depth)
		logger.debug('feeder number accessed = {:d}'.format(feedernumber))
		# write depth of plunge after conversion
		microSteps = self.makeZdepthconversion(Zconv)
		zb = microSteps.to_bytes(length=2, byteorder='big')
		b[1:3] = zb
		b[3] = feedernumber
		b[4] = delay
		b[5] = typecanned
		b[6] = (int(zaxisKayo) << 4) | speed
		return b

	def makeNozzleRotationMsg(self, rotation : float , nozzlename : str  , direction : str,speed : int ):
		b = self.NozzRotation.copy()
		zaxisKayo = g.md.headZNameDict[nozzlename][0]
		
		# rotation appears to be 8 x microsteps, and 200 steps per revolution
		
		rb = int((rotation * g.md.ROTATION_SCALE_FACTOR)+0.5 ).to_bytes(length=2, byteorder='big')
		b[2:4] = rb
		
		if direction == 'CW' : 	b[5] = 1
		else : b[5] = 0

		b[6] = (speed << 4 ) | (int(zaxisKayo) & 0xf)
		return b


	def makeRotationMulti(self,direction : list, speed = 1):
		b = self.RotationMulti.copy()
		if 0 :
			b[2] = 0x2
			b[3] = 0xe8
			b[6]  = 0x31
		else :
			b[2] = 0x00	# MSN of [2] is highest nibble of nozzle 3
			b[3] = 0x80 # MSN of [3] is loost nibble of nozzle 3
			
			b[4] = 0x00 # MSN of [4] is highest nibble of nozzle 4... 0xf0 total generates about 400 degrees
			b[5] = 0x80	# MSN = lower nibble of nozzle 4
			
			dirfield = 0
			for i in direction :
				dirfield <<=1
				if i=='CW' :bit = 1
				else : bit = 0
				dirfield |= bit
				
			b[6]  = 0x80 | dirfield | (speed & 0xf)
			
		return b
	
	def makeTubeFeederAction(self,parameter1:int,parameter2:int):
		
		b = self.TubeFeederTemplate.copy()
		b[4] = parameter1
		b[6] = parameter2
		return b
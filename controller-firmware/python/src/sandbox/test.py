from amaranth import *
from amaranth.sim import Simulator
from enum import IntEnum, auto
import numpy as np
from amaranth.back import verilog
import math



class motorSim():

    def __init__(self) -> None:
        self.phaseInductance = 0
        self.phaseResistance = 0
        self.bemfConstant = 0
        self.polePairs = 0
        self.rotorInertia = 0
        
        self.Ucurrent = 0
        self.Vcurrent = 0
        self.Wcurrent = 0
        self.Uvoltage = 0
        self.Vvoltage = 0
        self.Wvoltage = 0

        self.velocity = 0
        self.electricalAngle = 0
        self.rotorAngle = 0

class driveSim():

    def __init__(self) -> None:
        self.shuntResistance = .005

        self.busVoltage = 0
        self.Upwm = 0 # 16 bit unsigned
        self.Vpwm = 0
        self.Wpwm = 0
        self.Ucurrent = 0 # 16 bit signed
        self.Vcurrent = 0
        self.Wcurrent = 0

    def measureCurrent(self, motor: motorSim):
        self.Ucurrent = int(((motor.Ucurrent * self.shuntResistance) / .250) * (2**15 -1))
        self.Vcurrent = int(((motor.Vcurrent * self.shuntResistance) / .250) * (2**15 -1))
        self.Wcurrent = int(((motor.Wcurrent * self.shuntResistance) / .250) * (2**15 -1))

    def simDrive(self, rxPin, txPin, txEnPin):
        pass

class fanucEncoder():

    def __init__(self, mode: str) -> None:
        assert(mode == "rs422" or mode == "rs485")
        self.clock = 10e6

        self.mode = mode

        self.multiturnCount = 0
        self.singleturnCount = 0
        self.commutationCount = 0
        self.battFail = 0
        self.notIndexed = 1

        self.reqPulseCount = 0
        self.sendInProgress = False

    def getBits(self):
        #a860-360 encoder
        data = f"{0b00101:05b}{self.battFail:01b}10{self.notIndexed:01b}{0:09b}{self.singleturnCount:016b}01{self.multiturnCount:016b}01{self.commutationCount:010b}"   #TODO: Add CRC
        return data

    def updateEncoder(self, motor: motorSim):
        newSingleturnCount = int((motor.rotorAngle / (2*np.pi)) * (2**16 -1))

        # if count has jumped by more than half, change the multiturn count
        if (newSingleturnCount - self.singleturnCount > 2**15):
            if (newSingleturnCount > self.singleturnCount):
                self.multiturnCount -= 1
                if (self.multiturnCount == -1):
                    self.multiturnCount = 2**16 -1
            else:
                self.multiturnCount += 1
                if (self.multiturnCount == 2**16):
                    self.multiturnCount = 0

        self.singleturnCount = newSingleturnCount
        self.commutationCount = int((motor.electricalAngle / (2*np.pi)) * (2**10 -1))

# class piController(Elaboratable):

#     def __init__(self, clock):
#         self.clock = clock

#         self.trigger = Signal()
#         self.done = Signal()

#         self.command = Signal(shape=signed(64))     # in
#         self.feedback = Signal(shape=signed(64))    # in

#         self.output = Signal(shape=signed(64))      # out

#         self.divider = Signal(32)   # in
#         self.triggerClockCycles = Signal(32)    # in

#         self.pGain = Signal(32)     # in
#         self.iGain = Signal(32)     # in

#         self.pLimit = Signal(31)    # in
#         self.iLimit = Signal(31)    # in

#         self.pSat = Signal()    # out
#         self.iSat = Signal()    # out

        

        

#     def elaborate(self, platform):
#         m = Module()

#         self.iMem - Signal(shape=signed(32))



# class memoryManager(Elaboratable):

#     def __init__(self, clock, depth):
#         self.clock = clock
#         self.depth = depth

#         self.trigger = Signal()

#         self.inData = Signal(64)
#         self.inAddr = Signal(range(self.depth+1))
#         self.updateIn = Signal()
#         self.inUpdated = Signal()
#         self.outData = Signal(64)
#         self.outAddr = Signal(range(self.depth+1))
#         self.updateOut = Signal()
#         self.outUpdated = Signal()



#     def elaborate(self, platform):
#         m = Module()

#         uart1 = uart(self.clock)

#         mem = Memory(width=64, depth=256)
#         m.submodules["read_port"] = self.readPort = mem.read_port(transparent=False)
#         m.submodules["write_port"] = self.writePort = mem.write_port()
#         m.d.sync += self.writePort.addr.eq(0)
#         m.d.sync += self.writePort.en.eq(1)
#         m.d.sync += self.readPort.addr.eq(0)
#         m.d.sync += self.writePort.addr.eq(self.inData)
#         m.d.sync += self.outData.eq(self.readPort.data)

#         return m



class uart(Elaboratable):
    """
    handles comunication for serial rs422 and rs485 devices

    """

    def __init__(self, clock):

        self.clock = clock

        # Ports

        # Config
        self.baud = Signal(24)

        self.txWordWidth = Signal(8)
        self.txStartBitPolarity = Signal()
        self.txStartBits = Signal(2)
        self.txStopBitPolarity = Signal()
        self.txStopBits = Signal(2)

        self.rxWordWidth = Signal(8)
        self.rxStartBitPolarity = Signal()
        self.rxStartBits = Signal(2)
        self.rxStopBitPolarity = Signal()
        self.rxStopBits = Signal(2)

        # triggers
        self.txStart = Signal()
        self.rxStart = Signal()

        # physical pins
        self.rx = Signal()
        self.tx = Signal(reset=1)
        self.txen = Signal()

        # status
        self.txBusy = Signal()
        self.rxBusy = Signal()
        self.txData = Signal(128)
        self.rxData = Signal(128)
        self.txDataSent = Signal()
        self.rxDataUpdated = Signal()
        self.fault = Signal()


    class txStates(IntEnum):
        IDLE = 0
        START_BITS_DELAY = auto()
        STOP_BITS_DELAY = auto()
        SEND = auto()
        FAULT = auto()

    class rxStates(IntEnum):
        IDLE = 0
        START_BITS_DELAY = auto()
        STOP_BITS_DELAY = auto()
        RECEIVE = auto()
        FAULT = auto()
        


    def elaborate(self, platform):
        m = Module()

        self.txTimer = Signal(range(int(self.clock // (9600 // 8)) + 1))     # a timer that can count up to 8 bits at the lowest baud rate (9600)
        self.txState = Signal(Shape.cast(self.txStates))
        self.txCurrentBit = Signal(8)

        self.rxTimer = Signal(range(int(self.clock // (9600 // 8)) + 1))     # a timer that can count up to 8 bits at the lowest baud rate (9600)
        self.rxState = Signal(Shape.cast(self.rxStates))
        self.rxCurrentBit = Signal(8)

        # Start transmit
        with m.If(self.txStart & (self.txState == self.txStates.IDLE)):
            m.d.sync += self.txDataSent.eq(1)
            m.d.sync += self.txBusy.eq(1)
            m.d.sync += self.tx.eq(self.txStartBitPolarity)
            m.d.sync += self.txen.eq(1)
            m.d.sync += self.txCurrentBit.eq(0)
            m.d.sync += self.txTimer.eq((self.clock // (self.baud)) * self.txStartBits)    # set timer to width of start bits
            m.d.sync += self.txState.eq(self.txStates.START_BITS_DELAY)

        # Send start bits
        with m.If(self.txState == self.txStates.START_BITS_DELAY):
            with m.If(self.txTimer == 0):
                m.d.sync += self.tx.eq(self.txData.bit_select(self.txCurrentBit, 1))    # set tx to first data bit
                m.d.sync += self.txCurrentBit.eq(1)
                m.d.sync += self.txState.eq(self.txStates.SEND)
                m.d.sync += self.txTimer.eq(self.clock // (self.baud) - 1)   # set timer to 1 bit width

            with m.Else():
                m.d.sync += self.txTimer.eq(self.txTimer - 1)

        # Send data bits
        with m.If(self.txState == self.txStates.SEND):
            with m.If(self.txTimer == 0):

                with m.If(self.txCurrentBit == self.txWordWidth):   # word is complete, send stop bits
                    m.d.sync += self.txState.eq(self.txStates.STOP_BITS_DELAY)    
                    m.d.sync += self.tx.eq(self.txStopBitPolarity)
                    m.d.sync += self.txTimer.eq((self.clock // (self.baud)) * self.txStopBits)     # set timer to width of stop bits

                with m.Else():      # continue sending bits
                    m.d.sync += self.tx.eq(self.txData.bit_select(self.txCurrentBit, 1))    # set tx to next data bit
                    m.d.sync += self.txCurrentBit.eq(self.txCurrentBit + 1)     # increment next bit to send
                    m.d.sync += self.txTimer.eq(self.clock // (self.baud) - 1)   # set timer to 1 bit width

                with m.If(self.txCurrentBit == self.txWordWidth - 1):   # we are done with the tx data as soon as we use the last bit
                    m.d.sync += self.txDataSent.eq(1)

            with m.Else():
                m.d.sync += self.txTimer.eq(self.txTimer - 1)

        # Send stop bits
        with m.If(self.txState == self.txStates.STOP_BITS_DELAY):
            with m.If(self.txTimer == 0):
                with m.If(self.txStart):    # start next word if start is triggered
                    m.d.sync += self.tx.eq(self.txStartBitPolarity)
                    m.d.sync += self.txCurrentBit.eq(0)
                    m.d.sync += self.txTimer.eq((self.clock // (self.baud)) * self.txStartBits)    # set timer to width of start bits
                    m.d.sync += self.txState.eq(self.txStates.START_BITS_DELAY)
                    m.d.sync += self.txDataSent.eq(0)

                #TODO: fix last bit lasting 1 clock cycle too long
                
                with m.Else():
                    m.d.sync += self.txCurrentBit.eq(0)
                    m.d.sync += self.txBusy.eq(0)
                    m.d.sync += self.tx.eq(1)
                    m.d.sync += self.txen.eq(0)
                    m.d.sync += self.txState.eq(self.txStates.IDLE)

            with m.Else():
                m.d.sync += self.txTimer.eq(self.txTimer - 1)
                



        
        # Start receive
        with m.If(self.rxStart & (self.rxState == self.rxStates.IDLE)):
            m.d.sync += self.rxDataUpdated.eq(0)
            m.d.sync += self.rxBusy.eq(1)
            m.d.sync += self.rxCurrentBit.eq(0)
            m.d.sync += self.rxState.eq(self.rxStates.START_BITS_DELAY)

        # Wait for start bit edge
        with m.If(self.rxState == self.rxStates.START_BITS_DELAY):
            with m.If(self.rx == self.rxStartBitPolarity):
                m.d.sync += self.rxCurrentBit.eq(0)
                m.d.sync += self.rxState.eq(self.rxStates.RECEIVE)
                m.d.sync += self.rxTimer.eq((self.clock // (self.baud)) * self.rxStartBits + (self.clock // (self.baud * 2)))   # set timer to number of start bits + 1/2 bit

            with m.Else():
                m.d.sync += self.rxTimer.eq(self.rxTimer - 1)

        # receive data bits
        with m.If(self.rxState == self.rxStates.RECEIVE):
            with m.If(self.rxTimer == 0):
                m.d.sync += self.rxData.bit_select(self.txCurrentBit, 1).eq(self.rx)    # save rx bit

                with m.If(self.rxCurrentBit == self.rxWordWidth - 1):   # word is complete, receive stop bits
                    m.d.sync += self.rxState.eq(self.rxStates.STOP_BITS_DELAY)    
                    m.d.sync += self.rxTimer.eq((self.clock // (self.baud)) * self.rxStopBits + (self.clock // (self.baud * 2)))     # set timer to width of stop bits + 1/2

                with m.Else():      # continue receiving bits
                    m.d.sync += self.rxCurrentBit.eq(self.txCurrentBit + 1)     # increment next bit to read
                    m.d.sync += self.rxTimer.eq(self.clock // (self.baud * 2) - 1)   # set timer to 1/2 bit width

                with m.If(self.rxCurrentBit == self.rxWordWidth - 1):   # we are done with the rx data as soon as we save the last bit
                    m.d.sync += self.rxDataUpdated.eq(1)

            with m.Else():
                m.d.sync += self.rxTimer.eq(self.rxTimer - 1)

        # Wait for stop bits
        with m.If(self.rxState == self.rxStates.STOP_BITS_DELAY):
            with m.If(self.rxTimer == 0):
                with m.If(self.rxStart):    # receive next word if start is triggered
                    m.d.sync += self.rxDataUpdated.eq(0)
                    m.d.sync += self.rxBusy.eq(1)
                    m.d.sync += self.rxCurrentBit.eq(0)
                    m.d.sync += self.rxState.eq(self.rxStates.START_BITS_DELAY)
                
                with m.Else():
                    m.d.sync += self.rxCurrentBit.eq(0)
                    m.d.sync += self.rxBusy.eq(0)
                    m.d.sync += self.rxState.eq(self.rxStates.IDLE)

            with m.Else():
                m.d.sync += self.rxTimer.eq(self.rxTimer - 1)
                


        return m

    

class simpleFanucEncoder(Elaboratable):
    """
    handles comunication for serial rs422 devices
    trigger with a positive pulse on txStart shorter than 8us
    """

    def __init__(self, clock):

        self.clock = clock
        self.requsetPulseWidth = 8e-6   # 8us

        self.txIdleLevel = 0
        self.rxIdleLevel = 0
        self.baud = int(1.024e6)     # encoder baudrate
        self.encoderDataWidth = 76

        # triggers
        self.txStart = Signal()
        # physical pins
        self.rx = Signal()
        self.tx = Signal(reset=self.txIdleLevel)
        #self.txen = Signal()

        # status
        self.rxData = Signal(128)
        self.rxDone = Signal(reset=1)
        #self.sampleCounter = Signal()

    class txStates(IntEnum):
        IDLE = 0
        SEND_START_PULSE = auto()
        WAIT = auto()

    class rxStates(IntEnum):
        IDLE = 0
        RECEIVE = auto()
        

    def elaborate(self, platform):
        m = Module()

        self.txTimer = Signal(range(math.ceil(self.clock * self.requsetPulseWidth)))     # a timer that can count up to the tx request pulse width
        self.txState = Signal(Shape.cast(self.txStates))

        self.rxTimer = Signal(range(math.ceil(self.clock / self.baud * 1)))     # a timer that can count up to 1 bit at the baud rate
        self.rxState = Signal(Shape.cast(self.rxStates))
        self.rxCurrentBit = Signal(8)

        self.debounceCycles = 5
        self.debounceCounter = Signal(range(self.debounceCycles)) # debounce counter
        self.debouncedRx = Signal()

        self.oldRxLevel = Signal(reset=~self.rxIdleLevel)

        # debounce rx signal
        with m.If((self.rx == 1) & (self.debounceCounter < self.debounceCycles)):
            m.d.sync += self.debounceCounter.eq(self.debounceCounter + 1)

        with m.If((self.rx == 0) & (self.debounceCounter > 0)):
            m.d.sync += self.debounceCounter.eq(self.debounceCounter - 1)

        with m.If(self.debounceCounter == self.debounceCycles):
            m.d.sync += self.debouncedRx.eq(1)

        with m.If(self.debounceCounter == 0):
            m.d.sync += self.debouncedRx.eq(0)


        # Start sending request pulse
        with m.If((self.txStart) & (self.txState != self.txStates.SEND_START_PULSE)):
            m.d.sync += self.tx.eq(not self.txIdleLevel)
            m.d.sync += self.txTimer.eq(math.ceil(self.clock * self.requsetPulseWidth))
            m.d.sync += self.txState.eq(self.txStates.SEND_START_PULSE)
            m.d.sync += self.rxState.eq(self.rxStates.IDLE)
            m.d.sync += self.rxDone.eq(0)
            #m.d.sync += self.sampleCounter.eq(0)
            m.d.sync += self.rxTimer.eq(self.clock // (self.baud * 2) - 2)   # set timer to 1/2 bit
            m.d.sync += self.rxCurrentBit.eq(0)

        # Finish sending request pulse
        with m.If(self.txState == self.txStates.SEND_START_PULSE):
            m.d.sync += self.tx.eq(not self.txIdleLevel)
            with m.If(self.txTimer == 0):
                m.d.sync += self.txState.eq(self.txStates.WAIT)
                m.d.sync += self.tx.eq(self.txIdleLevel)
            with m.Else():
                m.d.sync += self.txTimer.eq(self.txTimer - 1)

        with m.If((self.txState == self.txStates.WAIT) | (self.txState == self.txStates.IDLE)):
            m.d.sync += self.tx.eq(self.txIdleLevel)
    
        # Start receive
        with m.If((self.txState == self.txStates.WAIT) & ((self.rxState == self.rxStates.IDLE) & (self.debouncedRx != self.rxIdleLevel))):
            m.d.sync += self.rxState.eq(self.rxStates.RECEIVE)
            m.d.sync += self.rxCurrentBit.eq(0)
            #m.d.sync += self.rxTimer.eq(self.clock // (self.baud * 2) - 2)   # set timer to 1/2 bit

        # receive data bits
        with m.If(self.rxState == self.rxStates.RECEIVE):


            with m.If(self.rxTimer == 0):
                #m.d.sync += self.sampleCounter.eq(~self.sampleCounter)
                m.d.sync += self.rxData.bit_select(self.rxCurrentBit, 1).eq(self.debouncedRx)    # save rx bit

                with m.If(self.rxCurrentBit >= self.encoderDataWidth):  # all bits received
                    m.d.sync += self.rxState.eq(self.rxStates.IDLE)
                    m.d.sync += self.txState.eq(self.txStates.IDLE)
                    m.d.sync += self.rxDone.eq(1)

                with m.Else():      # continue receiving bits
                    m.d.sync += self.rxCurrentBit.eq(self.rxCurrentBit + 1)     # increment next bit to read
                    m.d.sync += self.rxTimer.eq(self.clock // (self.baud) - 1)   # set timer to 1 bit width

            #resync on edge changes of rx signal
            with m.Elif(self.debouncedRx == ~self.oldRxLevel):
                 m.d.sync += self.rxTimer.eq(self.clock // (self.baud * 2) - 2)   # set timer to 1/2 bit
                 m.d.sync += self.oldRxLevel.eq(self.debouncedRx)
            
            with m.Else():
                m.d.sync += self.rxTimer.eq(self.rxTimer - 1)

            #with m.If(self.rxTimer > 0 & (self.rx == self.oldRxLevel)):
            #    m.d.sync += self.rxTimer.eq(self.rxTimer - 1)

              
        return m
    

class andTest(Elaboratable):

    def __init__(self, clock):

        self.clock = clock
        
        self.inA = Signal()
        self.inB = Signal()
        self.out = Signal()

    def elaborate(self, platform):
        m = Module()

        # Start sending request pulse
        with m.If(self.inA & (self.inB)):
            m.d.sync += self.out.eq(1)

        with m.Else():
            m.d.sync += self.out.eq(0)

        return m


class i2c(Elaboratable):
    """
    handles comunication for i2c devices

    """

    def __init__(self, clock):

        self.clock = clock

        self.frequency = 400000

        # Ports

        # triggers
        self.start = Signal()

        # physical pins
        self.scl = Signal(reset=1)
        self.sdaOut = Signal(reset=1)
        self.sdaIn = Signal(reset=1)
        self.drvSda = Signal()

        # control
        self.address = Signal(8)
        self.register = Signal(8)
        self.data = Signal(8)
        self.busy = Signal()
        self.fault = Signal()

    class states(IntEnum):
        IDLE = 0
        START = auto()
        START_DELAY = auto()
        SEND = auto()
        VERIFY_ACK = auto()
        STOP_DELAY = auto()
        STOP = auto()
        FAULT = auto()

    class sendStates(IntEnum):
        ADDR = 0
        DATA = auto()
        
    def elaborate(self, platform):
        m = Module()

        self.timer = Signal(range(int(self.clock // (self.frequency // 2)) + 1))     # a timer that can count up to atleast 2 bits
        self.state = Signal(Shape.cast(self.states))
        self.currentBit = Signal(range(16+1))   # handle up to 16bit words

        self.sendSource = Signal(Shape.cast(self.sendStates))

        self.ackBit = Signal()


        # Start
        with m.If(self.start & (self.state == self.states.IDLE)):
            m.d.sync += self.sdaOut.eq(0)
            m.d.sync += self.drvSda.eq(1)
            m.d.sync += self.currentBit.eq(0)
            m.d.sync += self.timer.eq((self.clock // (self.frequency)) // 4)    # set timer to 1/4 clock cycle
            m.d.sync += self.state.eq(self.states.START)

        # Wait to change clk
        with m.If(self.state == self.states.START):
            with m.If(self.timer == 0):
                m.d.sync += self.scl.eq(0)
                m.d.sync += self.state.eq(self.states.START_DELAY)
                m.d.sync += self.timer.eq(self.clock // (self.frequency) // 4)   # set timer to 1/4 clock cycle

            with m.Else():
                m.d.sync += self.timer.eq(self.timer - 1)

        # Change clk then wait to start sending bits
        with m.If(self.state == self.states.START_DELAY):
            with m.If(self.timer == 0):
                m.d.sync += self.state.eq(self.states.SEND)
                m.d.sync += self.timer.eq(self.clock // (self.frequency) // 2)   # set timer to 1/2 clock cycle

            with m.Else():
                m.d.sync += self.timer.eq(self.timer - 1)

        # Send data bits
        with m.If(self.state == self.states.SEND):
            with m.If(self.timer == 0):

                with m.If(self.currentBit == 8):   # word is complete
                    m.d.sync += self.state.eq(self.states.VERIFY_ACK)    
                    m.d.sync += self.sdaOut.eq(0)
                    m.d.sync += self.drvSda.eq(0)
                    m.d.sync += self.timer.eq((self.clock // (self.frequency)))     # set timer to 1 clock cycle

                with m.Else():      # continue sending bits

                    # set data pin to next data bit
                    with m.If(self.sendSource == self.sendStates.ADDR):
                        m.d.sync += self.sdaOut.eq(self.address.bit_select(self.currentBit, 1))
                    with m.If(self.sendSource == self.sendStates.DATA):
                        m.d.sync += self.sdaOut.eq(self.data.bit_select(self.currentBit, 1))  

                    m.d.sync += self.currentBit.eq(self.currentBit + 1)     # increment next bit to send
                    m.d.sync += self.timer.eq((self.clock // (self.frequency)))     # set timer to 1 clock cycle

            with m.Else():
                m.d.sync += self.timer.eq(self.timer - 1)
                
                with m.If(self.timer == (self.clock // (self.frequency) // 4) * 3): # set rising clock edge 1/4 into bit cycle
                    m.d.sync += self.scl.eq(1)

                with m.If(self.timer == (self.clock // (self.frequency) // 4) * 1): # set falling clock edge 3/4 into bit cycle
                    m.d.sync += self.scl.eq(0)

        # Verify ACK bit
        with m.If(self.state == self.states.VERIFY_ACK):
            with m.If(self.timer == 0):

                with m.If(self.ackBit == 0):   # ACK
                    with m.If(self.sendSource == self.sendStates.ADDR):
                        m.d.sync += self.state.eq(self.states.SEND)
                        m.d.sync += self.currentBit.eq(0)
                        m.d.sync += self.sendSource.eq(self.sendStates.DATA)
                    with m.Else():
                        m.d.sync += self.state.eq(self.states.STOP_DELAY)
                        m.d.sync += self.sdaOut.eq(0)

                with m.Else():      # NAK
                    m.d.sync += self.state.eq(self.states.STOP_DELAY)
                    m.d.sync += self.sdaOut.eq(0)

                m.d.sync += self.timer.eq((self.clock // (self.frequency)))     # set timer to 1 clock cycle

            with m.Else():
                m.d.sync += self.timer.eq(self.timer - 1)
                
                with m.If(self.timer == (self.clock // (self.frequency) // 4) * 3): # set rising clock edge 1/4 into bit cycle
                    m.d.sync += self.scl.eq(1)

                with m.If(self.timer == (self.clock // (self.frequency) // 4) * 1): # set falling clock edge 3/4 into bit cycle and check ACK bit
                    m.d.sync += self.scl.eq(0)
                    m.d.sync += self.ackBit.eq(self.sdaIn)

        # Change clk then wait to start sending bits
        with m.If(self.state == self.states.STOP_DELAY):
            with m.If(self.timer == 0):
                m.d.sync += self.scl.eq(1)
                m.d.sync += self.state.eq(self.states.STOP)
                m.d.sync += self.timer.eq(self.clock // (self.frequency) // 4)   # set timer to 1/4 clock cycle

            with m.Else():
                m.d.sync += self.timer.eq(self.timer - 1)

        # Change clk then wait to start sending bits
        with m.If(self.state == self.states.STOP):
            with m.If(self.timer == 0):
                m.d.sync += self.sdaOut.eq(1)
                m.d.sync += self.state.eq(self.states.IDLE)

            with m.Else():
                m.d.sync += self.timer.eq(self.timer - 1)

                
        return m



controlFrequency = 8000
clock = int(50e6) # 50 Mhz
dut = uart(clock)
baud = int(1e6)  # 1 Mbaud
#mem = memoryManager(clock)
def uartBench():
    yield dut.baud.eq(int(10e6))
    yield dut.txWordWidth.eq(40)
    yield dut.txData.eq(0xFF00FF00FF)
    yield dut.txStartBitPolarity.eq(0)
    yield dut.txStartBits.eq(0)
    yield dut.txStopBitPolarity.eq(1)
    yield dut.txStopBits.eq(0)

    yield dut.txStart.eq(1)
    yield
    yield dut.txStart.eq(0)

    for i in range(int(clock / 100000)):
        yield

def bench():
    
    #motor = motorSim()
    encoder = fanucEncoder("rs422")

    # for encoder
    yield dut.baud.eq(baud)
    yield dut.txWordWidth.eq(8)
    yield dut.txData.eq(0b11111111)
    yield dut.txStartBitPolarity.eq(0)
    yield dut.txStartBits.eq(0)
    yield dut.txStopBitPolarity.eq(1)
    yield dut.txStopBits.eq(0)
    yield dut.rx.eq(1)
    

    cycles = 0
    controlClockCycles = int(clock/controlFrequency)
    clockCount = controlClockCycles
    while( cycles < 2):
        if clockCount == 0:
            # send start pulse
            yield dut.txStart.eq(1)
            for i in range(int(clock * 1e-6)):
                        yield
            yield dut.txStart.eq(0)

            cycles += 1
            clockCount = controlClockCycles

    # Sim encoder data
        if encoder.mode == "rs422":
            if (yield dut.tx) == 1:
                encoder.reqPulseCount += 1
            if (yield dut.tx) == 0 and encoder.reqPulseCount != 0:
                if (7.5e-6 < encoder.reqPulseCount/clock < 8.5e-6):
                    data = encoder.getBits()
                     # short delay before transmitting encoder data
                    for i in range(int(clock * 1e-6)):
                        yield

                    bitPeriod = 1/1e6   # 1Mhz
                    for bit in data:
                        if bit == "0":
                            yield dut.rx.eq(1)
                        elif bit == "1":
                            yield dut.rx.eq(0)
                        else:
                            raise Exception(f"Invalid value in bitstream: {bit}")
                        
                        for i in range(int(clock * bitPeriod)):
                            yield
                else:
                    print("Invalid request signal for encoder")
                encoder.reqPulseCount = 0
        clockCount -= 1
        yield

simpleEncoder = simpleFanucEncoder(clock)
def simpleBench():
    
    #motor = motorSim()
    encoder = fanucEncoder("rs422")

    simpleEncoder.rx.eq(simpleEncoder.rxIdleLevel)

    # for encoder
    
    cycles = 0
    controlClockCycles = int(clock/controlFrequency)
    clockCount = 10
    while( cycles < 4):
        if clockCount == 0:
            # send start pulse
            yield simpleEncoder.txStart.eq(1)
            # for i in range(int(clock * 1e-6)):
            #     if (yield simpleEncoder.tx) == 1:
            #         encoder.reqPulseCount += 1
            #     yield
            yield
            yield simpleEncoder.txStart.eq(0)

            cycles += 1
            clockCount = controlClockCycles

        # Sim encoder data
        if (yield simpleEncoder.tx) == 1:
            encoder.reqPulseCount += 1
        if (yield simpleEncoder.tx) == 0 and encoder.reqPulseCount != 0:
            if (7.5e-6 < encoder.reqPulseCount/clock < 8.5e-6):
                data = encoder.getBits()
                    # short delay before transmitting encoder data
                for i in range(int(clock * 5e-6)):
                    yield

                bitPeriod = 1/1.024e6   # 1Mhz
                if (cycles > 2):
                    print(data)
                    for bit in data:
                        if bit == "0":
                            yield simpleEncoder.rx.eq(simpleEncoder.rxIdleLevel)
                            pass
                        elif bit == "1":
                            yield simpleEncoder.rx.eq(not simpleEncoder.rxIdleLevel)
                            pass
                        else:
                            raise Exception(f"Invalid value in bitstream: {bit}")
                        
                        for i in range(int(clock * bitPeriod)):
                            yield

                elif (cycles > 1):
                    for i in range(int(clock * bitPeriod * len(data))):
                        yield simpleEncoder.rx.eq(simpleEncoder.rxIdleLevel)
                        yield

                else:
                    for i in range(int(clock * bitPeriod * len(data))):
                        yield simpleEncoder.rx.eq(not simpleEncoder.rxIdleLevel)
                        yield
            else:
                print(f"Invalid request signal for encoder ({(encoder.reqPulseCount/clock) * 1e6}us)")
            encoder.reqPulseCount = 0
        clockCount -= 1
        yield


i2cInterface = i2c(clock)

def i2cBench():
    yield i2cInterface.address.eq(10)
    yield i2cInterface.data.eq(11)
    yield i2cInterface.sdaIn.eq(0)
    yield
    yield i2cInterface.start.eq(1)
    yield
    yield i2cInterface.start.eq(0)

    for i in range(int(clock / i2cInterface.frequency * 40)):
        yield

sim = Simulator(dut)
sim.add_clock(1/clock)
sim.add_sync_process(uartBench)
with sim.write_vcd("uart.vcd"):
    sim.run()

clock = int(50e6) # 50 Mhz
# i2cInterface = i2c(clock)
uartInterface = uart(clock)
encoderInterface = simpleFanucEncoder(clock)
andtest = andTest(clock)

# with open("i2c.v", "w") as f:
#     f.write(verilog.convert(i2cInterface, ports=[i2cInterface.start, i2cInterface.address, i2cInterface.data, i2cInterface.sdaIn, i2cInterface.sdaOut, i2cInterface.drvSda, i2cInterface.scl]))

# with open("src/amaranth sources/fanucEncoder.v", "w") as f:
#      f.write(verilog.convert(encoderInterface, name="fanucEncoder", ports=[encoderInterface.txStart, encoderInterface.tx, encoderInterface.rx, encoderInterface.rxData, encoderInterface.rxDone]))


with open("src/amaranth sources/uart.v", "w") as f:
    f.write(verilog.convert(uartInterface, name="uart", ports=[uartInterface.baud,
                                                  uartInterface.txWordWidth,
                                                  uartInterface.txData,
                                                  uartInterface.txStartBitPolarity,
                                                  uartInterface.txStartBits,
                                                  uartInterface.txStopBitPolarity,
                                                  uartInterface.txStopBits,
                                                  uartInterface.rxWordWidth,
                                                  uartInterface.rxData,
                                                  uartInterface.rxStartBitPolarity,
                                                  uartInterface.rxStartBits,
                                                  uartInterface.rxStopBitPolarity,
                                                  uartInterface.rxStopBits,
                                                  uartInterface.rx,
                                                  uartInterface.tx,
                                                  uartInterface.txen, 
                                                  uartInterface.txStart,
                                                  uartInterface.rxStart,
                                                  uartInterface.rxDataUpdated,
                                                  uartInterface.txDataSent,
                                                  uartInterface.fault,]))



"""
yield dut.baud.eq(baud)
    # yield dut.txWordWidth.eq(8)
    # yield dut.txData.eq(0b11111111)
    # yield dut.txStartBitPolarity.eq(0)
    # yield dut.txStartBits.eq(0)
    # yield dut.txStopBitPolarity.eq(1)
    # yield dut.txStopBits.eq(0)
    # yield dut.rx.eq(1)
"""
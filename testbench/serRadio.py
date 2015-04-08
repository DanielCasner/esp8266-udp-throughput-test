#!/usr/bin/env python
"""
A test script to go with the ESP8266 UDP throughput test firmware.
This script is specifically for sending formatted packets over UART for relay to UDP.
"""
__author__ = "Daniel Casner"

import sys, time, serial, socket, struct, random

def PacketGenerator(payloadLength, startSequenceNumber=0):
    "A generator for packets. payloadLength specifies how long the packet payload should be, minimum 8 bytes"
    text = open('test_data.txt', 'r')
    payloadLength = max(8, payloadLength)
    strlen = payloadLength - 8
    packer = struct.Struct("<BBIQ%ds" % strlen)
    seqno = startSequenceNumber
    while True:
        try:
            pld = text.read(strlen)
        except:
            break
        yield seqno, packer.pack(0xbe, 0xef, payloadLength, seqno, pld)
        seqno += 1

def unpackSeqno(pkt):
    assert len(pkt) >= 8
    return struct.unpack("Q", pkt[:8])[0]

class UARTRadioConn(serial.Serial):
    "A subclass of Serial with the right parameters"
    def __init__(self, port="/dev/ttyUSB0"):
        serial.Serial.__init__(self, port, 115200, timeout=0)

class UDPClient(socket.socket):
    "A wrapper around socket with the right parameters and some extra functionality"
    def __init__(self, host, port):
        socket.socket.__init__(self, socket.AF_INET, socket.SOCK_DGRAM)
        self.addr = host, port
        self.setblocking = 0

    def connect(self):
        "Send a wakeup message to the radio to establish a UDP \"connection\""
        self.sendto(b"0", self.addr)

    def receive(self):
        "Receive a packet or None"
        try:
            return self.recv(1500)
        except:
            return None

class Tester:
    "A class to administer testing"

    def __init__(self, serial_device="/dev/ttyUSB0", udp_host="172.31.1.1", udp_port=6661, payload_length=1400):
        "Sets up the test"
        self.uart   = UARTRadioConn(serial_device)
        self.client = UDPClient(udp_host, udp_port)
        self.pktGen = PacketGenerator(payload_length)
        self.pktLog = {}

    def connect(self):
        "Start UDP \"connection\""
        self.client.connect()

    def sendOne(self):
        "Send one packet over serial"
        s, p = self.pktGen.next()
        self.pktLog[s] = time.time()
        self.uart.write(p)

    def run(self, interval=1.0):
        "Run a test with a specified interval between packets"
        self.connect()
        while True:
            st = time.time()
            self.sendOne()
            while (time.time()-st) < interval:
                sys.stdout.write(self.uart.read(120))
                pkt = self.client.receive()
                rt = time.time()
                if pkt:
                    seqno = unpackSeqno(pkt)
                    if seqno in self.pktLog:
                        sys.stdout.write("RTT: %f ms\r\n" % ((rt-self.pktLog[seqno])*1000.0))
                        del self.pktLog[seqno]
                    else:
                        sys.stdout.write("Unexpected packet: \"%s\"\r\n" % pkt)
                time.sleep(0.0003)


if __name__ == '__main__':
    Tester().run()

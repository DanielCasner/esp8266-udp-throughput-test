#!/usr/bin/env python

import sys, socket, time

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

if len(sys.argv) > 1:
    interval = float(eval(sys.argv[1]))
else:
    interval = 1.0
if len(sys.argv) > 2:
    size = int(eval(sys.argv[2]))
else:
    size = 1420

s.settimeout(interval)

try:
    while True:
        s.sendto(b"\xa5"*size, ("172.31.1.1", 5551))
        tick = time.time()
        try:
            r, a = s.recvfrom(1500)
        except socket.timeout:
            sys.stdout.write("Timeout\r\n")
        else:
            delta = time.time()-tick
            sys.stdout.write("{:f}ms\r\n".format(delta*1000))
except KeyboardInterrupt:
    sys.exit(0)

        

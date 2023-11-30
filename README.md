# UDP Robust Design

Lab 6 of Intro to Network programming

The client and server programs aims to send files over UDP with a reliable approach.

## Protocol Design
```
seq: sequence generated by server or client
ACK: 
fin: set to 1 when finish transfering all the files
cksum: checksum of only payload
filename:
fileEnd: 
(payload)
```

## Strategy
- cumulative ACK
- If a gap is detected
    - reACK the sequence number of last received packet
    - discard newly received packets (sliding window: Go Back N)
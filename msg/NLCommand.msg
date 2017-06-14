# networking layer command and its subject

uint8 UNDEFINED = 0

# commands
uint8 GET = 1
uint8 SET = 2
uint8 DELETE = 3
uint8 RESET = 4
uint8 CLEAR = 5
uint8 SEND = 6
uint8 RECV = 7
uint8 START = 8
uint8 STOP = 9
uint8 FLUSH = 10

uint8 id

# subjects
uint8 ADDRESS = 1
uint8 PROTOCOL = 2
uint8 PROTOCOLS = 3
uint8 PROTOCOLINFO = 4
uint8 ROUTING = 5
uint8 NEIGHBOUR = 6
uint8 NEIGHBOURS = 7
uint8 STATE = 8
uint8 STATES = 9
uint8 STATISTICS = 10
uint8 VERSION = 11
uint8 DATA = 12
uint8 DISCOVERY = 13
uint8 POLLING = 14
uint8 BUFFER = 15

uint8 subject

# status
uint8 OK = 1
uint8 ERROR = 2
uint8 BUSY = 3
uint8 EMPTY = 4

uint8 status


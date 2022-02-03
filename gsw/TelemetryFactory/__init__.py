from TelemetryFactory import Telemetry
from tlm08A1 import tlm0x08A1_struct
from tlm08B1 import tlm0x08B1_struct
from tlm08C1 import tlm0x08C1_struct
from tlm08D1 import tlm0x08D1_struct
from tlm08D2 import tlm0x08D2_struct
from tlm08D4 import tlm0x08D4_struct

tlm0x08A1 = Telemetry(tlm0x08A1_struct)
tlm0x08B1 = Telemetry(tlm0x08B1_struct)
tlm0x08C1 = Telemetry(tlm0x08C1_struct)
tlm0x08D1 = Telemetry(tlm0x08D1_struct)
tlm0x08D2 = Telemetry(tlm0x08D2_struct)
tlm0x08D4 = Telemetry(tlm0x08D4_struct)

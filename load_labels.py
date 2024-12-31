import re
import idc
import idaapi

def parse_defines_and_apply(filename):
    """
    Reads a .h file with lines like:
      #define F_VISION2 0x0042D030 // optional comment
    Then creates labels in IDA for each address.
    """
    with open(filename, 'r') as f:
        for line in f:
            # Example line: #define F_VISION2 0x0042D030 // x y player
            match = re.match(r'#define\s+(F_\w+)\s+(0x[0-9A-Fa-f]+)', line)
            if match:
                name = match.group(1)
                addr_string = match.group(2)
                try:
                    addr = int(addr_string, 16)
                    # Optionally, ensure address is within your loaded segment, etc.
                    idc.create_name(addr, name)
                    print(f"Created name {name} at {hex(addr)}")
                except ValueError:
                    pass

def main():
    filename = r"defs.h"
    parse_defines_and_apply(filename)

if __name__ == '__main__':
    main()

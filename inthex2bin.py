import sys
from pathlib import Path
from intelhex import IntelHex

assert len(sys.argv) == 2

input_file = Path(sys.argv[1])
hex_data = IntelHex(input_file.as_posix())
output_file = input_file.with_suffix('.bin')
output_file.write_bytes(hex_data.tobinarray())
print('IntelHex to binary output:\n{}'.format(output_file))
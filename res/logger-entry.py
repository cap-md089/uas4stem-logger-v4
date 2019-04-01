from struct import pack

"""
	Serialize double to byte array:

	`pack("<d", 12.345)`

	<d for double, <f for float
	For boolean, just use 255 if True else 0
"""

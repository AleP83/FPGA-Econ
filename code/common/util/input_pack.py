IDS_FILE='../shocks/idshock.txt'
AGS_FILE='../shocks/agshock.txt'
IDS_OUT='../shocks/idshock.dat'
AGS_OUT='../shocks/agshock.dat'
NUM_AGSHOCKS=1100
NUM_IDSHOCKS=(1100 * 10000)


def write_binary_vals(vals, file):
	"""Writes array of integers to file as binary.
	"""
	pos = 0
	curr = 0
	wct = 0
	of = open(file, 'wb')
	for val in vals:
		valmask = val << pos
		curr |= valmask
		pos = pos + 1
		if (pos == 8):
			#print(str(curr))
			of.write(bytes([curr]))
			pos = 0
			curr = 0
			wct = wct + 1
	if (pos != 0):
		print("  Warning: final pos = " + str(pos))
		of.write(bytes([curr]))
		wct = wct + 1
	print("....Finished: " + str(wct) + " bytes written.")
	of.close()
	return
def pack_ags():
	"""Packs agshock.txt to binary agshock.dat.
	Each byte has b0 (lsb) as the earliest shock.
	"""
	vals = [None] * NUM_AGSHOCKS
	idx = 0
	print("Packing agshock...")
	with open(AGS_FILE) as fp:
		for line in fp:
			if line=="1\n":
				vals[idx] = 0
			elif line=="2\n":
				vals[idx] = 1
			idx = idx + 1
	print("  Read in: " + str(idx) + " values.")
	write_binary_vals(vals, AGS_OUT)
	return

def pack_ids():
	"""Packs idshock.txt to binary idshock.dat.
	Each byte has b0 (lsb) as the earliest shock.
	"""
	vals = [None] * NUM_IDSHOCKS
	idx = 0
	print("Packing idshock...")
	with open(IDS_FILE) as fp:
		for line in fp:
			for char in line:
				if char=="1":
					vals[idx] = 0
					idx = idx + 1
				elif char=="2":
					vals[idx] = 1
					idx = idx + 1
	print("  Read in: " + str(idx) + " values.")
	write_binary_vals(vals, IDS_OUT)
	return

def main():
	pack_ags()
	pack_ids()

if __name__ == "__main__":
	main()

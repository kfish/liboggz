import sys

my_cppdefines = {}

# Check endianness
if sys.byteorder == "big":
	print "Host is big endian"
	my_cppdefines['WORDS_BIGENDIAN'] = 1
else:
	print "Host is little endian"

opts = Options()
opts.Add('enable_read', 'Set to 0 to disable reading support', 1)
my_cppdefines['OGGZ_CONFIG_READ'] = '${enable_read}'

opts.Add('enable_write', 'Set to 0 to disable writing support', 1)
my_cppdefines['OGGZ_CONFIG_WRITE'] = '${enable_write}'

env = Environment(options = opts,
                  CPPPATH = '#/scons',
                  CPPDEFINES = my_cppdefines)
Export('env')

SConscript(['src/SConscript'])

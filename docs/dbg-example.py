# 
# Example of Dosbox debugger python bindings
# (c)2011 <samuli.tuomola@gmail.com>
#
# Place this file in ~/.dosbox/python/
#

from dosboxdbg import *
import time

def msg(m):
	ShowMsg('[PY] '+m)

def hexdump(src, length=16):
    result=[]
    for i in xrange(0, len(src), length):
       s = src[i:i+length]
       hexa = b' '.join(["%02X"%ord(x) for x in s])
       text = b''.join([x if 0x20 <= ord(x) < 0x7F else b'.'  for x in s])
       result.append("%-*s %s\n" % (length*3, hexa, text))
    return ''.join(result)

def demo():
	try:
		regs = (eax,ebx,ecx,edx, esi,edi,esp,ebp,eip) = GetRegs()
		segs = (cs,ds,es,fs,gs,ss) = GetSegments()
		msg( 'registers: '+str(regs) )
		msg( 'segments: '+str(segs) )
		msg( 'executing: '+disasm(0xF000, eip, eip) )
		msg( 'display mode: %x' % VgaMode() )
		m = ReadMem(GetAddress(0x0, 0x1200), 14)
		msg( 'some mem: '+hexdump(m) )
		ParseCommand("BP ")
		for h in GetBpoints():
			msg('BP %i. %x:%x' % (h.GetIntNr(), h.GetSegment(), h.GetOffset()))
			#msg(str(dir(h)))
		msg('F5 to continue')
	except Exception as e:
		msg(e)

def shift(seq, n=1):
	return seq[n:]+seq[:n]

def rotpal():
	pal = GetPalette()
	SetPalette( shift(pal,3) )
	if time.time() - starttime > 0.5:
		DontListenFor(VSYNC, rotpal)

def executed(binhash):
	msg('executed binary hash: %x' % binhash)

def logged(tick, logger, mesg):
	if logger == 'VGA' and 'normal' in mesg:
		msg('Hello, press alt-break for some python bindings')

starttime = time.time()

ListenFor(BREAK, demo)
ListenForExec(executed)
ListenForLog(logged)
ListenFor(VSYNC, rotpal)


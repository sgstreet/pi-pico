MEMORY
{
	ROM(xr):                 ORIGIN = 0x00000000, LENGTH = 16K
	BOOTSTRAP_CODE(xr):      ORIGIN = 0x10000000, LENGTH = 256
	XIP(xr):                 ORIGIN = 0x10000100, LENGTH = 2096896
	SRAM(xrw):               ORIGIN = 0x20000000, LENGTH = 256K
	SRAM4(xrw):              ORIGIN = 0x20040000, LENGTH = 4K
	SRAM5(xrw):              ORIGIN = 0x20041000, LENGTH = 4K
	SRAM0(xrw):              ORIGIN = 0x21000000, LENGTH = 64K
	SRAM1(xrw):              ORIGIN = 0x21010000, LENGTH = 64K
	SRAM2(xrw):              ORIGIN = 0x21020000, LENGTH = 64K
	SRAM3(xrw):              ORIGIN = 0x21030000, LENGTH = 64K
	APB(rw):                 ORIGIN = 0x40000000, LENGTH = 256M
	AHB(rw):                 ORIGIN = 0x50000000, LENGTH = 256M
	IOPORT(rw):              ORIGIN = 0xd0000000, LENGTH = 512M
	SYSTEM(rw):              ORIGIN = 0xe0000000, LENGTH = 512M
}

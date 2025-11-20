#pragma once

/**
 * These commands were taken from the datasheet of the AXS15231 - not verified
 * against the LilyGO T-Display S3 Long.
 */

#define OPCODE_WRITE_CMD (0x02ULL)
#define OPCODE_READ_CMD (0x0BULL)
#define OPCODE_WRITE_COLOR (0x32ULL)

#define CMD_NOP 0x00
#define CMD_SWRESET 0x01
#define CMD_RDNUMED 0x05
#define CMD_RDDST 0x09
#define CMD_RDDPM 0x0A
#define CMD_RDDMADCTL 0x0B
#define CMD_RDDIPF 0x0C
#define CMD_RDDIM 0x0D
#define CMD_RDDSM 0x0E
#define CMD_RDDSDR 0x0F

#define CMD_SLPIN 0x10
#define CMD_SLPOUT 0x11
#define CMD_PTLON 0x12
#define CMD_NORON 0x13

#define CMD_INVOFF 0x20
#define CMD_INVON 0x21
#define CMD_ALLPOFF 0x22
#define CMD_ALLPON 0x23
#define CMD_ALLPFILL 0x24
#define CMD_GAMSET 0x26
#define CMD_DISPOFF 0x28
#define CMD_DISPON 0x29
#define CMD_CASET 0x2A
#define CMD_RASET 0x2B
#define CMD_RAMWR 0x2C
#define CMD_RAMRD 0x2E
#define CMD_RAWFILL 0x2F

#define CMD_PTLAR 0x30
#define CMD_PTLCR 0x31
#define CMD_VSCRDEF 0x33
#define CMD_TEOFF 0x34
#define CMD_TEON 0x35
#define CMD_MADCTL 0x36
#define CMD_VSCRSADD 0x37
#define CMD_IDMOFF 0x38
#define CMD_IDMON 0x39
#define CMD_IPF 0x3A
#define CMD_RAMWRC 0x3C
#define CMD_RAMRDC 0x3E

#define CMD_TESCAN 0x44
#define CMD_RDTESCAN 0x45

#define CMD_WRDISBV 0x51
#define CMD_RDDISBV 0x52
#define CMD_WRCTRLD 0x53
#define CMD_RDCTRLD 0x54

#define CMD_RDFCHKSUM 0xAA
#define CMD_RDCCHKSUM 0xAF

#define CMD_RDID1 0xDA
#define CMD_RDID2 0xDB
#define CMD_RDID3 0xDC

#define CMD_DSTB 0x90
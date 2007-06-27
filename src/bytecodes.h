/*
 * there are three types of bytecode.
 *
 * FONT codes (bcf), which change the current font; 0x01, plus a font
 *                                                  attribute byte
 * DLE codes, which give start-of-line indent;      0x10, plus a length byte
 *	                                            (0-96, offset by 32)
 * TAG codes (bct), which manage html tags;         0xFE, the code, then
 *                                                  tag, then 0xFE
 * Any of these codes doubled generates itself (bcf bcf generates ^A, for
 * example)
 */

#define bcfID		0x01
#define	bcfSET_I	'I'
#define bcfCLEAR_I	'i'
#define bcfSET_B	'B'
#define bcfCLEAR_B	'b'
#define bcfSET_TT	'F'
#define bcfCLEAR_TT	'P'
#define bcfFONT(x)	('0'+(x))

#define DLE		0x10

#define bctID		0xFE
#define bctLABEL	'L'
#define bctSET_A	'A'
#define bctCLEAR_A	'a'

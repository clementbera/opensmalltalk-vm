/* Automatically generated from Squeak on #(22 March 2005 1:50:55 pm) */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Default EXPORT macro that does nothing (see comment in sq.h): */
#define EXPORT(returnType) returnType

/* Do not include the entire sq.h file but just those parts needed. */
/*  The virtual machine proxy definition */
#include "sqVirtualMachine.h"
/* Configuration options */
#include "sqConfig.h"
/* Platform specific definitions */
#include "sqPlatformSpecific.h"

#define true 1
#define false 0
#define null 0  /* using 'null' because nil is predefined in Think C */
#ifdef SQUEAK_BUILTIN_PLUGIN
#undef EXPORT
// was #undef EXPORT(returnType) but screws NorCroft cc
#define EXPORT(returnType) static returnType
#endif

/* memory access macros */
#define byteAt(i) (*((unsigned char *) (i)))
#define byteAtput(i, val) (*((unsigned char *) (i)) = val)
#define longAt(i) (*((int *) (i)))
#define longAtput(i, val) (*((int *) (i)) = val)


/*** Constants ***/
#define BlockWidthIndex 5
#define BlueIndex 2
#define ConstBits 13
#define CurrentXIndex 0
#define CurrentYIndex 1
#define DCTSize 8
#define DCTSize2 64
#define FIXn0n298631336 2446
#define FIXn0n34414 22554
#define FIXn0n390180644 3196
#define FIXn0n541196100 4433
#define FIXn0n71414 46802
#define FIXn0n765366865 6270
#define FIXn0n899976223 7373
#define FIXn1n175875602 9633
#define FIXn1n40200 91881
#define FIXn1n501321110 12299
#define FIXn1n77200 116130
#define FIXn1n847759065 15137
#define FIXn1n961570560 16069
#define FIXn2n053119869 16819
#define FIXn2n562915447 20995
#define FIXn3n072711026 25172
#define GreenIndex 1
#define HScaleIndex 2
#define MCUBlockIndex 4
#define MCUWidthIndex 8
#define MaxBits 16
#define MaxMCUBlocks 128
#define MaxSample 255
#define MinComponentSize 11
#define Pass1Bits 2
#define Pass1Div 2048
#define Pass2Div 262144
#define PriorDCValueIndex 10
#define RedIndex 0
#define SampleOffset 127
#define VScaleIndex 3

/*** Function Prototypes ***/
static int cbColorComponentFrom(int oop);
static int colorComponentfrom(int * aColorComponent, int oop);
static int colorComponentBlocksfrom(int ** blocks, int oop);
static int colorConvertGrayscaleMCU(void);
static int colorConvertMCU(void);
static int crColorComponentFrom(int oop);
static int decodeBlockIntocomponent(int * anArray, int * aColorComponent);
static int fillBuffer(void);
static int getBits(int requestedBits);
#pragma export on
EXPORT(const char*) getModuleName(void);
#pragma export off
static int halt(void);
static int idctBlockIntqt(int * anArray, int * qt);
static int jpegDecodeValueFromsize(int *table, int tableSize);
static int loadJPEGStreamFrom(int streamOop);
static int msg(char *s);
static int nextSampleCb(void);
static int nextSampleCr(void);
static int nextSampleFromblocks(int * aComponent, int ** aBlockArray);
static int nextSampleY(void);
#pragma export on
EXPORT(int) primitiveColorConvertGrayscaleMCU(void);
EXPORT(int) primitiveColorConvertMCU(void);
EXPORT(int) primitiveDecodeMCU(void);
EXPORT(int) primitiveIdctInt(void);
#pragma export off
static int scaleAndSignExtendinFieldWidth(int aNumber, int w);
#pragma export on
EXPORT(int) setInterpreter(struct VirtualMachine* anInterpreter);
#pragma export off
static int stInit(void);
static int storeJPEGStreamOn(int streamOop);
static int yColorComponentFrom(int oop);
/*** Variables ***/
static int *acTable;
static int acTableSize;
static int *cbBlocks[128];
static int cbComponent[11];
static int cbSampleStream;
static int *crBlocks[128];
static int crComponent[11];
static int crSampleStream;
static int *dcTable;
static int dcTableSize;
static int ditherMask;

#ifdef SQUEAK_BUILTIN_PLUGIN
extern
#endif
struct VirtualMachine* interpreterProxy;
static int *jpegBits;
static int jpegBitsSize;
static int jpegNaturalOrder[64] = {
	0, 1, 8, 16, 9, 2, 3, 10, 
	17, 24, 32, 25, 18, 11, 4, 5, 
	12, 19, 26, 33, 40, 48, 41, 34, 
	27, 20, 13, 6, 7, 14, 21, 28, 
	35, 42, 49, 56, 57, 50, 43, 36, 
	29, 22, 15, 23, 30, 37, 44, 51, 
	58, 59, 52, 45, 38, 31, 39, 46, 
	53, 60, 61, 54, 47, 55, 62, 63
};
static int jsBitBuffer;
static int jsBitCount;
static unsigned char *jsCollection;
static int jsPosition;
static int jsReadLimit;
static const char *moduleName =
#ifdef SQUEAK_BUILTIN_PLUGIN
	"JPEGReaderPlugin 22 March 2005 (i)"
#else
	"JPEGReaderPlugin 22 March 2005 (e)"
#endif
;
static int *residuals;
static int *yBlocks[128];
static int yComponent[11];
static int ySampleStream;


static int cbColorComponentFrom(int oop) {
	return (colorComponentfrom(cbComponent, oop)) && (colorComponentBlocksfrom(cbBlocks, oop));
}

static int colorComponentfrom(int * aColorComponent, int oop) {
	if ((oop & 1)) {
		return 0;
	}
	if (!(interpreterProxy->isPointers(oop))) {
		return 0;
	}
	if ((interpreterProxy->slotSizeOf(oop)) < MinComponentSize) {
		return 0;
	}
	aColorComponent[CurrentXIndex] = (interpreterProxy->fetchIntegerofObject(CurrentXIndex, oop));
	aColorComponent[CurrentYIndex] = (interpreterProxy->fetchIntegerofObject(CurrentYIndex, oop));
	aColorComponent[HScaleIndex] = (interpreterProxy->fetchIntegerofObject(HScaleIndex, oop));
	aColorComponent[VScaleIndex] = (interpreterProxy->fetchIntegerofObject(VScaleIndex, oop));
	aColorComponent[BlockWidthIndex] = (interpreterProxy->fetchIntegerofObject(BlockWidthIndex, oop));
	aColorComponent[MCUWidthIndex] = (interpreterProxy->fetchIntegerofObject(MCUWidthIndex, oop));
	aColorComponent[PriorDCValueIndex] = (interpreterProxy->fetchIntegerofObject(PriorDCValueIndex, oop));
	return !(interpreterProxy->failed());
}

static int colorComponentBlocksfrom(int ** blocks, int oop) {
    int i;
    int blockOop;
    int arrayOop;
    int max;

	if ((oop & 1)) {
		return 0;
	}
	if (!(interpreterProxy->isPointers(oop))) {
		return 0;
	}
	if ((interpreterProxy->slotSizeOf(oop)) < MinComponentSize) {
		return 0;
	}
	arrayOop = interpreterProxy->fetchPointerofObject(MCUBlockIndex, oop);
	if ((arrayOop & 1)) {
		return 0;
	}
	if (!(interpreterProxy->isPointers(arrayOop))) {
		return 0;
	}
	max = interpreterProxy->slotSizeOf(arrayOop);
	if (max > MaxMCUBlocks) {
		return 0;
	}
	for (i = 0; i <= (max - 1); i += 1) {
		blockOop = interpreterProxy->fetchPointerofObject(i, arrayOop);
		if ((blockOop & 1)) {
			return 0;
		}
		if (!(interpreterProxy->isWords(blockOop))) {
			return 0;
		}
		if (!((interpreterProxy->slotSizeOf(blockOop)) == DCTSize2)) {
			return 0;
		}
		blocks[i] = (interpreterProxy->firstIndexableField(blockOop));
	}
	return !(interpreterProxy->failed());
}

static int colorConvertGrayscaleMCU(void) {
    int i;
    int y;
    int sy;
    int dx;
    int sampleIndex;
    int sx;
    int dy;
    int sample;
    int blockIndex;
    int curX;

	yComponent[CurrentXIndex] = 0;
	yComponent[CurrentYIndex] = 0;
	for (i = 0; i <= (jpegBitsSize - 1); i += 1) {
		/* begin nextSampleY */
		dx = curX = yComponent[CurrentXIndex];
		dy = yComponent[CurrentYIndex];
		sx = yComponent[HScaleIndex];
		sy = yComponent[VScaleIndex];
		if (!((sx == 0) && (sy == 0))) {
			dx = dx / sx;
			dy = dy / sy;
		}
		blockIndex = ((((unsigned) dy >> 3)) * (yComponent[BlockWidthIndex])) + (((unsigned) dx >> 3));
		sampleIndex = (((unsigned) (dy & 7) << 3)) + (dx & 7);
		sample = (yBlocks[blockIndex])[sampleIndex];
		curX += 1;
		if (curX < ((yComponent[MCUWidthIndex]) * 8)) {
			yComponent[CurrentXIndex] = curX;
		} else {
			yComponent[CurrentXIndex] = 0;
			yComponent[CurrentYIndex] = ((yComponent[CurrentYIndex]) + 1);
		}
		y = sample;
		y += residuals[GreenIndex];
		y = ((y < MaxSample) ? y : MaxSample);
		residuals[GreenIndex] = (y & ditherMask);
		y = y & (MaxSample - ditherMask);
		y = ((y < 1) ? 1 : y);
		jpegBits[i] = (((4278190080U + (y << 16)) + (y << 8)) + y);
	}
}

static int colorConvertMCU(void) {
    int red;
    int i;
    int cb;
    int blue;
    int y;
    int cr;
    int green;
    int sy;
    int dx;
    int sampleIndex;
    int sx;
    int dy;
    int sample;
    int blockIndex;
    int curX;
    int sy1;
    int dx1;
    int sampleIndex1;
    int sx1;
    int dy1;
    int sample1;
    int blockIndex1;
    int curX1;
    int sy2;
    int dx2;
    int sampleIndex2;
    int sx2;
    int dy2;
    int sample2;
    int blockIndex2;
    int curX2;

	yComponent[CurrentXIndex] = 0;
	yComponent[CurrentYIndex] = 0;
	cbComponent[CurrentXIndex] = 0;
	cbComponent[CurrentYIndex] = 0;
	crComponent[CurrentXIndex] = 0;
	crComponent[CurrentYIndex] = 0;
	for (i = 0; i <= (jpegBitsSize - 1); i += 1) {
		/* begin nextSampleY */
		dx = curX = yComponent[CurrentXIndex];
		dy = yComponent[CurrentYIndex];
		sx = yComponent[HScaleIndex];
		sy = yComponent[VScaleIndex];
		if (!((sx == 0) && (sy == 0))) {
			dx = dx / sx;
			dy = dy / sy;
		}
		blockIndex = ((((unsigned) dy >> 3)) * (yComponent[BlockWidthIndex])) + (((unsigned) dx >> 3));
		sampleIndex = (((unsigned) (dy & 7) << 3)) + (dx & 7);
		sample = (yBlocks[blockIndex])[sampleIndex];
		curX += 1;
		if (curX < ((yComponent[MCUWidthIndex]) * 8)) {
			yComponent[CurrentXIndex] = curX;
		} else {
			yComponent[CurrentXIndex] = 0;
			yComponent[CurrentYIndex] = ((yComponent[CurrentYIndex]) + 1);
		}
		y = sample;
		/* begin nextSampleCb */
		dx1 = curX1 = cbComponent[CurrentXIndex];
		dy1 = cbComponent[CurrentYIndex];
		sx1 = cbComponent[HScaleIndex];
		sy1 = cbComponent[VScaleIndex];
		if (!((sx1 == 0) && (sy1 == 0))) {
			dx1 = dx1 / sx1;
			dy1 = dy1 / sy1;
		}
		blockIndex1 = ((((unsigned) dy1 >> 3)) * (cbComponent[BlockWidthIndex])) + (((unsigned) dx1 >> 3));
		sampleIndex1 = (((unsigned) (dy1 & 7) << 3)) + (dx1 & 7);
		sample1 = (cbBlocks[blockIndex1])[sampleIndex1];
		curX1 += 1;
		if (curX1 < ((cbComponent[MCUWidthIndex]) * 8)) {
			cbComponent[CurrentXIndex] = curX1;
		} else {
			cbComponent[CurrentXIndex] = 0;
			cbComponent[CurrentYIndex] = ((cbComponent[CurrentYIndex]) + 1);
		}
		cb = sample1;
		cb -= SampleOffset;
		/* begin nextSampleCr */
		dx2 = curX2 = crComponent[CurrentXIndex];
		dy2 = crComponent[CurrentYIndex];
		sx2 = crComponent[HScaleIndex];
		sy2 = crComponent[VScaleIndex];
		if (!((sx2 == 0) && (sy2 == 0))) {
			dx2 = dx2 / sx2;
			dy2 = dy2 / sy2;
		}
		blockIndex2 = ((((unsigned) dy2 >> 3)) * (crComponent[BlockWidthIndex])) + (((unsigned) dx2 >> 3));
		sampleIndex2 = (((unsigned) (dy2 & 7) << 3)) + (dx2 & 7);
		sample2 = (crBlocks[blockIndex2])[sampleIndex2];
		curX2 += 1;
		if (curX2 < ((crComponent[MCUWidthIndex]) * 8)) {
			crComponent[CurrentXIndex] = curX2;
		} else {
			crComponent[CurrentXIndex] = 0;
			crComponent[CurrentYIndex] = ((crComponent[CurrentYIndex]) + 1);
		}
		cr = sample2;
		cr -= SampleOffset;
		red = (y + (((int) (FIXn1n40200 * cr) >> 16))) + (residuals[RedIndex]);
		red = ((red < MaxSample) ? red : MaxSample);
		red = ((red < 0) ? 0 : red);
		residuals[RedIndex] = (red & ditherMask);
		red = red & (MaxSample - ditherMask);
		red = ((red < 1) ? 1 : red);
		green = ((y - (((int) (FIXn0n34414 * cb) >> 16))) - (((int) (FIXn0n71414 * cr) >> 16))) + (residuals[GreenIndex]);
		green = ((green < MaxSample) ? green : MaxSample);
		green = ((green < 0) ? 0 : green);
		residuals[GreenIndex] = (green & ditherMask);
		green = green & (MaxSample - ditherMask);
		green = ((green < 1) ? 1 : green);
		blue = (y + (((int) (FIXn1n77200 * cb) >> 16))) + (residuals[BlueIndex]);
		blue = ((blue < MaxSample) ? blue : MaxSample);
		blue = ((blue < 0) ? 0 : blue);
		residuals[BlueIndex] = (blue & ditherMask);
		blue = blue & (MaxSample - ditherMask);
		blue = ((blue < 1) ? 1 : blue);
		jpegBits[i] = (((4278190080U + (((unsigned) red << 16))) + (((unsigned) green << 8))) + blue);
	}
}

static int crColorComponentFrom(int oop) {
	return (colorComponentfrom(crComponent, oop)) && (colorComponentBlocksfrom(crBlocks, oop));
}

static int decodeBlockIntocomponent(int * anArray, int * aColorComponent) {
    int byte;
    int bits;
    int i;
    int index;
    int zeroCount;

	byte = jpegDecodeValueFromsize(dcTable, dcTableSize);
	if (byte < 0) {
		return interpreterProxy->primitiveFail();
	}
	if (byte != 0) {
		bits = getBits(byte);
		/* begin scaleAndSignExtend:inFieldWidth: */
		if (bits < ((((byte - 1) < 0) ? ((unsigned) 1 >> -(byte - 1)) : ((unsigned) 1 << (byte - 1))))) {
			byte = (bits - (((byte < 0) ? ((unsigned) 1 >> -byte) : ((unsigned) 1 << byte)))) + 1;
			goto l1;
		} else {
			byte = bits;
			goto l1;
		}
	l1:	/* end scaleAndSignExtend:inFieldWidth: */;
	}
	byte = aColorComponent[PriorDCValueIndex] = ((aColorComponent[PriorDCValueIndex]) + byte);
	anArray[0] = byte;
	for (i = 1; i <= (DCTSize2 - 1); i += 1) {
		anArray[i] = 0;
	}
	index = 1;
	while (index < DCTSize2) {
		byte = jpegDecodeValueFromsize(acTable, acTableSize);
		if (byte < 0) {
			return interpreterProxy->primitiveFail();
		}
		zeroCount = ((unsigned) byte) >> 4;
		byte = byte & 15;
		if (byte != 0) {
			index += zeroCount;
			bits = getBits(byte);
			/* begin scaleAndSignExtend:inFieldWidth: */
			if (bits < ((((byte - 1) < 0) ? ((unsigned) 1 >> -(byte - 1)) : ((unsigned) 1 << (byte - 1))))) {
				byte = (bits - (((byte < 0) ? ((unsigned) 1 >> -byte) : ((unsigned) 1 << byte)))) + 1;
				goto l2;
			} else {
				byte = bits;
				goto l2;
			}
		l2:	/* end scaleAndSignExtend:inFieldWidth: */;
			if ((index < 0) || (index >= DCTSize2)) {
				return interpreterProxy->primitiveFail();
			}
			anArray[jpegNaturalOrder[index]] = byte;
		} else {
			if (zeroCount == 15) {
				index += zeroCount;
			} else {
				return null;
			}
		}
		index += 1;
	}
}

static int fillBuffer(void) {
    int byte;

	while (jsBitCount <= 16) {
		if (!(jsPosition < jsReadLimit)) {
			return jsBitCount;
		}
		byte = jsCollection[jsPosition];
		jsPosition += 1;
		if (byte == 255) {
			if (!((jsPosition < jsReadLimit) && ((jsCollection[jsPosition]) == 0))) {
				jsPosition -= 1;
				return jsBitCount;
			}
			jsPosition += 1;
		}
		jsBitBuffer = (((unsigned) jsBitBuffer << 8)) | byte;
		jsBitCount += 8;
	}
	return jsBitCount;
}

static int getBits(int requestedBits) {
    int value;
    int byte;

	if (requestedBits > jsBitCount) {
		/* begin fillBuffer */
		while (jsBitCount <= 16) {
			if (!(jsPosition < jsReadLimit)) {
				goto l1;
			}
			byte = jsCollection[jsPosition];
			jsPosition += 1;
			if (byte == 255) {
				if (!((jsPosition < jsReadLimit) && ((jsCollection[jsPosition]) == 0))) {
					jsPosition -= 1;
					goto l1;
				}
				jsPosition += 1;
			}
			jsBitBuffer = (((unsigned) jsBitBuffer << 8)) | byte;
			jsBitCount += 8;
		}
	l1:	/* end fillBuffer */;
		if (requestedBits > jsBitCount) {
			return -1;
		}
	}
	value = (((requestedBits - jsBitCount) < 0) ? ((unsigned) jsBitBuffer >> -(requestedBits - jsBitCount)) : ((unsigned) jsBitBuffer << (requestedBits - jsBitCount)));
	jsBitBuffer = jsBitBuffer & (((((jsBitCount - requestedBits) < 0) ? ((unsigned) 1 >> -(jsBitCount - requestedBits)) : ((unsigned) 1 << (jsBitCount - requestedBits)))) - 1);
	jsBitCount -= requestedBits;
	return value;
}


/*	Note: This is hardcoded so it can be run from Squeak.
	The module name is used for validating a module *after*
	it is loaded to check if it does really contain the module
	we're thinking it contains. This is important! */

EXPORT(const char*) getModuleName(void) {
	return moduleName;
}

static int halt(void) {
	;
}

static int idctBlockIntqt(int * anArray, int * qt) {
    int t13;
    int z3;
    int dcval;
    int anACTerm;
    int z4;
    int v;
    int t0;
    int row;
    int t10;
    int z5;
    int t1;
    int z1;
    int t11;
    int i;
    int t2;
    int t12;
    int z2;
    int ws[64];
    int j;
    int t3;

	;
	for (i = 0; i <= (DCTSize - 1); i += 1) {
		anACTerm = -1;
		for (row = 1; row <= (DCTSize - 1); row += 1) {
			if (anACTerm == -1) {
				if (!((anArray[(row * DCTSize) + i]) == 0)) {
					anACTerm = row;
				}
			}
		}
		if (anACTerm == -1) {
			dcval = ((unsigned) ((anArray[i]) * (qt[0])) << 2);
			for (j = 0; j <= (DCTSize - 1); j += 1) {
				ws[(j * DCTSize) + i] = dcval;
			}
		} else {
			z2 = (anArray[(DCTSize * 2) + i]) * (qt[(DCTSize * 2) + i]);
			z3 = (anArray[(DCTSize * 6) + i]) * (qt[(DCTSize * 6) + i]);
			z1 = (z2 + z3) * FIXn0n541196100;
			t2 = z1 + (z3 * (0 - FIXn1n847759065));
			t3 = z1 + (z2 * FIXn0n765366865);
			z2 = (anArray[i]) * (qt[i]);
			z3 = (anArray[(DCTSize * 4) + i]) * (qt[(DCTSize * 4) + i]);
			t0 = ((unsigned) (z2 + z3) << 13);
			t1 = ((unsigned) (z2 - z3) << 13);
			t10 = t0 + t3;
			t13 = t0 - t3;
			t11 = t1 + t2;
			t12 = t1 - t2;
			t0 = (anArray[(DCTSize * 7) + i]) * (qt[(DCTSize * 7) + i]);
			t1 = (anArray[(DCTSize * 5) + i]) * (qt[(DCTSize * 5) + i]);
			t2 = (anArray[(DCTSize * 3) + i]) * (qt[(DCTSize * 3) + i]);
			t3 = (anArray[DCTSize + i]) * (qt[DCTSize + i]);
			z1 = t0 + t3;
			z2 = t1 + t2;
			z3 = t0 + t2;
			z4 = t1 + t3;
			z5 = (z3 + z4) * FIXn1n175875602;
			t0 = t0 * FIXn0n298631336;
			t1 = t1 * FIXn2n053119869;
			t2 = t2 * FIXn3n072711026;
			t3 = t3 * FIXn1n501321110;
			z1 = z1 * (0 - FIXn0n899976223);
			z2 = z2 * (0 - FIXn2n562915447);
			z3 = z3 * (0 - FIXn1n961570560);
			z4 = z4 * (0 - FIXn0n390180644);
			z3 += z5;
			z4 += z5;
			t0 = (t0 + z1) + z3;
			t1 = (t1 + z2) + z4;
			t2 = (t2 + z2) + z3;
			t3 = (t3 + z1) + z4;
			ws[i] = (((int) (t10 + t3) >> 11));
			ws[(DCTSize * 7) + i] = (((int) (t10 - t3) >> 11));
			ws[(DCTSize * 1) + i] = (((int) (t11 + t2) >> 11));
			ws[(DCTSize * 6) + i] = (((int) (t11 - t2) >> 11));
			ws[(DCTSize * 2) + i] = (((int) (t12 + t1) >> 11));
			ws[(DCTSize * 5) + i] = (((int) (t12 - t1) >> 11));
			ws[(DCTSize * 3) + i] = (((int) (t13 + t0) >> 11));
			ws[(DCTSize * 4) + i] = (((int) (t13 - t0) >> 11));
		}
	}
	for (i = 0; i <= (DCTSize2 - DCTSize); i += DCTSize) {
		z2 = ws[i + 2];
		z3 = ws[i + 6];
		z1 = (z2 + z3) * FIXn0n541196100;
		t2 = z1 + (z3 * (0 - FIXn1n847759065));
		t3 = z1 + (z2 * FIXn0n765366865);
		t0 = ((unsigned) ((ws[i]) + (ws[i + 4])) << 13);
		t1 = ((unsigned) ((ws[i]) - (ws[i + 4])) << 13);
		t10 = t0 + t3;
		t13 = t0 - t3;
		t11 = t1 + t2;
		t12 = t1 - t2;
		t0 = ws[i + 7];
		t1 = ws[i + 5];
		t2 = ws[i + 3];
		t3 = ws[i + 1];
		z1 = t0 + t3;
		z2 = t1 + t2;
		z3 = t0 + t2;
		z4 = t1 + t3;
		z5 = (z3 + z4) * FIXn1n175875602;
		t0 = t0 * FIXn0n298631336;
		t1 = t1 * FIXn2n053119869;
		t2 = t2 * FIXn3n072711026;
		t3 = t3 * FIXn1n501321110;
		z1 = z1 * (0 - FIXn0n899976223);
		z2 = z2 * (0 - FIXn2n562915447);
		z3 = z3 * (0 - FIXn1n961570560);
		z4 = z4 * (0 - FIXn0n390180644);
		z3 += z5;
		z4 += z5;
		t0 = (t0 + z1) + z3;
		t1 = (t1 + z2) + z4;
		t2 = (t2 + z2) + z3;
		t3 = (t3 + z1) + z4;
		v = (((int) (t10 + t3) >> 18)) + SampleOffset;
		v = ((v < MaxSample) ? v : MaxSample);
		v = ((v < 0) ? 0 : v);
		anArray[i] = v;
		v = (((int) (t10 - t3) >> 18)) + SampleOffset;
		v = ((v < MaxSample) ? v : MaxSample);
		v = ((v < 0) ? 0 : v);
		anArray[i + 7] = v;
		v = (((int) (t11 + t2) >> 18)) + SampleOffset;
		v = ((v < MaxSample) ? v : MaxSample);
		v = ((v < 0) ? 0 : v);
		anArray[i + 1] = v;
		v = (((int) (t11 - t2) >> 18)) + SampleOffset;
		v = ((v < MaxSample) ? v : MaxSample);
		v = ((v < 0) ? 0 : v);
		anArray[i + 6] = v;
		v = (((int) (t12 + t1) >> 18)) + SampleOffset;
		v = ((v < MaxSample) ? v : MaxSample);
		v = ((v < 0) ? 0 : v);
		anArray[i + 2] = v;
		v = (((int) (t12 - t1) >> 18)) + SampleOffset;
		v = ((v < MaxSample) ? v : MaxSample);
		v = ((v < 0) ? 0 : v);
		anArray[i + 5] = v;
		v = (((int) (t13 + t0) >> 18)) + SampleOffset;
		v = ((v < MaxSample) ? v : MaxSample);
		v = ((v < 0) ? 0 : v);
		anArray[i + 3] = v;
		v = (((int) (t13 - t0) >> 18)) + SampleOffset;
		v = ((v < MaxSample) ? v : MaxSample);
		v = ((v < 0) ? 0 : v);
		anArray[i + 4] = v;
	}
}


/*	Decode the next value in the receiver using the given huffman table. */

static int jpegDecodeValueFromsize(int *table, int tableSize) {
    int bits;
    int tableIndex;
    int index;
    int value;
    int bitsNeeded;


	/* Initial bits needed */

	bitsNeeded = ((unsigned) (table[0])) >> 24;
	if (bitsNeeded > MaxBits) {
		return -1;
	}

	/* First real table */

	tableIndex = 2;
	while (1) {

		/* Get bits */

		bits = getBits(bitsNeeded);
		if (bits < 0) {
			return -1;
		}
		index = (tableIndex + bits) - 1;
		if (index >= tableSize) {
			return -1;
		}

		/* Lookup entry in table */

		value = table[index];
		if ((value & 1056964608) == 0) {
			return value;
		}

		/* Table offset in low 16 bit */

		tableIndex = value & 65535;

		/* Additional bits in high 8 bit */

		bitsNeeded = (((unsigned) value) >> 24) & 255;
		if (bitsNeeded > MaxBits) {
			return -1;
		}
	}
	return -1;
}

static int loadJPEGStreamFrom(int streamOop) {
    int oop;
    int sz;

	if ((interpreterProxy->slotSizeOf(streamOop)) < 5) {
		return 0;
	}
	if (!(interpreterProxy->isPointers(streamOop))) {
		return 0;
	}
	oop = interpreterProxy->fetchPointerofObject(0, streamOop);
	if ((oop & 1)) {
		return 0;
	}
	if (!(interpreterProxy->isBytes(oop))) {
		return 0;
	}
	jsCollection = interpreterProxy->firstIndexableField(oop);
	sz = interpreterProxy->byteSizeOf(oop);
	jsPosition = interpreterProxy->fetchIntegerofObject(1, streamOop);
	jsReadLimit = interpreterProxy->fetchIntegerofObject(2, streamOop);
	jsBitBuffer = interpreterProxy->fetchIntegerofObject(3, streamOop);
	jsBitCount = interpreterProxy->fetchIntegerofObject(4, streamOop);
	if (interpreterProxy->failed()) {
		return 0;
	}
	if (sz < jsReadLimit) {
		return 0;
	}
	if ((jsPosition < 0) || (jsPosition >= jsReadLimit)) {
		return 0;
	}
	return 1;
}

static int msg(char *s) {
	fprintf(stderr, "\n%s: %s", moduleName, s);
}

static int nextSampleCb(void) {
    int sy;
    int dx;
    int sampleIndex;
    int sx;
    int dy;
    int sample;
    int blockIndex;
    int curX;

	dx = curX = cbComponent[CurrentXIndex];
	dy = cbComponent[CurrentYIndex];
	sx = cbComponent[HScaleIndex];
	sy = cbComponent[VScaleIndex];
	if (!((sx == 0) && (sy == 0))) {
		dx = dx / sx;
		dy = dy / sy;
	}
	blockIndex = ((((unsigned) dy >> 3)) * (cbComponent[BlockWidthIndex])) + (((unsigned) dx >> 3));
	sampleIndex = (((unsigned) (dy & 7) << 3)) + (dx & 7);
	sample = (cbBlocks[blockIndex])[sampleIndex];
	curX += 1;
	if (curX < ((cbComponent[MCUWidthIndex]) * 8)) {
		cbComponent[CurrentXIndex] = curX;
	} else {
		cbComponent[CurrentXIndex] = 0;
		cbComponent[CurrentYIndex] = ((cbComponent[CurrentYIndex]) + 1);
	}
	return sample;
}

static int nextSampleCr(void) {
    int sy;
    int dx;
    int sampleIndex;
    int sx;
    int dy;
    int sample;
    int blockIndex;
    int curX;

	dx = curX = crComponent[CurrentXIndex];
	dy = crComponent[CurrentYIndex];
	sx = crComponent[HScaleIndex];
	sy = crComponent[VScaleIndex];
	if (!((sx == 0) && (sy == 0))) {
		dx = dx / sx;
		dy = dy / sy;
	}
	blockIndex = ((((unsigned) dy >> 3)) * (crComponent[BlockWidthIndex])) + (((unsigned) dx >> 3));
	sampleIndex = (((unsigned) (dy & 7) << 3)) + (dx & 7);
	sample = (crBlocks[blockIndex])[sampleIndex];
	curX += 1;
	if (curX < ((crComponent[MCUWidthIndex]) * 8)) {
		crComponent[CurrentXIndex] = curX;
	} else {
		crComponent[CurrentXIndex] = 0;
		crComponent[CurrentYIndex] = ((crComponent[CurrentYIndex]) + 1);
	}
	return sample;
}

static int nextSampleFromblocks(int * aComponent, int ** aBlockArray) {
    int sy;
    int dx;
    int sampleIndex;
    int sx;
    int dy;
    int sample;
    int blockIndex;
    int curX;

	dx = curX = aComponent[CurrentXIndex];
	dy = aComponent[CurrentYIndex];
	sx = aComponent[HScaleIndex];
	sy = aComponent[VScaleIndex];
	if (!((sx == 0) && (sy == 0))) {
		dx = dx / sx;
		dy = dy / sy;
	}
	blockIndex = ((((unsigned) dy >> 3)) * (aComponent[BlockWidthIndex])) + (((unsigned) dx >> 3));
	sampleIndex = (((unsigned) (dy & 7) << 3)) + (dx & 7);
	sample = (aBlockArray[blockIndex])[sampleIndex];
	curX += 1;
	if (curX < ((aComponent[MCUWidthIndex]) * 8)) {
		aComponent[CurrentXIndex] = curX;
	} else {
		aComponent[CurrentXIndex] = 0;
		aComponent[CurrentYIndex] = ((aComponent[CurrentYIndex]) + 1);
	}
	return sample;
}

static int nextSampleY(void) {
    int sy;
    int dx;
    int sampleIndex;
    int sx;
    int dy;
    int sample;
    int blockIndex;
    int curX;

	dx = curX = yComponent[CurrentXIndex];
	dy = yComponent[CurrentYIndex];
	sx = yComponent[HScaleIndex];
	sy = yComponent[VScaleIndex];
	if (!((sx == 0) && (sy == 0))) {
		dx = dx / sx;
		dy = dy / sy;
	}
	blockIndex = ((((unsigned) dy >> 3)) * (yComponent[BlockWidthIndex])) + (((unsigned) dx >> 3));
	sampleIndex = (((unsigned) (dy & 7) << 3)) + (dx & 7);
	sample = (yBlocks[blockIndex])[sampleIndex];
	curX += 1;
	if (curX < ((yComponent[MCUWidthIndex]) * 8)) {
		yComponent[CurrentXIndex] = curX;
	} else {
		yComponent[CurrentXIndex] = 0;
		yComponent[CurrentYIndex] = ((yComponent[CurrentYIndex]) + 1);
	}
	return sample;
}


/*	Requires:
		JPEGColorComponent
		bits
		WordArray with: 3*Integer (residuals)
		ditherMask
	 */

EXPORT(int) primitiveColorConvertGrayscaleMCU(void) {
    int arrayOop;
    int i;
    int y;
    int sy;
    int dx;
    int sampleIndex;
    int sx;
    int dy;
    int sample;
    int blockIndex;
    int curX;

	stInit();
	if (!((interpreterProxy->methodArgumentCount()) == 4)) {
		return interpreterProxy->primitiveFail();
	}
	ditherMask = interpreterProxy->stackIntegerValue(0);
	arrayOop = interpreterProxy->stackObjectValue(1);
	if (interpreterProxy->failed()) {
		return null;
	}
	if (!((interpreterProxy->isWords(arrayOop)) && ((interpreterProxy->slotSizeOf(arrayOop)) == 3))) {
		return interpreterProxy->primitiveFail();
	}
	residuals = interpreterProxy->firstIndexableField(arrayOop);
	arrayOop = interpreterProxy->stackObjectValue(2);
	if (interpreterProxy->failed()) {
		return null;
	}
	if (!(interpreterProxy->isWords(arrayOop))) {
		return interpreterProxy->primitiveFail();
	}
	jpegBitsSize = interpreterProxy->slotSizeOf(arrayOop);
	jpegBits = interpreterProxy->firstIndexableField(arrayOop);
	arrayOop = interpreterProxy->stackObjectValue(3);
	if (interpreterProxy->failed()) {
		return null;
	}
	if (!((colorComponentfrom(yComponent, arrayOop)) && (colorComponentBlocksfrom(yBlocks, arrayOop)))) {
		return interpreterProxy->primitiveFail();
	}
	/* begin colorConvertGrayscaleMCU */
	yComponent[CurrentXIndex] = 0;
	yComponent[CurrentYIndex] = 0;
	for (i = 0; i <= (jpegBitsSize - 1); i += 1) {
		/* begin nextSampleY */
		dx = curX = yComponent[CurrentXIndex];
		dy = yComponent[CurrentYIndex];
		sx = yComponent[HScaleIndex];
		sy = yComponent[VScaleIndex];
		if (!((sx == 0) && (sy == 0))) {
			dx = dx / sx;
			dy = dy / sy;
		}
		blockIndex = ((((unsigned) dy >> 3)) * (yComponent[BlockWidthIndex])) + (((unsigned) dx >> 3));
		sampleIndex = (((unsigned) (dy & 7) << 3)) + (dx & 7);
		sample = (yBlocks[blockIndex])[sampleIndex];
		curX += 1;
		if (curX < ((yComponent[MCUWidthIndex]) * 8)) {
			yComponent[CurrentXIndex] = curX;
		} else {
			yComponent[CurrentXIndex] = 0;
			yComponent[CurrentYIndex] = ((yComponent[CurrentYIndex]) + 1);
		}
		y = sample;
		y += residuals[GreenIndex];
		y = ((y < MaxSample) ? y : MaxSample);
		residuals[GreenIndex] = (y & ditherMask);
		y = y & (MaxSample - ditherMask);
		y = ((y < 1) ? 1 : y);
		jpegBits[i] = (((4278190080U + (y << 16)) + (y << 8)) + y);
	}
	interpreterProxy->pop(4);
}


/*	Requires:
		Array with: 3*JPEGColorComponent
		bits
		WordArray with: 3*Integer (residuals)
		ditherMask
	 */

EXPORT(int) primitiveColorConvertMCU(void) {
    int arrayOop;
    int red;
    int i;
    int cb;
    int blue;
    int y;
    int cr;
    int green;
    int sy;
    int dx;
    int sampleIndex;
    int sx;
    int dy;
    int sample;
    int blockIndex;
    int curX;
    int sy1;
    int dx1;
    int sampleIndex1;
    int sx1;
    int dy1;
    int sample1;
    int blockIndex1;
    int curX1;
    int sy2;
    int dx2;
    int sampleIndex2;
    int sx2;
    int dy2;
    int sample2;
    int blockIndex2;
    int curX2;

	stInit();
	if (!((interpreterProxy->methodArgumentCount()) == 4)) {
		return interpreterProxy->primitiveFail();
	}
	ditherMask = interpreterProxy->stackIntegerValue(0);
	arrayOop = interpreterProxy->stackObjectValue(1);
	if (interpreterProxy->failed()) {
		return null;
	}
	if (!((interpreterProxy->isWords(arrayOop)) && ((interpreterProxy->slotSizeOf(arrayOop)) == 3))) {
		return interpreterProxy->primitiveFail();
	}
	residuals = interpreterProxy->firstIndexableField(arrayOop);
	arrayOop = interpreterProxy->stackObjectValue(2);
	if (interpreterProxy->failed()) {
		return null;
	}
	if (!(interpreterProxy->isWords(arrayOop))) {
		return interpreterProxy->primitiveFail();
	}
	jpegBitsSize = interpreterProxy->slotSizeOf(arrayOop);
	jpegBits = interpreterProxy->firstIndexableField(arrayOop);
	arrayOop = interpreterProxy->stackObjectValue(3);
	if (interpreterProxy->failed()) {
		return null;
	}
	if (!((interpreterProxy->isPointers(arrayOop)) && ((interpreterProxy->slotSizeOf(arrayOop)) == 3))) {
		return interpreterProxy->primitiveFail();
	}
	if (!(yColorComponentFrom(interpreterProxy->fetchPointerofObject(0, arrayOop)))) {
		return interpreterProxy->primitiveFail();
	}
	if (!(cbColorComponentFrom(interpreterProxy->fetchPointerofObject(1, arrayOop)))) {
		return interpreterProxy->primitiveFail();
	}
	if (!(crColorComponentFrom(interpreterProxy->fetchPointerofObject(2, arrayOop)))) {
		return interpreterProxy->primitiveFail();
	}
	/* begin colorConvertMCU */
	yComponent[CurrentXIndex] = 0;
	yComponent[CurrentYIndex] = 0;
	cbComponent[CurrentXIndex] = 0;
	cbComponent[CurrentYIndex] = 0;
	crComponent[CurrentXIndex] = 0;
	crComponent[CurrentYIndex] = 0;
	for (i = 0; i <= (jpegBitsSize - 1); i += 1) {
		/* begin nextSampleY */
		dx = curX = yComponent[CurrentXIndex];
		dy = yComponent[CurrentYIndex];
		sx = yComponent[HScaleIndex];
		sy = yComponent[VScaleIndex];
		if (!((sx == 0) && (sy == 0))) {
			dx = dx / sx;
			dy = dy / sy;
		}
		blockIndex = ((((unsigned) dy >> 3)) * (yComponent[BlockWidthIndex])) + (((unsigned) dx >> 3));
		sampleIndex = (((unsigned) (dy & 7) << 3)) + (dx & 7);
		sample = (yBlocks[blockIndex])[sampleIndex];
		curX += 1;
		if (curX < ((yComponent[MCUWidthIndex]) * 8)) {
			yComponent[CurrentXIndex] = curX;
		} else {
			yComponent[CurrentXIndex] = 0;
			yComponent[CurrentYIndex] = ((yComponent[CurrentYIndex]) + 1);
		}
		y = sample;
		/* begin nextSampleCb */
		dx1 = curX1 = cbComponent[CurrentXIndex];
		dy1 = cbComponent[CurrentYIndex];
		sx1 = cbComponent[HScaleIndex];
		sy1 = cbComponent[VScaleIndex];
		if (!((sx1 == 0) && (sy1 == 0))) {
			dx1 = dx1 / sx1;
			dy1 = dy1 / sy1;
		}
		blockIndex1 = ((((unsigned) dy1 >> 3)) * (cbComponent[BlockWidthIndex])) + (((unsigned) dx1 >> 3));
		sampleIndex1 = (((unsigned) (dy1 & 7) << 3)) + (dx1 & 7);
		sample1 = (cbBlocks[blockIndex1])[sampleIndex1];
		curX1 += 1;
		if (curX1 < ((cbComponent[MCUWidthIndex]) * 8)) {
			cbComponent[CurrentXIndex] = curX1;
		} else {
			cbComponent[CurrentXIndex] = 0;
			cbComponent[CurrentYIndex] = ((cbComponent[CurrentYIndex]) + 1);
		}
		cb = sample1;
		cb -= SampleOffset;
		/* begin nextSampleCr */
		dx2 = curX2 = crComponent[CurrentXIndex];
		dy2 = crComponent[CurrentYIndex];
		sx2 = crComponent[HScaleIndex];
		sy2 = crComponent[VScaleIndex];
		if (!((sx2 == 0) && (sy2 == 0))) {
			dx2 = dx2 / sx2;
			dy2 = dy2 / sy2;
		}
		blockIndex2 = ((((unsigned) dy2 >> 3)) * (crComponent[BlockWidthIndex])) + (((unsigned) dx2 >> 3));
		sampleIndex2 = (((unsigned) (dy2 & 7) << 3)) + (dx2 & 7);
		sample2 = (crBlocks[blockIndex2])[sampleIndex2];
		curX2 += 1;
		if (curX2 < ((crComponent[MCUWidthIndex]) * 8)) {
			crComponent[CurrentXIndex] = curX2;
		} else {
			crComponent[CurrentXIndex] = 0;
			crComponent[CurrentYIndex] = ((crComponent[CurrentYIndex]) + 1);
		}
		cr = sample2;
		cr -= SampleOffset;
		red = (y + (((int) (FIXn1n40200 * cr) >> 16))) + (residuals[RedIndex]);
		red = ((red < MaxSample) ? red : MaxSample);
		red = ((red < 0) ? 0 : red);
		residuals[RedIndex] = (red & ditherMask);
		red = red & (MaxSample - ditherMask);
		red = ((red < 1) ? 1 : red);
		green = ((y - (((int) (FIXn0n34414 * cb) >> 16))) - (((int) (FIXn0n71414 * cr) >> 16))) + (residuals[GreenIndex]);
		green = ((green < MaxSample) ? green : MaxSample);
		green = ((green < 0) ? 0 : green);
		residuals[GreenIndex] = (green & ditherMask);
		green = green & (MaxSample - ditherMask);
		green = ((green < 1) ? 1 : green);
		blue = (y + (((int) (FIXn1n77200 * cb) >> 16))) + (residuals[BlueIndex]);
		blue = ((blue < MaxSample) ? blue : MaxSample);
		blue = ((blue < 0) ? 0 : blue);
		residuals[BlueIndex] = (blue & ditherMask);
		blue = blue & (MaxSample - ditherMask);
		blue = ((blue < 1) ? 1 : blue);
		jpegBits[i] = (((4278190080U + (((unsigned) red << 16))) + (((unsigned) green << 8))) + blue);
	}
	interpreterProxy->pop(4);
}


/*	In:
		anArray 		WordArray of: DCTSize2
		aColorComponent JPEGColorComponent
		dcTable			WordArray
		acTable			WordArray
		stream			JPEGStream
	 */

EXPORT(int) primitiveDecodeMCU(void) {
    int oop;
    int * anArray;
    int arrayOop;
    int streamOop;

	;
	if (!((interpreterProxy->methodArgumentCount()) == 5)) {
		return interpreterProxy->primitiveFail();
	}
	oop = interpreterProxy->stackObjectValue(0);
	if (interpreterProxy->failed()) {
		return null;
	}
	if (!(loadJPEGStreamFrom(oop))) {
		return interpreterProxy->primitiveFail();
	}
	arrayOop = interpreterProxy->stackObjectValue(1);
	if (interpreterProxy->failed()) {
		return null;
	}
	if (!(interpreterProxy->isWords(arrayOop))) {
		return interpreterProxy->primitiveFail();
	}
	acTableSize = interpreterProxy->slotSizeOf(arrayOop);
	acTable = interpreterProxy->firstIndexableField(arrayOop);
	arrayOop = interpreterProxy->stackObjectValue(2);
	if (interpreterProxy->failed()) {
		return null;
	}
	if (!(interpreterProxy->isWords(arrayOop))) {
		return interpreterProxy->primitiveFail();
	}
	dcTableSize = interpreterProxy->slotSizeOf(arrayOop);
	dcTable = interpreterProxy->firstIndexableField(arrayOop);
	oop = interpreterProxy->stackObjectValue(3);
	if (interpreterProxy->failed()) {
		return null;
	}
	if (!(colorComponentfrom(yComponent, oop))) {
		return interpreterProxy->primitiveFail();
	}
	arrayOop = interpreterProxy->stackObjectValue(4);
	if (interpreterProxy->failed()) {
		return null;
	}
	if (!(interpreterProxy->isWords(arrayOop))) {
		return interpreterProxy->primitiveFail();
	}
	if (!((interpreterProxy->slotSizeOf(arrayOop)) == DCTSize2)) {
		return interpreterProxy->primitiveFail();
	}
	anArray = interpreterProxy->firstIndexableField(arrayOop);
	if (interpreterProxy->failed()) {
		return null;
	}
	decodeBlockIntocomponent(anArray, yComponent);
	if (interpreterProxy->failed()) {
		return null;
	}
	/* begin storeJPEGStreamOn: */
	streamOop = interpreterProxy->stackValue(0);
	interpreterProxy->storeIntegerofObjectwithValue(1, streamOop, jsPosition);
	interpreterProxy->storeIntegerofObjectwithValue(3, streamOop, jsBitBuffer);
	interpreterProxy->storeIntegerofObjectwithValue(4, streamOop, jsBitCount);
	interpreterProxy->storeIntegerofObjectwithValue(PriorDCValueIndex, interpreterProxy->stackValue(3), yComponent[PriorDCValueIndex]);
	interpreterProxy->pop(5);
}


/*	In:
		anArray: IntegerArray new: DCTSize2
		qt: IntegerArray new: DCTSize2.
	 */

EXPORT(int) primitiveIdctInt(void) {
    int * anArray;
    int arrayOop;
    int * qt;

	;
	if (!((interpreterProxy->methodArgumentCount()) == 2)) {
		return interpreterProxy->primitiveFail();
	}
	arrayOop = interpreterProxy->stackObjectValue(0);
	if (interpreterProxy->failed()) {
		return null;
	}
	if (!((interpreterProxy->isWords(arrayOop)) && ((interpreterProxy->slotSizeOf(arrayOop)) == DCTSize2))) {
		return interpreterProxy->primitiveFail();
	}
	qt = interpreterProxy->firstIndexableField(arrayOop);
	arrayOop = interpreterProxy->stackObjectValue(1);
	if (interpreterProxy->failed()) {
		return null;
	}
	if (!((interpreterProxy->isWords(arrayOop)) && ((interpreterProxy->slotSizeOf(arrayOop)) == DCTSize2))) {
		return interpreterProxy->primitiveFail();
	}
	anArray = interpreterProxy->firstIndexableField(arrayOop);
	idctBlockIntqt(anArray, qt);
	interpreterProxy->pop(2);
}

static int scaleAndSignExtendinFieldWidth(int aNumber, int w) {
	if (aNumber < ((((w - 1) < 0) ? ((unsigned) 1 >> -(w - 1)) : ((unsigned) 1 << (w - 1))))) {
		return (aNumber - (((w < 0) ? ((unsigned) 1 >> -w) : ((unsigned) 1 << w)))) + 1;
	} else {
		return aNumber;
	}
}


/*	Note: This is coded so that is can be run from Squeak. */

EXPORT(int) setInterpreter(struct VirtualMachine* anInterpreter) {
    int ok;

	interpreterProxy = anInterpreter;
	ok = interpreterProxy->majorVersion() == VM_PROXY_MAJOR;
	if (ok == 0) {
		return 0;
	}
	ok = interpreterProxy->minorVersion() >= VM_PROXY_MINOR;
	return ok;
}

static int stInit(void) {
	;
}

static int storeJPEGStreamOn(int streamOop) {
	interpreterProxy->storeIntegerofObjectwithValue(1, streamOop, jsPosition);
	interpreterProxy->storeIntegerofObjectwithValue(3, streamOop, jsBitBuffer);
	interpreterProxy->storeIntegerofObjectwithValue(4, streamOop, jsBitCount);
}

static int yColorComponentFrom(int oop) {
	return (colorComponentfrom(yComponent, oop)) && (colorComponentBlocksfrom(yBlocks, oop));
}


#ifdef SQUEAK_BUILTIN_PLUGIN


void* JPEGReaderPlugin_exports[][3] = {
	{"JPEGReaderPlugin", "setInterpreter", (void*)setInterpreter},
	{"JPEGReaderPlugin", "primitiveIdctInt", (void*)primitiveIdctInt},
	{"JPEGReaderPlugin", "primitiveColorConvertMCU", (void*)primitiveColorConvertMCU},
	{"JPEGReaderPlugin", "getModuleName", (void*)getModuleName},
	{"JPEGReaderPlugin", "primitiveColorConvertGrayscaleMCU", (void*)primitiveColorConvertGrayscaleMCU},
	{"JPEGReaderPlugin", "primitiveDecodeMCU", (void*)primitiveDecodeMCU},
	{NULL, NULL, NULL}
};


#endif /* ifdef SQ_BUILTIN_PLUGIN */


/*************************************************************************
 * $Id: CodecTest.cxx,v 1.5 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created:  11/27/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#include "Hex.hxx"
#include "Base64.hxx"

#define VERIFY_STRING(X, Y)                             \
    if(strcmp(X, Y) != 0) {                             \
        cout << endl;                                   \
        cout << "Failed for \"" << X << "\"" << endl;   \
        cout << "expecting  \"" << Y << "\"" << endl;   \
        return;                                         \
    }

#define VERIFY_MEMAREA(X, Y, LEN)               \
    if(memcmp(X, Y, LEN) != 0) {                \
        cout << "failed" << endl;               \
        return;                                 \
    }

__USING_NAMESPACE(SELFSOFT);

static const char HEX_CODE1[] =
                            "292e43c9a2d87c01"
                            "3d3654a1ecf00613"
                            "62a705f3c0c7738c"
                            "98932bd9bc4c82ca";

static const char HEX_CODE2[] =
                            "1e9b573cfdd4e016"
                            "67426f188a17e512"
                            "be4ec4d6da9ede49"
                            "a0fbf58ebb2fee7a";

static const char B64_CODE1[] = "KS5DyaLYfAE9NlSh7PAGE2KnBfPAx3OMmJMr2bxMgso=";
static const char B64_CODE2[] = "HptXPP3U4BZnQm8YihflEr5OxNbant5JoPv1jrsv7no=";

static const byte BIN_CODE1[] = {
    0x29, 0x2e, 0x43, 0xc9, 0xa2, 0xd8, 0x7c, 0x01,
    0x3d, 0x36, 0x54, 0xa1, 0xec, 0xf0, 0x06, 0x13,
    0x62, 0xa7, 0x05, 0xf3, 0xc0, 0xc7, 0x73, 0x8c,
    0x98, 0x93, 0x2b, 0xd9, 0xbc, 0x4c, 0x82, 0xca
};

static const byte BIN_CODE2[] = {
    0x1e, 0x9b, 0x57, 0x3c, 0xfd, 0xd4, 0xe0, 0x16,
    0x67, 0x42, 0x6f, 0x18, 0x8a, 0x17, 0xe5, 0x12,
    0xbe, 0x4e, 0xc4, 0xd6, 0xda, 0x9e, 0xde, 0x49,
    0xa0, 0xfb, 0xf5, 0x8e, 0xbb, 0x2f, 0xee, 0x7a
};

void doHexTest() {
    cout << "Hex encoder/decoder test ... " << flush;
    HexEncoder hexEnc;
    HexDecoder hexDec;

    // Test HexEncoder
    hexEnc.update(BIN_CODE1, sizeof(BIN_CODE1));
    VERIFY_STRING(hexEnc.getEncoded(), HEX_CODE1);
    hexEnc.reset();
    hexEnc.update(BIN_CODE2, sizeof(BIN_CODE2));
    VERIFY_STRING(hexEnc.getEncoded(), HEX_CODE2);
    hexEnc.reset();

    // Test HexDecoder
    hexDec.update(HEX_CODE1, strlen(HEX_CODE1));
    VERIFY_MEMAREA(hexDec.getDecoded(), BIN_CODE1, sizeof(BIN_CODE1));
    hexDec.reset();
    hexDec.update(HEX_CODE2, strlen(HEX_CODE2));
    VERIFY_MEMAREA(hexDec.getDecoded(), BIN_CODE2, sizeof(BIN_CODE2));
    hexDec.reset();
    cout << "passed" << endl;
}

void doBase64Test() {
    cout << "Base64 encoder/decoder test ... " << flush;
    Base64Encoder b64Enc;
    Base64Decoder b64Dec;

    // Test Base64Encoder
    b64Enc.update(BIN_CODE1, sizeof(BIN_CODE1));
    VERIFY_STRING(b64Enc.getEncoded(), B64_CODE1);
    b64Enc.reset();
    b64Enc.update(BIN_CODE2, sizeof(BIN_CODE2));
    VERIFY_STRING(b64Enc.getEncoded(), B64_CODE2);
    b64Enc.reset();

    // Test Bas64Decoder
    b64Dec.update(B64_CODE1, strlen(B64_CODE1));
    VERIFY_MEMAREA(b64Dec.getDecoded(), BIN_CODE1, sizeof(BIN_CODE1));
    b64Dec.reset();
    b64Dec.update(B64_CODE2, strlen(B64_CODE2));
    VERIFY_MEMAREA(b64Dec.getDecoded(), BIN_CODE2, sizeof(BIN_CODE2));
    b64Dec.reset();

    cout << "passed" << endl;
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

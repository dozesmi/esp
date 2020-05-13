// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "fp.hpp"
#include <sstream>

// Optional application-specific helper functions
//Returns the fraction bits of the double-precision floating-point value `a'.
inline sc_dt::sc_uint<52>
extractFloat64Frac (sc_dt::sc_uint<64> a) {
  return a.range(51,0);
}

//Returns the exponent bits of the double-precision floating-point value `a'.
inline sc_dt::sc_uint<11>
extractFloat64Exp (sc_dt::sc_uint<64> a) {
  return a.range(62,52);
}


//Returns the sign bit of the double-precision floating-point value `a'.
inline bool
extractFloat64Sign (sc_dt::sc_uint<64> a) {
  return (bool) a[63];
}

inline sc_dt::sc_uint<64>
packFloat64 (bool zSign, sc_dt::sc_uint<11> zExp, sc_dt::sc_uint<52> zFrac)
{
  sc_dt::sc_uint<64> z;
  z[63] = zSign;
  z.range(62,52) = zExp;
  z.range(51,0) = zFrac;
  return z;
}

inline sc_dt::sc_uint<7>
getLeadingZeroes (sc_dt::sc_uint<64> z)
{
    /*sc_dt::sc_uint<64> y;
    sc_dt::sc_uint<7> n = 64;
    y = z >> 32; if(y != 0) {n = n - 32; z = y;}
    y = z >> 16; if(y != 0) {n = n - 16; z = y;}
    y = z >> 8; if(y != 0) {n = n - 8; z = y;}
    y = z >> 4; if(y != 0) {n = n - 4; z = y;}
    y = z >> 2; if(y != 0) {n = n - 2; z = y;}
    y = z >> 1; if(y != 0) return n - 2;
    return n - z.range(6,0);*/
    if(z[63] == 1) return 0;
    else if(z[62] == 1) return 0;
    else if(z[61] == 1) return 1;
    else if(z[60] == 1) return 2;
    else if(z[59] == 1) return 3;
    else if(z[58] == 1) return 4;
    else if(z[57] == 1) return 5;
    else if(z[56] == 1) return 6;
    else if(z[55] == 1) return 7;
    else if(z[54] == 1) return 8;
    else if(z[53] == 1) return 9;
    else if(z[52] == 1) return 10;
    else if(z[51] == 1) return 11;
    else if(z[50] == 1) return 12;
    else if(z[49] == 1) return 13;
    else if(z[48] == 1) return 14;
    else if(z[47] == 1) return 15;
    else if(z[46] == 1) return 16;
    else if(z[45] == 1) return 17;
    else if(z[44] == 1) return 18;
    else if(z[43] == 1) return 19;
    else if(z[42] == 1) return 20;
    else if(z[41] == 1) return 21;
    else if(z[40] == 1) return 22;
    else if(z[39] == 1) return 23;
    else if(z[38] == 1) return 24;
    else if(z[37] == 1) return 25;
    else if(z[36] == 1) return 26;
    else if(z[35] == 1) return 27;
    else if(z[34] == 1) return 28;
    else if(z[33] == 1) return 29;
    else if(z[32] == 1) return 30;
    else if(z[31] == 1) return 31;
    else if(z[30] == 1) return 32;
    else if(z[29] == 1) return 33;
    else if(z[28] == 1) return 34;
    else if(z[27] == 1) return 35;
    else if(z[26] == 1) return 36;
    else if(z[25] == 1) return 37;
    else if(z[24] == 1) return 38;
    else if(z[23] == 1) return 39;
    else if(z[22] == 1) return 40;
    else if(z[21] == 1) return 41;
    else if(z[20] == 1) return 42;
    else if(z[19] == 1) return 43;
    else if(z[18] == 1) return 44;
    else if(z[17] == 1) return 45;
    else if(z[16] == 1) return 46;
    else if(z[15] == 1) return 47;
    else if(z[14] == 1) return 48;
    else if(z[13] == 1) return 49;
    else if(z[12] == 1) return 50;
    else if(z[11] == 1) return 51;
    else if(z[10] == 1) return 52;
    else if(z[9] == 1) return 53;
    else if(z[8] == 1) return 54;
    else if(z[7] == 1) return 55;
    else if(z[6] == 1) return 56;
    else if(z[5] == 1) return 57;
    else if(z[4] == 1) return 58;
    else if(z[3] == 1) return 59;
    else if(z[2] == 1) return 60;
    else if(z[1] == 1) return 61;
    else if(z[0] == 1) return 62;
    else return 63;
}

static uint64_t
addFloat64Sigs (sc_dt::sc_uint<64> a, sc_dt::sc_uint<64> b, bool zSign)
{
    bool aSign;
    sc_dt::sc_uint<52> aFrac, bFrac, zFrac;
    sc_dt::sc_uint<11> aExp, bExp, zExp;
    sc_dt::sc_int<12> expDiff;
    sc_dt::sc_uint<64> aTempFrac, bTempFrac, zTempFrac;

    {
    HLS_EXTLAT_CONSTRAIN;
    aSign = extractFloat64Sign (a);
    aFrac = extractFloat64Frac (a);
    aExp = extractFloat64Exp (a);
    bFrac = extractFloat64Frac (b);
    bExp = extractFloat64Exp (b);

    expDiff = aExp - bExp;

    aTempFrac.range(61,10) = aFrac;
    bTempFrac.range(61,10) = bFrac;
    aTempFrac[62] = 1;
    bTempFrac[62] = 1;
    

    //Handles zero representation
    if(aFrac == 0 && aExp == 0)
        return (uint64_t) b;
    if(bFrac == 0 && bExp == 0)
        return (uint64_t) a;
    

    //aExp is bigger
    if (0 < expDiff) {
        zExp = aExp;
        bTempFrac = bTempFrac >> expDiff;
    //bExp is bigger    
    } else if (expDiff < 0) {
        zExp = bExp;
        aTempFrac = aTempFrac >> -expDiff;
    //exp are the same    
    } else {
        zExp = aExp;
    }

    

    zTempFrac = aTempFrac + bTempFrac;
    //cout << "aTempFrac2: " << aTempFrac << endl;
    //cout << hex << zTempFrac << endl;
    if(zTempFrac[63] == 1) {
        zExp += 1;
        zFrac = zTempFrac.range(62,11);
    } else {
        zFrac = zTempFrac.range(61,10);
    }

    //cout << hex << packFloat64(aSign, zExp, zFrac) << endl;
    return packFloat64(aSign, zExp, zFrac);
    }
}

static uint64_t
subFloat64Sigs (sc_dt::sc_uint<64> a, sc_dt::sc_uint<64> b, bool zSign)
{
    bool aSign, bSign;
    sc_dt::sc_uint<52> aFrac, bFrac, zFrac;
    sc_dt::sc_uint<11> aExp, bExp, zExp;
    sc_dt::sc_int<12> expDiff;
    sc_dt::sc_uint<64> aTempFrac, bTempFrac, zTempFrac;

    {
    HLS_ZERO_CONSTRAIN;
    aSign = extractFloat64Sign (a);
    aFrac = extractFloat64Frac (a);
    aExp = extractFloat64Exp (a);
    bSign = extractFloat64Sign (b);
    bFrac = extractFloat64Frac (b);
    bExp = extractFloat64Exp (b);

    expDiff = aExp - bExp;

    aTempFrac.range(61,10) = aFrac;
    bTempFrac.range(61,10) = bFrac;
    aTempFrac[62] = 1;
    bTempFrac[62] = 1;
    

    //Handles zero representation
    if(aFrac == 0 && aExp == 0)
        return (uint64_t) b;
    if(bFrac == 0 && bExp == 0)
        return (uint64_t) a;
    if(bExp == aExp && bFrac == aFrac)
        return 0;
    
    //aExp is bigger
    if (0 < expDiff) {
        zSign = aSign;
        zExp = aExp;
        bTempFrac = bTempFrac >> expDiff;
    //bExp is bigger    
    } else if (expDiff < 0) {
        zSign = bSign;
        zExp = bExp;
        aTempFrac = aTempFrac >> -expDiff;
    //exp are the same    
    } else {
        zExp = aExp;
        if(aFrac > bFrac)
            zSign = aSign;
        else
            zSign = bSign;
    }

    if(aTempFrac > bTempFrac)
        zTempFrac = aTempFrac - bTempFrac;
    else
        zTempFrac = bTempFrac - aTempFrac;

    sc_dt::sc_uint<7> zeros = getLeadingZeroes(zTempFrac);
    zTempFrac = zTempFrac << zeros;
    zExp -= zeros;
    
    zFrac = zTempFrac.range(61,10);
    return packFloat64(zSign, zExp, zFrac);
    }
}

uint64_t
float64_add (uint64_t a, uint64_t b)
{
  bool aSign, bSign;
  
  aSign = extractFloat64Sign (a);
  bSign = extractFloat64Sign (b);
  
  //cout << "aSign: " << aSign << " bSign: " << bSign << endl;
  //cout << "aExp: " << aExp << " bExp: " << bExp << endl;
  //cout << "aFrac: " << aFrac << " bFrac: " << bFrac << endl;
  //cout << "a: " << zA << " b: " << zB << endl;

  if (aSign == bSign)
    return addFloat64Sigs (a, b, aSign);
  else
    return subFloat64Sigs (a, b, aSign);
}

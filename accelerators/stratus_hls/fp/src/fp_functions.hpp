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

static uint64_t
addFloat64Sigs (sc_dt::sc_uint<64> a, sc_dt::sc_uint<64> b, bool zSign)
{
    bool aSign;
    sc_dt::sc_uint<52> aFrac, bFrac, zFrac;
    sc_dt::sc_uint<11> aExp, bExp, zExp;
    sc_dt::sc_int<12> expDiff;
    sc_dt::sc_uint<64> aTempFrac, bTempFrac, zTempFrac;

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

static uint64_t
subFloat64Sigs (sc_dt::sc_uint<64> a, sc_dt::sc_uint<64> b, bool zSign)
{
    bool aSign, bSign;
    sc_dt::sc_uint<52> aFrac, bFrac, zFrac;
    sc_dt::sc_uint<11> aExp, bExp, zExp;
    sc_dt::sc_int<12> expDiff;
    sc_dt::sc_uint<64> aTempFrac, bTempFrac, zTempFrac;

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
    
    while(zTempFrac[62] != 1) {
        zTempFrac = zTempFrac << 1;
        zExp -= 1;
    }
    zFrac = zTempFrac.range(61,10);

    return packFloat64(zSign, zExp, zFrac);
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

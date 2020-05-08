// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __FP_CONF_INFO_HPP__
#define __FP_CONF_INFO_HPP__

#include <systemc.h>

//
// Configuration parameters for the accelerator.
//
class conf_info_t
{
public:

    //
    // constructors
    //
    conf_info_t()
    {
        /* <<--ctor-->> */
        this->fp_len = 64;
        this->fp_n = 1;
        this->fp_vec = 100;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t fp_len, 
        int32_t fp_n, 
        int32_t fp_vec
        )
    {
        /* <<--ctor-custom-->> */
        this->fp_len = fp_len;
        this->fp_n = fp_n;
        this->fp_vec = fp_vec;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (fp_len != rhs.fp_len) return false;
        if (fp_n != rhs.fp_n) return false;
        if (fp_vec != rhs.fp_vec) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        fp_len = other.fp_len;
        fp_n = other.fp_n;
        fp_vec = other.fp_vec;
        return *this;
    }

    // VCD dumping function
    friend void sc_trace(sc_trace_file *tf, const conf_info_t &v, const std::string &NAME)
    {}

    // redirection operator
    friend ostream& operator << (ostream& os, conf_info_t const &conf_info)
    {
        os << "{";
        /* <<--print-->> */
        os << "fp_len = " << conf_info.fp_len << ", ";
        os << "fp_n = " << conf_info.fp_n << ", ";
        os << "fp_vec = " << conf_info.fp_vec << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t fp_len;
        int32_t fp_n;
        int32_t fp_vec;
};

#endif // __FP_CONF_INFO_HPP__

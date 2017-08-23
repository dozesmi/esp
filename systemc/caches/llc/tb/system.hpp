/* Copyright 2017 Columbia University, SLD Group */

#ifndef __SYSTEM_HPP__
#define __SYSTEM_HPP__


#include "llc.hpp"
#include "llc_wrap.h"
#include "llc_tb.hpp"


class system_t : public sc_module
{

public:

    // Clock signal
    sc_in<bool> clk;

    // Reset signal
    sc_in<bool> rst;

    // Signals
    sc_signal< sc_bv<LLC_ASSERT_WIDTH> > asserts;
    sc_signal< sc_bv<LLC_BOOKMARK_WIDTH> > bookmark;
    sc_signal<uint32_t> custom_dbg;

    // Channels
    // To LLC cache
    put_get_channel<llc_req_in_t>  llc_req_in_chnl;
    put_get_channel<llc_rsp_in_t>  llc_rsp_in_chnl;
    put_get_channel<llc_mem_rsp_t> llc_mem_rsp_chnl;
    // From LLC cache
    put_get_channel<llc_rsp_out_t> llc_rsp_out_chnl;
    put_get_channel<llc_fwd_out_t> llc_fwd_out_chnl;
    put_get_channel<llc_mem_req_t> llc_mem_req_chnl;

    // Modules
    // LLC cache instance
    llc_wrapper	*dut;
    // LLC testbench module
    llc_tb        	*tb;

    // Constructor
    SC_CTOR(system_t)
    {
	// Modules
	dut = new llc_wrapper("llc_wrapper");
	tb  = new llc_tb("llc_tb");

	// Binding LLC cache
	dut->clk(clk);
	dut->rst(rst);
        dut->asserts(asserts);
        dut->bookmark(bookmark);
        dut->custom_dbg(custom_dbg);
	dut->llc_req_in(llc_req_in_chnl);
	dut->llc_rsp_in(llc_rsp_in_chnl);
	dut->llc_mem_rsp(llc_mem_rsp_chnl);
	dut->llc_rsp_out(llc_rsp_out_chnl);
	dut->llc_fwd_out(llc_fwd_out_chnl);
	dut->llc_mem_req(llc_mem_req_chnl);

	// Binding testbench
	tb->clk(clk);
	tb->rst(rst);
        tb->asserts(asserts);
        tb->bookmark(bookmark);
        tb->custom_dbg(custom_dbg);
	tb->llc_req_in_tb(llc_req_in_chnl);
	tb->llc_rsp_in_tb(llc_rsp_in_chnl);
	tb->llc_mem_rsp_tb(llc_mem_rsp_chnl); 
	tb->llc_rsp_out_tb(llc_rsp_out_chnl);
	tb->llc_fwd_out_tb(llc_fwd_out_chnl);
	tb->llc_mem_req_tb(llc_mem_req_chnl);
    }
};

#endif // __SYSTEM_HPP__

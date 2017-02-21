/* Copyright 2017 Columbia University, SLD Group */

#include "sort.hpp"
#include "sort_directives.hpp"

// Functions

#include "sort_functions.cpp"

// Processes

void sort::load_input()
{
	bool ping;
	unsigned len; // from conf_info.len
	unsigned bursts; // from conf_info.batch
	unsigned index;

	// Reset
	{
		HLS_DEFINE_PROTOCOL("load-reset");

		this->reset_load_input();

 		index = 0;
		len = 0;
		bursts = 0;

		ping = true;
		A0.port1.reset();
		A1.port1.reset();
#if (DMA_WIDTH == 64)
		A0.port2.reset();
		A1.port2.reset();
#endif
		wait();
	}

	// Config
	{
		HLS_DEFINE_PROTOCOL("load-config");

		cfg.wait_for_config(); // config process
		conf_info_t config = this->conf_info.read();
		len = config.len;
		bursts = config.batch;
	}

	// Load
	{
		for (uint16_t b = 0; b < bursts; b++)
		{
			HLS_LOAD_INPUT_BATCH_LOOP;

			{
				HLS_DEFINE_PROTOCOL("load-dma-conf");

				dma_info_t dma_info(index / (DMA_WIDTH / 32), len / (DMA_WIDTH / 32));
				index += len;
				this->dma_read_ctrl.put(dma_info);
			}
#if (DMA_WIDTH == 32)
			for (uint16_t i = 0; i < len; i++)
			{
				HLS_LOAD_INPUT_LOOP;

				uint32_t data = this->dma_read_chnl.get().to_uint();
				{
					HLS_LOAD_INPUT_PLM_WRITE;
					if (ping)
						A0.port1[0][i] = data;
					else
						A1.port1[0][i] = data;
				}
#elif (DMA_WIDTH == 64)
			for (uint16_t i = 0; i < len; i += 2)
			{
				HLS_LOAD_INPUT_LOOP;

				sc_dt::sc_bv<64> data_bv = this->dma_read_chnl.get();
				{
					HLS_LOAD_INPUT_PLM_WRITE;
					uint32_t data_1 = data_bv.range(31, 0).to_uint();
					uint32_t data_2 = data_bv.range(63, 32).to_uint();
					const uint16_t j = i + 1;
					if (ping) {
						A0.port1[0][i] = data_1;
						A0.port2[0][j] = data_2;
					} else {
						A1.port1[0][i] = data_1;
						A1.port2[0][j] = data_2;
					}
				}
#endif
			}
			ping = !ping;
			this->load_compute_handshake();
		}
	}

	// Conclude
	{
		this->process_done();
	}
}



void sort::store_output()
{
	bool ping;
	unsigned len; // from conf_info.len
	unsigned bursts; // from conf_info.batch
	unsigned index;

	// Reset
	{
		HLS_DEFINE_PROTOCOL("store-reset");

		this->reset_store_output();

 		index = 0;
		len = 0;
		bursts = 0;

		ping = true;
		B0.port2.reset();
		B1.port2.reset();
#if (DMA_WIDTH == 64)
		B0.port3.reset();
		B1.port3.reset();
#endif
		wait();
	}

	// Config
	{
		HLS_DEFINE_PROTOCOL("store-config");

		cfg.wait_for_config(); // config process
		conf_info_t config = this->conf_info.read();
		len = config.len;
		bursts = config.batch;
	}

	// Store
	{
		for (uint16_t b = 0; b < bursts; b++)
		{
			HLS_STORE_OUTPUT_BATCH_LOOP;

			this->store_compute_handshake();

			{
				HLS_DEFINE_PROTOCOL("store-dma-conf");

				dma_info_t dma_info(index / (DMA_WIDTH / 32), len / (DMA_WIDTH / 32));
				index += len;
				this->dma_write_ctrl.put(dma_info);
			}
#if (DMA_WIDTH == 32)
			for (uint16_t i = 0; i < len; i++)
			{
				HLS_STORE_OUTPUT_LOOP;

				uint32_t data;
				{
					HLS_STORE_OUTPUT_PLM_READ;
					if (ping)
						data = B0.port2[0][i];
					else
						data = B1.port2[0][i];
				}
				this->dma_write_chnl.put(data);
#elif (DMA_WIDTH == 64)
			for (uint16_t i = 0; i < len; i += 2)
			{
				HLS_STORE_OUTPUT_LOOP;

				sc_dt::sc_bv<64> data_bv;
				{
					HLS_STORE_OUTPUT_PLM_READ;
					const uint16_t j = i + 1;
					if (ping) {
						data_bv.range(31, 0)  = B0.port2[0][i];
						data_bv.range(63, 32) = B0.port3[0][j];
					} else {
						data_bv.range(31, 0)  = B1.port2[0][i];
						data_bv.range(63, 32) = B1.port3[0][j];
					}
				}
				this->dma_write_chnl.put(data_bv);
#endif
			}
			ping = !ping;
		}
	}

	// Conclude
	{
		this->accelerator_done();
		this->process_done();
	}
}


void sort::compute_kernel()
{
	// Bi-tonic sort
	bool ping;
	unsigned len; // from conf_info.len
	unsigned bursts; // from conf_info.batch

	// Reset
	{
		HLS_DEFINE_PROTOCOL("compute-reset");

		this->reset_compute_1_kernel();

		len = 0;
		bursts = 0;

		ping = true;
#if (DMA_WIDTH == 32)
		A0.port2.reset();
		A1.port2.reset();
#elif (DMA_WIDTH == 64)
		A0.port3.reset();
		A1.port3.reset();
#endif
		C0.port1.reset();
		C1.port1.reset();

		wait();
	}

	// Config
	{
		HLS_DEFINE_PROTOCOL("compute-config");

		cfg.wait_for_config(); // config process
		conf_info_t config = this->conf_info.read();
		len = config.len;
		bursts = config.batch;
	}


	// Compute
	{
	for (uint16_t b = 0; b < bursts; b++)
	{
		HLS_RB_SORT_LOOP;

		this->compute_load_handshake();

		// Flatten array
		unsigned regs[2][NUM];
		HLS_RB_MAP_REGS;

		bool pipe_full = false;
		unsigned wchunk = 0;

		// Break the following
		for (int chunk = 0; chunk < LEN / NUM; )
		{
			HLS_RB_MAIN;

			if ((unsigned) chunk * NUM == len)
				break;

			// Unroll the following
			for (int idx = 0; idx < 2; idx++, chunk++)
			{
				HLS_RB_CHAIN_SELECT;

				if ((unsigned) chunk * NUM == len)
					break;

				//Break the following
				for (int i = 0; i < NUM; i++)
				{
					HLS_RB_RW_CHUNK;

					unsigned elem;
					if (ping)
					{
#if (DMA_WIDTH == 32)
						elem = A0.port2[0][chunk * NUM + i];
#elif (DMA_WIDTH == 64)
						elem = A0.port3[0][chunk * NUM + i];
#endif
						if (pipe_full)
							C0.port1[0][wchunk * NUM + i] = regs[idx][i];
					}
					else
					{
#if (DMA_WIDTH == 32)
						elem = A1.port2[0][chunk * NUM + i];
#elif (DMA_WIDTH == 64)
						elem = A1.port3[0][chunk * NUM + i];
#endif
						if (pipe_full)
							C1.port1[0][wchunk * NUM + i] = regs[idx][i];
					}
					regs[idx][i] = elem;
				}
				if (pipe_full)
					wchunk++;

				//Break the following
				for (int k = 0; k < NUM; k++)
				{
					HLS_RB_INSERTION_OUTER;
					//Unroll the following
					for (int i = 0; i < NUM; i += 2)
					{
						HLS_RB_INSERTION_EVEN;

						if (!lt_float(regs[idx][i], regs[idx][i + 1]))
						{
							unsigned tmp   = regs[idx][i];
							regs[idx][i]   = regs[idx][i + 1];
							regs[idx][i + 1] = tmp;
						}
					}

					//Unroll the following
					for (int i = 1; i < NUM - 1; i += 2)
					{
						HLS_RB_INSERTION_ODD;
						if (!lt_float(regs[idx][i], regs[idx][i + 1]))
						{
							unsigned tmp   = regs[idx][i];
							regs[idx][i]   = regs[idx][i + 1];
							regs[idx][i + 1] = tmp;
						}
					}
				}
			}
			pipe_full = true;
		}

		// for (int i = 0; i < NUM; i++)
		//    cout << i << ": " << std::hex << regs[0][i] << std::dec << endl;
		// cout << pipe_full << " " << wchunk << endl;

		//Break the following
		for (int idx = 0; idx < 2; idx++)
		{
			HLS_RB_W_LAST_CHUNKS_OUTER;
			if ((unsigned) idx * NUM == len)
				break;

			for (int i = 0; i < NUM; i++)
			{
				HLS_RB_W_LAST_CHUNKS_INNER;
				if (ping)
					C0.port1[0][wchunk * NUM + i] = regs[idx][i];
				else
					C1.port1[0][wchunk * NUM + i] = regs[idx][i];
			}
			wchunk++;
		}

		ping = !ping;

		this->compute_compute_2_handshake();
	}

	// Conclude
	{
		this->process_done();
	}
	}
}


void sort::compute_2_kernel()
{
	// Bi-tonic sort
	bool ping;
	unsigned len; // from conf_info.len
	unsigned bursts; // from conf_info.batch

	// Reset
	{
		HLS_DEFINE_PROTOCOL("compute-2-reset");

		this->reset_compute_2_kernel();

		len = 0;
		bursts = 0;

		ping = true;
		C0.port2.reset();
		C1.port2.reset();
		B0.port1.reset();
		B1.port1.reset();

		wait();
	}

	// Config
	{
		HLS_DEFINE_PROTOCOL("compute-2-config");

		cfg.wait_for_config(); // config process
		conf_info_t config = this->conf_info.read();
		len = config.len;
		bursts = config.batch;
	}


	// Compute
	const unsigned chunk_max = (len >> lgNUM);
	{
	for (uint16_t b = 0; b < bursts; b++)
	{
		HLS_MERGE_SORT_LOOP;
		unsigned head[LEN / NUM];  // Fifo output
		unsigned fidx[LEN / NUM];  // Fifo index
		bool     pop[LEN / NUM];   // pop from ith fifo
		bool     shift[LEN / NUM];   // shift from ith fifo
		unsigned regs[LEN / NUM];  // State
		unsigned regs_in[LEN / NUM]; // Next state
		HLS_MERGE_SORT_MAP_REGS;

		//Should not be a combinational loop. BTW unroll.
		for (int i = 0; i < LEN / NUM; i++)
		{
			HLS_MERGE_INIT_ZERO_FIDX;

			fidx[i] = 0;
			pop[i] = false;
			shift[i] = false;
		}

		this->compute_2_compute_handshake();

		if (chunk_max > 1)  //MERGE is needed
		{
			// // DEBUG
			// for (int chunk = 0; chunk < LEN/NUM; chunk++) {
			//    if (chunk*NUM == len)
			//       break;
			//    cout << "CHUNK: " << chunk << endl;
			//    for (int i = 0; i < NUM; i++) {
			//       if (ping)
			//          cout << std::hex << C0[chunk][i] << " ";
			//       else
			//          cout << std::hex << C1[chunk][i] << " ";
			//    }
			//    cout << std::dec << endl << endl;
			// }

			//Break the following
			for (int chunk = 0; chunk < LEN / NUM; chunk++)
			{
				HLS_MERGE_RD_FIRST_ELEMENTS;

				if ((unsigned) chunk == chunk_max)
					break;
				if (ping)
					head[chunk] = C0.port2[0][chunk * NUM];
				else
					head[chunk] = C1.port2[0][chunk * NUM];

				fidx[chunk]++;
			}

			regs[0] = head[0];
			if (ping)
				head[0] = C0.port2[0][1];
			else
				head[0] = C1.port2[0][1];
			fidx[0]++;

			// // DEBUG
			// cout << "INIT HEADS" << endl;
			// for (int chunk = 0; chunk < LEN/NUM; chunk++) {
			//    if (chunk == chunk_max)
			//       break;
			//    cout << chunk << ": " << std::hex << head[chunk] << "; ";
			// }
			// cout << std::dec << endl << endl;

			unsigned cur = 2;
			unsigned cnt = 0;
			//Break the following
			while(true)
			{
				HLS_MERGE_MAIN;
				//Unroll the following
				for (int chunk = 1; chunk < LEN / NUM; chunk++)
				{
					HLS_MERGE_COMPARE;

					if ((unsigned) chunk == cur)
						break;
					if (lt_float(regs[chunk - 1], head[chunk]))
						shift[chunk] = true;
					else
						shift[chunk] = false;
				}

				regs_in[0] = regs[0];
				//Unroll the following
				for (int chunk = LEN / NUM - 1; chunk > 0; chunk--)
				{
					HLS_MERGE_SHIFT;

					if ((unsigned) chunk >= cur)
						continue;

					if (shift[chunk])
					{
						regs_in[chunk] = regs[chunk - 1];
						if (chunk == 1)
						{
							regs_in[0] = head[0];
							pop[0] = true;
						}
					}
					else
					{
						regs_in[chunk] = head[chunk];
						pop[chunk] = true;
						break;
					}
				}

				// // DEBUG:
				// cout << "POP from...";
				// for (int chunk = 0; chunk < LEN/NUM; chunk++) {
				//    if (chunk == chunk_max)
				//       break;
				//    cout << " " << pop[chunk];
				// }
				// cout << endl;

				//Unroll the following
				for (int chunk = 0; chunk < LEN / NUM; chunk++)
				{
					HLS_MERGE_SEQ;

					if ((unsigned) chunk == cur)
						break;
					regs[chunk] = regs_in[chunk];
				}

				// // DEBUG
				// cout << "regs after shift: ";
				// for (int chunk = 0; chunk < LEN/NUM; chunk++) {
				//    if (chunk == chunk_max)
				//       break;
				//    cout << chunk << ": " << std::hex << regs[chunk] << "; ";
				// }
				// cout << endl;
				// cout << "heads after shift: ";
				// for (int chunk = 0; chunk < LEN/NUM; chunk++) {
				//    if (chunk == chunk_max)
				//       break;
				//    cout << chunk << ": " << std::hex << head[chunk] << "; ";
				// }
				// cout << std::dec << endl;

				if (cur == chunk_max)
				{
					HLS_MERGE_WR_LAST_ELEMENTS;
					// write output
					if (ping)
						B0.port1[0][cnt] = regs[chunk_max - 1];
					else
						B1.port1[0][cnt] = regs[chunk_max - 1];
					cnt++;
				}

				int pop_idx = -1;
				//Unroll the following
				//Notice that only one pop[i] will be true at any time
				for (int chunk = 0; chunk < LEN / NUM; chunk++)
				{
					HLS_MERGE_SET_POP;

					if ((unsigned) chunk == cur)
						break;
					if (pop[chunk])
					{
						if (fidx[chunk] < NUM)
						{
							pop_idx = chunk;
						}
						else
						{
							head[chunk] = 0x7f800000; // +INF
						}
						pop[chunk] = false;
						break;
					}
				}

				if (pop_idx != -1)
				{
					HLS_MERGE_DO_POP;
					if (ping)
						head[pop_idx] = C0.port2[0][pop_idx * NUM + fidx[pop_idx]];
					else
						head[pop_idx] = C1.port2[0][pop_idx * NUM + fidx[pop_idx]];
					fidx[pop_idx]++;
				}

				for (int i = 0; i < LEN / NUM; i++)
				{
					HLS_MERGE_RESTORE_ZERO_POP;

					pop[i] = false;
					shift[i] = false;
				}

				// DEBUG
				// cout << "heads after pop: ";
				// for (int chunk = 0; chunk < LEN/NUM; chunk++) {
				//    if (chunk == chunk_max)
				//       break;
				//    cout << chunk << ": " << std::hex << head[chunk] << "; ";
				// }
				// cout << std::dec << endl << endl;

				if (cur < chunk_max)
					cur++;

				if (cnt == len)
					break;
			}
		}
		else     // MERGE is not required
		{
			for (int chunk = 0; chunk < LEN / NUM; chunk++)
			{
				if ((unsigned) chunk == chunk_max)
					break;

				for (int i = 0; i < NUM; i++)
				{
					HLS_MERGE_NO_MERGE;

					unsigned elem;
					if (ping)
						elem = C0.port2[0][chunk * NUM + i];
					else
						elem = C1.port2[0][chunk * NUM + i];

					if (ping)
						B0.port1[0][i] = elem;
					else
						B1.port1[0][i] = elem;
				}
			}
		}
		ping = !ping;

		this->compute_store_handshake();
	}

	// Conclude
	{
		this->process_done();
	}
	}
}
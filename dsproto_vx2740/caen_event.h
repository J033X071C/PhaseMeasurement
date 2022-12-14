#ifndef CAEN_EVENT_H
#define CAEN_EVENT_H

#include "midas.h"
#include <inttypes.h>
#include <stdlib.h>

// Helper struct for parsing event header information.
struct CaenEventHeader {
   uint8_t format;
   uint32_t event_counter;
   uint32_t size_64bit_words;// includes size of 24 byte header
   uint16_t flags;
   uint8_t overlap;
   uint64_t trigger_time;
   uint64_t ch_enable_mask;

   CaenEventHeader(uint64_t *buffer = NULL, bool is_host_order=true);

   uint32_t size_bytes();
   uint32_t samples_per_chan();

   // Encode header in host-order byte format
   void hencode(uint64_t* buffer);
};

// Helper struct for parsing event data.
struct CaenEvent {
   CaenEvent(uint64_t *buffer, bool is_host_order=true);

   uint32_t get_channel_samples(int channel, uint16_t *chan_buffer, uint32_t chan_buf_size_samples);
   std::vector<uint16_t> get_channel_samples_vec(int channel);
   std::vector<uint64_t> get_channel_words_vec(int channel);

   // Encode event in host-order byte format
   void hencode(uint64_t* buffer);

   CaenEventHeader header;
   uint64_t *wf_begin;
   uint64_t *wf_end;
};

#endif
#include "caen_event.h"
#include <arpa/inet.h>

CaenEventHeader::CaenEventHeader(uint64_t *buffer, bool is_host_order) {
   // Decode data directly from device, in network-order byte format

   if (buffer) {
      if (is_host_order) {
         format = buffer[0] >> 56;
         event_counter = (buffer[0] >> 32) & 0xFFFFFF;
         size_64bit_words = buffer[0] & 0xFFFFFFFF;

         flags = (buffer[1] >> 52) & 0xFFF;
         overlap = (buffer[1] >> 48) & 0xF;
         trigger_time = buffer[1] & 0xFFFFFFFFFFFF;

         ch_enable_mask = buffer[2];
      } else {
         DWORD* buf32 = (DWORD*)buffer;
         format = buf32[0] & 0xFF;
         event_counter = htonl(buf32[0] & ~0xFF);
         size_64bit_words = htonl(buf32[1]);

         flags = htonl((buf32[2] & 0xFFF) << 20);
         overlap = (buf32[2] >> 16) & 0xF;
         trigger_time = ((uint64_t) htonl(buf32[2] & ~0xFFFF) << 32) | htonl(buf32[3]);

         ch_enable_mask = ((uint64_t) htonl(buf32[4]) << 32) | htonl(buf32[5]);
      }
   }
}

// Encode in host-ordering
void CaenEventHeader::hencode(uint64_t* buffer) {
   buffer[0] = ((uint64_t)(format & 0xFF) << 56) | ((uint64_t)(event_counter & 0xFFFFFF) << 32) | (size_64bit_words);
   buffer[1] = ((uint64_t)(flags & 0xFFF) << 52) | ((uint64_t)(overlap & 0xF) << 48) | (trigger_time & 0xFFFFFFFFFFFF);
   buffer[2] = ch_enable_mask;
}

uint32_t CaenEventHeader::size_bytes() {
   return size_64bit_words * sizeof(uint64_t);
}

uint32_t CaenEventHeader::samples_per_chan() {
   int num_chans = 0;

   for (int i = 0; i < 64; i++) {
      if (ch_enable_mask & ((uint64_t)1 << i)) {
         num_chans++;
      }
   }

   int total_num_samples = (size_64bit_words - 3) * 4;

   if (num_chans) {
      return total_num_samples / num_chans;
   } else {
      return 0;
   }
}

CaenEvent::CaenEvent(uint64_t *buffer, bool is_host_order) {
   header = CaenEventHeader(buffer, is_host_order);
   wf_begin = buffer + 3;
   wf_end = buffer + (header.size_bytes() / sizeof(uint64_t));
}

uint32_t CaenEvent::get_channel_samples(int channel, uint16_t *chan_buffer, uint32_t chan_buf_size_samples) {
   if (!(header.ch_enable_mask & (((uint64_t)1) << channel))) {
      // Channel wasn't enabled.
      return 0;
   }

   // Data format is 4 samples from channel 1; 4 samples from channel 2; ...
   uint64_t *p = wf_begin;
   DWORD i = 0;

   while (p < wf_end) {
      for (int j = 0; j < channel; j++) {
         if (header.ch_enable_mask & (((uint64_t)1) << j)) {
            p++; //
         }
      }

      uint64_t samp_dcba = *p++;

      if (i == chan_buf_size_samples) {
         break;
      }

      chan_buffer[i++] = (samp_dcba & 0xFFFF);

      if (i == chan_buf_size_samples) {
         break;
      }

      chan_buffer[i++] = (samp_dcba >> 16) & 0xFFFF;

      if (i == chan_buf_size_samples) {
         break;
      }

      chan_buffer[i++] = (samp_dcba >> 32) & 0xFFFF;

      if (i == chan_buf_size_samples) {
         break;
      }

      chan_buffer[i++] = (samp_dcba >> 48);

      for (int j = channel + 1; j < 64; j++) {
         if (header.ch_enable_mask & (((uint64_t)1) << j)) {
            p++; //
         }
      }
   }

   return i;
}

std::vector<uint16_t> CaenEvent::get_channel_samples_vec(int channel) {
   std::vector<uint64_t> words = get_channel_words_vec(channel);
   std::vector<uint16_t> retval(words.size() * 4);

   for (uint32_t i = 0; i < words.size(); i++) {
      retval[i*4 + 0] = words[i] & 0xFFFF;
      retval[i*4 + 1] = (words[i] >> 16) & 0xFFFF;
      retval[i*4 + 2] = (words[i] >> 32) & 0xFFFF;
      retval[i*4 + 3] = (words[i] >> 48) & 0xFFFF;
   }

   return retval;
}

std::vector<uint64_t> CaenEvent::get_channel_words_vec(int channel) {
   std::vector<uint64_t> retval;
   int num_words = header.samples_per_chan() / 4;
   retval.resize(num_words);

   uint64_t *p = wf_begin;
   DWORD i = 0;

   while (p < wf_end) {
      for (int j = 0; j < channel; j++) {
         if (header.ch_enable_mask & (((uint64_t)1) << j)) {
            p++; //
         }
      }

      retval[i] = *p++;

      for (int j = channel + 1; j < 64; j++) {
         if (header.ch_enable_mask & (((uint64_t)1) << j)) {
            p++; //
         }
      }

      i++;
   }

   return retval;
}

void CaenEvent::hencode(uint64_t* buffer) {
   header.hencode(buffer);
   buffer += 3;

   for (uint64_t *p = wf_begin; p < wf_end; p++) {
      *buffer++ = *p;
   }
}

#ifndef CAEN_DATA_H
#define CAEN_DATA_H

#include "midas.h"
#include "caen_device.h"
#include "caen_parameters.h"
#include <inttypes.h>
#include <stdlib.h>
#include <map>
#include <memory>
#include <exception>

#define VX_NO_EVENT 150

class CaenData {
   public:
      CaenData() {}
      ~CaenData() {}

      void set_device(std::shared_ptr<CaenDevice> _dev) {
         dev = _dev;
      }

      // Get raw data from board.
      // buffer should have enough space to hold at least get_max_raw_bytes_per_read() bytes.
      // convert_to_host_order=false does a direct memcpy
      // convert_to_host_order=true applies htonl() to all the incoming data, so is slower, but the data is easier to use.
      INT get_raw_data(int timeout_ms, uint8_t* buffer, size_t& num_bytes_read, bool convert_to_host_order=true);

      // Get decoded data for an event.
      // Waveforms should be pre-allocated as [64][NUM_SAMPLES].
      INT get_decoded_scope_data(int timeout_ms, uint64_t &timestamp, uint32_t &event_counter, uint16_t &event_flags, uint16_t** waveforms);

      // Get decoded data for an event.
      // Waveform should be pre-allocated as [MAX_NUM_SAMPLES].
      INT get_decoded_user_data(int timeout_ms, uint8_t& channel_id, uint64_t &timestamp, size_t& waveform_size, uint16_t* waveform);

      // Encode data read by get_decoded_scope_data() into a buffer in the same format
      // as the prototype boards gave their data.
      // Returns the number of bytes written to buffer.
      uint32_t encode_scope_data_to_buffer(uint64_t chan_enable_mask, uint32_t wf_len_samples, uint64_t timestamp, uint32_t event_counter, uint16_t event_flags, uint16_t** waveforms, uint8_t* buffer);

      // Encode data read by get_decoded_user_data() into a buffer in the same format
      // as the prototype boards gave their data.
      // Returns the number of bytes written to buffer.
      uint32_t encode_user_data_to_buffer(uint8_t channel_id, uint64_t timestamp, size_t waveform_size, uint16_t* waveform, uint8_t* buffer);

      // Get a handle for reading waveform data.
      // Choose either raw or decoded data.
      // This must be called BEFORE starting acquisition!
      INT setup_data_handle(bool raw, CaenParameters& params);

   protected:
      std::shared_ptr<CaenDevice> dev = nullptr;
      bool debug = false;

      int user_mode_event_count = 0;
      bool setup_raw_data = false;
      bool setup_decoded_scope_data = false;
      bool setup_decoded_user_data = false;
      uint64_t data_handle = 0;
      std::string fw_type;
};

#endif
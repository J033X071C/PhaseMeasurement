#include "caen_data.h"
#include "caen_event.h"
#include "caen_exceptions.h"
#include "CAEN_FELib.h"
#include "midas.h"
#include <arpa/inet.h>
#include <string.h>

INT CaenData::setup_data_handle(bool raw, CaenParameters& params) {
   user_mode_event_count = 0;
   params.get_firmware_type(fw_type);

   std::string endpoint, format, activeendpoint;

   if (raw) {
      endpoint = "/endpoint/raw";
      format = "";
      activeendpoint = "raw";
   } else {
      if (fw_type == "Scope") {
         endpoint = "/endpoint/scope";
         format = "[ { \"name\" : \"TIMESTAMP\", \"type\" : \"U64\" }, { \"name\" : \"TRIGGER_ID\", \"type\" : \"U32\" }, { \"name\" : \"WAVEFORM\", \"type\" : \"U16\", \"dim\" : 2 }, { \"name\" : \"WAVEFORM_SIZE\", \"type\" : \"U32\", \"dim\" : 1 }, { \"name\" : \"FLAGS\", \"type\" : \"U16\" } ]";
         activeendpoint = "scope";
      } else {
         endpoint = "/endpoint/opendpp";
         format = "[ { \"name\" : \"CHANNEL\", \"type\" : \"U8\" }, { \"name\" : \"TIMESTAMP\", \"type\" : \"U64\" }, { \"name\" : \"WAVEFORM_SIZE\", \"type\" : \"SIZE_T\" }, { \"name\" : \"WAVEFORM\", \"type\" : \"U16\", \"dim\" : 1 } ]";
         activeendpoint = "opendpp";
      }
   }

   // Get the data handle
   if (CAEN_FELib_GetHandle(dev->get_root_handle(), endpoint.c_str(), &data_handle) != CAEN_FELib_Success) {
      dev->print_last_error("Failed to get data endpoint handle\n");
      return FE_ERR_DRIVER;
   }

   // Configure whether to give raw or decoded data
   try {
      params.set("/endpoint/par/activeendpoint", activeendpoint);
   } catch (CaenException& e) {
      dev->print_last_error("Failed to set active endpoint. You may need to upgrade to Dig2 library version >= 1.2.0 and/or upgrade your firmware\n");
      return FE_ERR_DRIVER;
   }

   if (CAEN_FELib_SetReadDataFormat(data_handle, format.c_str()) != CAEN_FELib_Success) {
      dev->print_last_error("Failed to set data format");
      return FE_ERR_DRIVER;
   }

   if (raw) {
      setup_raw_data = true;
      setup_decoded_scope_data = false;
      setup_decoded_user_data = false;
   } else if (fw_type == "Scope") {
      setup_raw_data = false;
      setup_decoded_scope_data = true;
      setup_decoded_user_data = false;
   } else {
      setup_raw_data = false;
      setup_decoded_scope_data = false;
      setup_decoded_user_data = true;
   }

   return SUCCESS;
}

INT CaenData::get_raw_data(int timeout_ms, uint8_t* buffer, size_t& num_bytes_read, bool convert_to_host_order) {
   if (!setup_raw_data) {
      return FE_ERR_DRIVER;
   }

   int ret = CAEN_FELib_ReadData(data_handle, timeout_ms, buffer, &num_bytes_read);

   if (ret == CAEN_FELib_Success) {
      if (convert_to_host_order) {
         uint64_t* buf64 = (uint64_t*) buffer;
         uint32_t* buf32 = (uint32_t*) buffer;

         for (int i = 0; i < num_bytes_read/sizeof(uint64_t); i++) {
            buf64[i] = (((uint64_t)htonl(buf32[i * 2])) << 32) | htonl(buf32[i * 2 + 1]);
         }
      }
      return SUCCESS;
   } else if (ret == CAEN_FELib_Timeout) {
      return VX_NO_EVENT;
   } else {
      return FE_ERR_DRIVER;
   }
}

INT CaenData::get_decoded_scope_data(int timeout_ms, uint64_t &timestamp, uint32_t &event_counter, uint16_t &event_flags, uint16_t** waveforms) {
   if (!setup_decoded_scope_data) {
      return FE_ERR_DRIVER;
   }

   uint32_t waveform_sizes[64];
   int ret = CAEN_FELib_ReadData(data_handle, timeout_ms, &timestamp, &event_counter, waveforms, waveform_sizes, &event_flags);

   if (ret == CAEN_FELib_Success) {
      return SUCCESS;
   } else if (ret == CAEN_FELib_Timeout) {
      return VX_NO_EVENT;
   } else {
      return FE_ERR_DRIVER;
   }
}

INT CaenData::get_decoded_user_data(int timeout_ms, uint8_t& channel_id, uint64_t &timestamp, size_t& waveform_size, uint16_t* waveform) {
   if (!setup_decoded_user_data) {
      return FE_ERR_DRIVER;
   }

   int ret = CAEN_FELib_ReadData(data_handle, timeout_ms, &channel_id, &timestamp, &waveform_size, waveform);

   if (ret == CAEN_FELib_Success) {
      return SUCCESS;
   } else if (ret == CAEN_FELib_Timeout) {
      return VX_NO_EVENT;
   } else {
      return FE_ERR_DRIVER;
   }
}

uint32_t CaenData::encode_scope_data_to_buffer(uint64_t chan_enable_mask, uint32_t wf_len_samples, uint64_t timestamp, uint32_t event_counter, uint16_t event_flags, uint16_t** waveforms, uint8_t* buffer) {
   // Encode into buffer in same format as prototype VX2740 did.
   CaenEventHeader header;
   header.ch_enable_mask = chan_enable_mask;

   int num_chan = 0;

   for (int i = 0; i < 64; i++) {
      if (header.ch_enable_mask & ((uint64_t)1) << i) {
         num_chan++;
      }
   }

   header.event_counter = event_counter;
   header.flags = event_flags;
   header.format = 0x10;
   header.overlap = 0;
   header.size_64bit_words = 3 + num_chan * (wf_len_samples/4);
   header.trigger_time = timestamp;

   uint64_t* dwp = (uint64_t*)buffer;
   header.hencode(dwp);
   dwp += 3;

   for (int s = 0; s < wf_len_samples; s += 4) {
      for (int c = 0; c < 64; c++) {
         if (header.ch_enable_mask & ((uint64_t)1) << c) {
            uint64_t samp_dc = htonl(((uint32_t)(waveforms[c][s+3]) << 16) | (waveforms[c][s+2]));
            uint64_t samp_ba = htonl(((uint32_t)(waveforms[c][s+1]) << 16) | (waveforms[c][s]));

            *dwp++ = (samp_dc << 32) | samp_ba;
         }
      }
   }

   return header.size_bytes();
}

uint32_t CaenData::encode_user_data_to_buffer(uint8_t channel_id, uint64_t timestamp, size_t waveform_size, uint16_t* waveform, uint8_t* buffer) {
   // Encode into buffer in same format as prototype VX2740 did.
   CaenEventHeader header;
   header.ch_enable_mask = ((uint64_t)1) << channel_id;

   header.event_counter = user_mode_event_count++;
   header.flags = 0;
   header.format = 0x10;
   header.overlap = 0;
   header.size_64bit_words = 3 + (waveform_size/4);
   header.trigger_time = timestamp;

   uint64_t* dwp = (uint64_t*)buffer;
   header.hencode(dwp);
   dwp += 3;

   // User data is already in correct byte order.
   memcpy((void*)dwp, (void*)waveform, waveform_size*sizeof(uint16_t));

   return header.size_bytes();
}

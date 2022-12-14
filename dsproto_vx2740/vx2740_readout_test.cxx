/**
 * Test program for reading data from different endpoints.
 */

#include "vx2740_wrapper.h"
#include "vx2740_fe_class.h"
#include <sys/time.h>

#define NUM_SAMPLES 1000

void print_sources(VX2740& vx) {
  std::string s;
  vx.params().get_dig("acqtriggersource", s);
  printf("Trigger sources: %s\n", s.c_str());
}

void set_settings(VX2740& vx, bool is_scope) {
  // SW and test pulse only
  vx.params().set_trigger_sources(false, false, false, false, true, false, true, false);
  vx.params().set_channel_enable_mask(0x1, 0x0);
  vx.params().set_test_pulse(500, 104, 0, 1000);

  if (is_scope) {
    vx.params().set_waveform_length_samples(NUM_SAMPLES);
    vx.params().set_num_trigger_delay_samples(100);
    vx.params().set_pre_trigger_samples(100);
  } else {
    vx.params().set_user_register(0x300, NUM_SAMPLES); // Waveform length
    vx.params().set_user_register(0xB00, 100); // Pre-trig
    vx.params().set_user_register(0xC, 0); // No DS trigger
    vx.params().set_user_register(0x10, 0); // No DS trigger
    vx.params().set_user_register(0x600, 0x8); // Global trigger for chan 0

    for (int chan = 1; chan < 64; chan++) {
      vx.params().set_user_register(0x600 + 4 * chan, 0x0); // No trigger for other channels
    }
  }
}

/**
 * Run test using VX2740::get_raw_data() function.
*/
void do_raw(VX2740& vx) {
  vx.data().setup_data_handle(true, vx.params());
  printf("Reading raw data for 5s...\n");
  print_sources(vx);

  vx.commands().start_acq(true);
  size_t num_bytes_read = 0;
  uint8_t* buffer = (uint8_t*)calloc(1000000, sizeof(uint8_t));
  int num_read = 0;
  int num_empty = 0;
  int num_err = 0;

  timeval start = {};
  gettimeofday(&start, NULL);

  while (true) {
    INT status = vx.data().get_raw_data(100, buffer, num_bytes_read);

    if (status == SUCCESS) {
      num_read++;
    } else if (status == VX_NO_EVENT) {
      num_empty++;
    } else {
      num_err++;
    }

    ss_sleep(1);
    timeval now = {};
    gettimeofday(&now, NULL);

    if (now.tv_sec - start.tv_sec > 5) {
      break;
    }
  }
  
  vx.commands().stop_acq();
  printf("Read %d events in raw mode\n", num_read);

  free(buffer);
}

/**
 * Run test using VX2740::get_decoded_scope_data() or 
 * VX2740::get_decoded_user_data() functions.
*/
void do_formatted(VX2740& vx, bool is_scope) {
  vx.data().setup_data_handle(false, vx.params());
  printf("Reading formatted data for 5s...\n");
  print_sources(vx);

  vx.commands().start_acq(true);
  size_t num_bytes_read = 0;
  uint8_t* buffer = (uint8_t*)calloc(1000000, sizeof(uint8_t));
  int num_read = 0;
  int num_empty = 0;
  int num_err = 0;

  timeval start = {};
  gettimeofday(&start, NULL);

  uint64_t timestamp = 0;
  uint32_t event_counter = 0;
  uint16_t event_flags = 0;
  uint16_t** waveforms = (uint16_t**)calloc(64, sizeof(uint16_t*));
  uint8_t channel_id = 0;
  size_t waveform_size = 0;
	
	for (unsigned i = 0; i < 64; i++) {
    waveforms[i] = (uint16_t*)calloc(NUM_SAMPLES, sizeof(uint16_t));
	}

  uint16_t* user_waveform = (uint16_t*)calloc(100000, sizeof(uint16_t));

  while (true) {
    INT status = SUCCESS;

    if (is_scope) {
      status = vx.data().get_decoded_scope_data(100, timestamp, event_counter, event_flags, waveforms);
    } else {
      status = vx.data().get_decoded_user_data(100, channel_id, timestamp, waveform_size, user_waveform);
    }
    
    if (status == SUCCESS) {
      num_read++;
    } else if (status == VX_NO_EVENT) {
      num_empty++;
    } else {
      num_err++;
    }

    ss_sleep(1);
    timeval now = {};
    gettimeofday(&now, NULL);

    if (now.tv_sec - start.tv_sec > 5) {
      break;
    }
  }
  
  vx.commands().stop_acq();
  printf("Read %d events in formatted mode\n", num_read);

  for (int i = 0; i < 64; i++) {
    free(waveforms[i]);
  }
  free(waveforms);
  free(user_waveform);
}

/**
 * Run test using VX2740GroupFrontend class without ODB access.
*/
void do_fe(std::string board_name, bool is_scope) {
  std::shared_ptr<VX2740FeSettingsManual> fake_odb = std::make_shared<VX2740FeSettingsManual>();
  VX2740GroupFrontend fe(fake_odb, false);

  fake_odb->manual_group_settings.num_boards = 1;
  fake_odb->manual_group_settings.multithreaded_readout = false;

  BoardSettings settings;
  settings.bools.at("Scope mode (restart on change)") = is_scope;
  settings.strings.at("Hostname (restart on change)") = board_name;

  settings.bools.at("Trigger on test pulse") = true;
  settings.bools.at("Trigger on external signal") = false;
  settings.doubles.at("Test pulse period (ms)") = 500;

  settings.uint32s.at("Readout channel mask (31-0)") = 0x1;
  settings.uint32s.at("Readout channel mask (63-32)") = 0;

  settings.uint32s.at("Waveform length (samples)") = NUM_SAMPLES;
  settings.uint32s.at("Trigger delay (samples)") = 100;
  settings.uint16s.at("Pre-trigger (samples)") = 100;

  settings.vec_uint16s.at("User registers/Waveform length (samples)") = std::vector<uint16_t>(64, NUM_SAMPLES);
  settings.vec_uint16s.at("User registers/Pre-trigger (samples)") = std::vector<uint16_t>(64, 100);
  settings.uint32s.at("User registers/Trigger on global (31-0)") = 0x1; // Global trigger for chan 0

  fake_odb->manual_board_settings[0] = settings;

  char error[256];

  if (fe.init(-1, false) != SUCCESS) {
    printf("Failed to init FE test\n");
    return;
  }
  
  if (fe.begin_of_run(0, error) != SUCCESS) {
    printf("Failed to setup FE test\n");
    return;
  }

  printf("Reading frontend data for 5s...\n");
  print_sources(*(fe.get_boards()[0]));

  int num_read = 0;
  int num_empty = 0;
  timeval start = {};
  gettimeofday(&start, NULL);
  char* pevent =  (char*)calloc(1000000, sizeof(char));

  while (true) {
    if (fe.is_event_ready()) {
      fe.write_data(pevent);
      num_read++;
    } else {
      num_empty++;
    }

    ss_sleep(1);
    timeval now = {};
    gettimeofday(&now, NULL);

    if (now.tv_sec - start.tv_sec > 5) {
      break;
    }
  }

  printf("Read %d events in frontend mode\n", num_read);
  fe.end_of_run(0, error);

  free(pevent);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("This program connects to a board and reads data, like the CAEN demo programs.\n");
    printf("Usage: %s <hostname>\n", argv[0]);
    return 0;
  }

  VX2740 vx;

  if (vx.connect(argv[1]) != SUCCESS) {
    printf("Aborting test as we failed to connect to digitizer\n");
    return 1;
  }

  std::string fw_type;
  vx.params().get_firmware_type(fw_type);
  bool is_scope = (fw_type == "Scope");

  vx.commands().stop_acq();

  set_settings(vx, is_scope);

  do_raw(vx);
  do_formatted(vx, is_scope);

  vx.disconnect();
  do_fe(argv[1], is_scope);

  return 0;
}
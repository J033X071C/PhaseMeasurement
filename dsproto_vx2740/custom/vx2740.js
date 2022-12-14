var ds = ds || {};

/**
* Functions for the vslice.html page.
*/ 

ds.vx2740 = (function() {
  var group_board_row_ids_with_override = [];
  var group_board_row_ids_without_override = [];
  var num_group_board_rows = 0;
  var all_board_odb_classes = [];
  var is_fep_mode = false;

  // Global counter for number of modb* inputs we've created, so we can automate
  // generating IDs and callback functions that refer to IDs.
  var modb_idx = 0;

  // Help text to show beneath parameter name.
  let help_texts = {
    "LVDS quartet is input": "Configure 4 groups of 4 LVDS lines (0-3, 4-7, 8-11, 12-15).",
    "LVDS quartet mode": "SelfTriggers, Sync, IORegister, User.<br>Configure 4 groups of 4 LVDS lines (0-3, 4-7, 8-11, 12-15).<br>Sync lines are Busy/Veto/Trigger/Run.",
    "LVDS trigger mask (31-0)": "Configure each LVDS line individually.<br>Only useful if the corresponding quartet is set for output and SelfTriggers mode.",
    "LVDS trigger mask (63-32)": "Configure each LVDS line individually.<br>Only useful if the corresponding quartet is set for output and SelfTriggers mode.",
    "LVDS IO register": "Configure each LVDS line individually.<br>Only useful if the corresponding quartet is set for output and IORegister mode.",
    "User registers/LVDS output": "Configure each LVDS line individually.<br>Only useful if the corresponding quartet is set for output and User mode.",
    "Busy in source": "SIN, GPIO, LVDS, Disabled",
    "Sync out mode": "SyncIn, TestPulse, IntClk, Run, Disabled",
    "GPIO mode": "Disabled, TrgIn, P0, SIN, LVDS, ITLA, ITLB, ITLA_AND_ITLB, ITLA_OR_ITLB, EncodedClkIn, SwTrg, Run, RefClk, TestPulse, Busy, UserGPO, Fixed0, Fixed1",
    "Trigger ID mode": "TriggerCnt, EventCnt, LVDSpattern",
    "Trigger out mode": "Disabled, TRGIN, P0, SwTrg, LVDS, ITLA, ITLB, ITLA_AND_ITLB, ITLA_OR_ITLB, EncodedClkIn, Run, RefClk, TestPulse, Busy, UserTrgout, Fixed0, Fixed1, SyncIn, SIN, GPIO, LBinClk, AcceptTrg, TrgClk",
    "Veto source": "SIN, LVDS, GPIO, P0, EncodedClkIn, Disabled.<br>Multiple options allowed, separated by |",
    "Chan over thresh width (ns)": "0 means trigger signal lasts as long as the signal is over threshold.<br>Otherwise, use multiples of 8ns.",
    "Chan over thresh rising edge": "Checked means trigger is on when signal is above threshold.<br>Unchecked means trigger is on when signal is below threshold.",
    "Chan over thresh thresholds": "See also 'Use relative trig thresholds'",
    "Trigger on ch over thresh A&&B": "Only trigger if A and B fire at the same time",
    "Use external clock": "Checked means use clock from front panel connector.<br>Unchecked means use internal 62.5MHz clock.",
    "Enable clock out": "On 4-pin CLKOUT connector",
    "Ch over thresh A multiplicity": "Require N channels to be over threshold together to fire the 'A' trigger",
    "Ch over thresh B multiplicity": "Require N channels to be over threshold together to fire the 'B' trigger",
    "DC offset (pct)": "See also 'Enable DC offsets'",
    "User registers/Expert mode for trig settings": "WARNING: Expert trigger settings have odd interactions that are not intuitive. Only use if you know what you are doing!",
    "Trigger on user mode signal": "See 'Darkside trigger' section for threshold and LVDS logic settings",
    "Trigger on LVDS": "This is the default CAEN logic. Not the same as the Darkside-specific 'Enable LVDS pair 12 trigger' setting!",
    "Waveform length (samples)": "Max 524280 samples; multiples of 4 samples",
    "Pre-trigger (samples)": "Max 2042 samples",
    "Trigger delay (samples)": "Max 4294967295 samples",
    "User registers/Only read triggering channel": "If unchecked, each time a channel goes under threshold we'll read the triggering channel PLUS all channels that are in 'Readout channel mask' but NOT in 'Darkside trigger en mask'",
    "User registers/FIR filter coefficients": "-32768 to +32767 each",
    "User registers/Waveform length (samples)": "Max 16380 samples; multiples of 4 samples",
    "User registers/Pre-trigger (samples)": "Max 4095 samples",
    "User registers/Darkside trigger en mask(31-0)": "These channels can self-trigger",
    "User registers/Darkside trigger en mask(63-32)": "These channels can self-trigger",
    "User registers/Enable LVDS loopback": "Set LVDS output quartet 0 to be same as input quartet 1, and output quartet 2 to be same as input quartet 3.<br>Only applies if output quartets are set to User mode.",
    "User registers/Enable LVDS pair 12 trigger": "Trigger when signal seen on LVDS pair 12. Quartet 3 should be set to input and User mode.<br>NOT the same as 'Trigger on LVDS Sync signal'!",
    "User registers/Upper 32 mirror raw of lower 32": "Raw data for upper 32 channels should be same as lower 32 channels",
    "VGA gain": "0-40dB in 0.5dB increments<br>Group 0 affects channels 0-15, group 1 affects channels 16-31 etc.",
    "Test pulse width (ns)": "Multiples of 8ns"
  };

  let rdb_help_texts = {
    "LVDS IO register": "This reports the current LVDS state for both input quartets and output quartets.<br>The true state is reported regardless of the quartet mode.",
    "User registers/LVDS output": "This reports the LVDS state that would be output if all quartets were in output and User mode.",
    "User registers/LVDS input": "This reports the LVDS state for input quartets, regardless of the quartet mode.",
  };

  let display_names = {
    "Waveform length (samples)": "Scope waveform length",
    "User registers/Waveform length (samples)": "DPP waveform length",
    "Pre-trigger (samples)": "Scope pre-trigger",
    "User registers/Pre-trigger (samples)": "DPP pre-trigger",
    "User registers/Darkside trigger en mask(31-0)": "Darkside trigger enable mask (31-0)",
    "User registers/Darkside trigger en mask(63-32)": "Darkside trigger enable mask (63-32)",
    "LVDS IO register": "CAEN LVDS IO register",
    "User registers/LVDS output": "User LVDS output",
    "User registers/LVDS input": "User LVDS input",
    "VGA gain": "VGA gain (VX2745 only)",
    "Trigger on LVDS": "Trigger on LVDS Sync signal",
    "User registers/Trigger on falling edge": "Darkside trigger - on falling edge"
  };

  /**
  * Entry function that builds dynamic bits of webpage, then registers
  * with midas.
  */
  var init = function() {
    mjsonrpc_db_get_values(["/Equipment", "/VX2740 defaults"]).then(function(rpc_result) {
      let equip = rpc_result.result.data[0];
      let defaults = rpc_result.result.data[1];

      let properties = calc_table_properties(equip, defaults);
      is_fep_mode = properties["is_fep_mode"];

      for (let k in properties["board_odb_classes"]) {
        all_board_odb_classes.push(properties["board_odb_classes"][k]);
      }
      
      if (properties["show_groups_table"]) {
        build_groups_table(properties);
      }

      build_force_update_buttons(properties);
      build_parameters_table(properties);
      
      mhttpd_init('VX2740 settings');
    });
  };

  var calc_table_properties = function(equip, defaults) {
    let properties = {};
    properties["is_fep_mode"] = false;
    properties["show_defaults_column"] = (defaults !== null)
    properties["defaults_odb_path"] = "";
    properties["show_groups_table"] = (defaults !== null)
    properties["group_odb_paths"] = {};
    properties["board_odb_paths"] = {};
    properties["board_odb_classes"] = {};
    properties["num_boards"] = 0;
    properties["overridden_odb_paths"] = [];
    
    if (defaults === null) {
      // Single-fe mode
      document.getElementById("group_settings").style.display = "none";
      document.getElementById("group_info").style.display = "none";
    } else {
      for (let k in equip) {
        if (k.indexOf("fep_") == 0 && k.indexOf("/") == -1) {
          // Look for settings in /Equipment/FEP_001/Settings/VX2740 instead of
          // /Equipment/VX2740_Config_Group_001/Settings.
          properties["is_fep_mode"] = true;
        }
      }

      // Group-fe mode
      if (document.getElementById("single_info") != null) {
        document.getElementById("single_info").style.display = "none";
      }
    }

    if (properties["is_fep_mode"] && document.getElementById("group_to_single_help") != null) {
      document.getElementById("group_to_single_help").style.display = "none";
    }

    if (defaults === null) {
      // Single-fe mode
      for (let k in equip) {
        if (k.indexOf("vx2740_config") == 0 && k.indexOf("group") == -1 && k.indexOf("/") == -1) {
          let proper_case = "/Equipment/" + equip[k + "/name"] + "/Settings";
          properties["board_odb_paths"][proper_case] = k.replace("vx2740_config_", "");
          properties["board_odb_classes"][proper_case] = k.replace("vx2740_config_", "board_");
        }
      }
    } else {
      let equip_prefix = properties["is_fep_mode"] ? "fep_" : "vx2740_config_group_";
      properties["defaults_odb_path"] = "/VX2740 defaults"

      for (let k in equip) {
        if (k.indexOf(equip_prefix) == 0 && k.indexOf("/") == -1) {
          let settings_base = equip[k]["settings"];
          let readback_base = equip[k]["readback"];
          let suffix = "Settings";
  
          if (properties["is_fep_mode"]) {
            settings_base = settings_base["vx2740"];
            suffix += "/VX2740";
          }
          
          let group_proper_case = equip[k + "/name"];
          let group_odb_path = "/Equipment/" + group_proper_case + "/" + suffix;
          let group_title = k.replace(equip_prefix, "");
          properties["group_odb_paths"][group_odb_path] = group_title;
  
          for (let s in settings_base) {
            if (s.indexOf("board") != 0 || s.indexOf("/") != -1) {
              continue;
            }

            let board_odb_path = "/Equipment/" + group_proper_case + "/" + suffix + "/" + settings_base[s + "/name"];
            let board_title = k.replace(equip_prefix, "") + "/" + s.replace("board", "");
            properties["board_odb_paths"][board_odb_path] = board_title;
            properties["board_odb_classes"][board_odb_path] = "board_" + group_title + "_" + s.replace("board", "");

            let overrides = get_odb_paths_present(settings_base[s], board_odb_path);

            for (let i in overrides) {
              properties["overridden_odb_paths"].push(overrides[i]);
            }
          }
        }
      }
    }

    properties["num_groups"] = Object.keys(properties["group_odb_paths"]).length;
    properties["num_boards"] = Object.keys(properties["board_odb_paths"]).length;

    return properties;
  }

  var get_odb_paths_present = function(odb_obj, curr_path) {
    let retval = [];

    for (let prop in odb_obj) {
      if (prop.indexOf("/") != -1) {
        continue;
      }

      let subpath = curr_path + "/" + odb_obj[prop + "/name"];

      if (typeof(odb_obj[prop]) === 'object' && !Array.isArray(odb_obj[prop])) {
        let subpaths = get_odb_paths_present(odb_obj[prop], subpath);
        for (let i in subpaths) {
          retval.push(subpaths[i]);
        }
      } else {
        retval.push(subpath);
      }
    }

    return retval;
  }

  function FormatOptions() {
    this.is_boolean = false; // Show a checkbox
    this.is_array = false; // Show an array
    this.array_len = undefined; // Length of array, if is_array
    this.array_width = 4; // Number of columns if is_array
    this.modb_format = undefined; // See https://midas.triumf.ca/MidasWiki/index.php/Custom_Page#Formatting
    this.show_ns = false; // Show box for converting between samples and ns/us/ms
    this.show_channels_0_31 = false; // Show checkboxes for converting channel 0-31 bitmasks
    this.show_channels_32_63 = false; // Show checkboxes for converting channel 32-63 bitmasks
    this.pad_right = false; // Add a bit of padding to the right
    this.changes_scope_mode = false; // Add a callback to `scope_mode_changed` if value changes
    this.changes_expert_mode = false; // Add a callback to `expert_mode_changed` if value changes
  };

  function ShowInModes() {
    this.scope = true;    // Show when running Scope mode FW
    this.open_dpp = true; // Show when running Open DPP mode FW
    this.expert = true;  // Show when expert mode enabled
    this.simple = true;  // Show when expert mode disabled
  };

  var build_groups_table = function(properties) {
    let tot_cols = properties["num_groups"] + 1;

    let html = '<thead><tr><th class="mtableheader" colspan="' + tot_cols + '">';
    html += properties["is_fep_mode"] ? "Per-FEP" : "Per-frontend group";
    html += ' settings</th></tr></thead>';
    html += '<tbody>';
    
    if (properties["num_boards"] == 0) {
      html += '<tr><td>No group settings found</td></tr>';
    } else {
      // Header row
      html += '<tr><th>Parameter</th>';
      
      for (let k in properties["group_odb_paths"]) {
        html += '<th style="vertical-align:top">' + properties["group_odb_paths"][k] + '</th>';
      }

      let as_checkbox = new FormatOptions();
      as_checkbox.is_boolean = true;
      
      // Data rows
      html += add_group_row("Num boards (restart on change)", properties);
      html += add_group_row("Merge data using event ID", properties, as_checkbox);
      html += add_group_row("Debug data", properties, as_checkbox);
      html += add_group_row("Debug settings", properties, as_checkbox);
      html += add_group_row("Debug ring buffers", properties, as_checkbox);
      html += add_group_row("Multi-threaded readout", properties, as_checkbox);
    }
    
    html += '</tbody>';
    document.getElementById("group_settings").innerHTML = html;
  };

  var build_parameters_table = function(properties) {
    let only_scope = new ShowInModes();
    only_scope.open_dpp = false;

    let only_dpp = new ShowInModes();
    only_dpp.scope = false;

    let only_dpp_expert = new ShowInModes();
    only_dpp_expert.scope = false;
    only_dpp_expert.simple = false;

    let only_dpp_simple = new ShowInModes();
    only_dpp_simple.scope = false;
    only_dpp_simple.expert = false;

    let fmt_default = new FormatOptions();

    let chan_arr = new FormatOptions();
    chan_arr.is_array = true;
    chan_arr.array_len = 64;

    let chan_arr_ns = new FormatOptions();
    chan_arr_ns.is_array = true;
    chan_arr_ns.array_len = 64;
    chan_arr_ns.array_width = 1;
    chan_arr_ns.show_ns = true;
    
    let one_checkbox = new FormatOptions();
    one_checkbox.is_boolean = true;

    let arr_checkbox = new FormatOptions();
    arr_checkbox.is_boolean = true;
    arr_checkbox.is_array = true;
    arr_checkbox.array_len = 64;

    let bitmask_0_15 = new FormatOptions();
    bitmask_0_15.show_channels_0_15 = true;
    bitmask_0_15.modb_format = "x";
    bitmask_0_15.array_width = 1;

    let bitmask_0_31 = new FormatOptions();
    bitmask_0_31.show_channels_0_31 = true;
    bitmask_0_31.modb_format = "x";

    let bitmask_32_63 = new FormatOptions();
    bitmask_32_63.show_channels_32_63 = true;
    bitmask_32_63.modb_format = "x";

    let convert_ns = new FormatOptions();
    convert_ns.show_ns = true;

    let fmt_scope_mode = new FormatOptions();
    fmt_scope_mode.is_boolean = true;
    fmt_scope_mode.changes_scope_mode = true;

    let fmt_expert_mode = new FormatOptions();
    fmt_expert_mode.is_boolean = true;
    fmt_expert_mode.changes_expert_mode = true;

    let fmt_lvds_input = new FormatOptions();
    fmt_lvds_input.is_boolean = true;
    fmt_lvds_input.is_array = true;
    fmt_lvds_input.array_len = 4;
    fmt_lvds_input.array_width = 1;

    let fmt_lvds_mode = new FormatOptions();
    fmt_lvds_mode.is_array = true;
    fmt_lvds_mode.array_len = 4;
    fmt_lvds_mode.array_width = 1;

    let fmt_lvds_mask = new FormatOptions();
    fmt_lvds_mask.is_array = true;
    fmt_lvds_mask.array_len = 16;
    fmt_lvds_mask.array_width = 2;
    fmt_lvds_mask.modb_format = "x";

    let fmt_vga_gain = new FormatOptions();
    fmt_vga_gain.is_array = true;
    fmt_vga_gain.array_len = 4;
    fmt_vga_gain.array_width = 1;

    let fmt_coeff = new FormatOptions();
    fmt_coeff.is_array = true;
    fmt_coeff.array_len = 48;
    fmt_coeff.array_width = 4;

    let tot_cols = properties["num_boards"] + 1;

    if (properties["show_defaults_column"]) {
      tot_cols += 1;
    }

    let html = '<thead><tr><th class="mtableheader" colspan="' + tot_cols + '">Per-board settings</th></tr></thead>';
    html += '<tbody>';
    
    if (properties["num_boards"] == 0) {
      html += '<tr><td>No board settings found</td></tr>';
    } else {
      // Header row
      html += '<tr><th rowspan="2">Parameter</th>';
      
      if (properties["show_defaults_column"]) {
        html += '<th rowspan="2">Default value</th>';
      }

      let override_title = "Board settings";

      if (properties["show_defaults_column"]) {
        override_title = properties["is_fep_mode"] ? "FEP/board overrides" : "Group/board overrides";
      }

      html += '<th colspan="' + properties["num_boards"] + '">' + override_title + '</th></tr>';
      html += '<tr>';
      
      for (let k in properties["board_odb_paths"]) {
        html += '<th>' + properties["board_odb_paths"][k] + '</th>';
      }
      
      html += '</tr>';
      
      // Data rows
      html += begin_section("Main settings", properties);
      html += add_row("Hostname (restart on change)", properties);
      html += ds.vx2740.add_readback_row("Model name", properties);
      html += ds.vx2740.add_readback_row("Firmware version", properties);
      html += add_row("Enable", properties, one_checkbox);
      html += add_row("Read data", properties, one_checkbox), 
      html += add_row("Scope mode (restart on change)", properties, fmt_scope_mode);
  
      html += begin_section("Waveform readout", properties);
      html += add_row("Waveform length (samples)", properties, convert_ns, only_scope);
      html += add_row("User registers/Waveform length (samples)", properties, chan_arr_ns, only_dpp);
      html += add_row("Pre-trigger (samples)", properties, convert_ns, only_scope);
      html += add_row("User registers/Pre-trigger (samples)", properties, chan_arr_ns, only_dpp);
      html += add_row("Readout channel mask (31-0)", properties, bitmask_0_31);
      html += add_row("Readout channel mask (63-32)", properties, bitmask_32_63);
      html += add_row("Read fake sinewave data", properties, one_checkbox);
  
      html += begin_section("Global Trigger", properties);
      html += add_row("Trigger on ch over thresh A", properties, one_checkbox, only_scope);
      html += add_row("Trigger on ch over thresh B", properties, one_checkbox, only_scope);
      html += add_row("Trigger on ch over thresh A&&B", properties, one_checkbox, only_scope);
      html += add_row("Trigger on external signal", properties, one_checkbox);
      html += add_row("Trigger on software signal", properties, one_checkbox);
      html += add_row("Trigger on user mode signal", properties, one_checkbox);
      html += add_row("Trigger on test pulse", properties, one_checkbox);
      html += add_row("Trigger on LVDS", properties, one_checkbox);
      html += add_row("Test pulse period (ms)", properties);
      html += add_row("Allow trigger overlap", properties, one_checkbox, only_scope);
      html += add_row("Trigger delay (samples)", properties, convert_ns, only_scope);
      html += add_row("Trigger ID mode", properties, undefined, only_scope);
      html += add_row("Trigger out mode", properties);
  
      html += begin_section("CAEN threshold trigger", properties, only_scope);
      html += add_row("Ch over thresh A en mask(31-0)", properties, bitmask_0_31, only_scope);
      html += add_row("Ch over thresh A en mask(63-32)", properties, bitmask_32_63, only_scope);
      html += add_row("Ch over thresh A multiplicity", properties, fmt_default, only_scope);
      html += add_row("Ch over thresh B en mask(31-0)", properties, bitmask_0_31, only_scope);
      html += add_row("Ch over thresh B en mask(63-32)", properties, bitmask_32_63, only_scope);
      html += add_row("Ch over thresh B multiplicity", properties, fmt_default, only_scope);
      html += add_row("Chan over thresh rising edge", properties, arr_checkbox, only_scope);
      html += add_row("Use relative trig thresholds", properties, one_checkbox, only_scope);
      html += add_row("Chan over thresh thresholds", properties, chan_arr, only_scope);
      html += add_row("Chan over thresh width (ns)", properties, chan_arr, only_scope);
      
      html += begin_section("Darkside trigger", properties, only_dpp);
      html += add_row("User registers/Expert mode for trig settings", properties, fmt_expert_mode, only_dpp);
      html += add_row("User registers/Only read triggering channel", properties, one_checkbox, only_dpp_simple);
      html += add_row("User registers/Enable LVDS pair 12 trigger", properties, one_checkbox, only_dpp);
      html += add_row("User registers/Darkside trigger en mask(31-0)", properties, bitmask_0_31, only_dpp);
      html += add_row("User registers/Darkside trigger en mask(63-32)", properties, bitmask_32_63, only_dpp);
      html += add_row("User registers/Darkside trigger threshold", properties, chan_arr, only_dpp);
      html += add_row("User registers/Trigger on falling edge", properties, one_checkbox, only_dpp);
      html += add_row("User registers/Trigger on threshold (31-0)", properties, bitmask_0_31, only_dpp_expert);
      html += add_row("User registers/Trigger on threshold (63-32)", properties, bitmask_32_63, only_dpp_expert);
      html += add_row("User registers/Trigger on global (31-0)", properties, bitmask_0_31, only_dpp_expert);
      html += add_row("User registers/Trigger on global (63-32)", properties, bitmask_32_63, only_dpp_expert);
      html += add_row("User registers/Trigger on internal (31-0)", properties, bitmask_0_31, only_dpp_expert);
      html += add_row("User registers/Trigger on internal (63-32)", properties, bitmask_32_63, only_dpp_expert);
      html += add_row("User registers/Trigger on external (31-0)", properties, bitmask_0_31, only_dpp_expert);
      html += add_row("User registers/Trigger on external (63-32)", properties, bitmask_32_63, only_dpp_expert);
      
      html += begin_section("Darkside filter", properties, only_dpp);
      //html += add_row("User registers/Qlong length (samples)", properties, chan_arr_ns, only_dpp);
      //html += add_row("User registers/Qshort length (samples)", properties, chan_arr_ns, only_dpp);
      html += add_row("User registers/Upper 32 mirror raw of lower 32", properties, one_checkbox, only_dpp);
      html += add_row("User registers/Enable FIR filter (31-0)", properties, bitmask_0_31, only_dpp);
      html += add_row("User registers/Enable FIR filter (63-32)", properties, bitmask_32_63, only_dpp);
      html += add_row("User registers/Write unfiltered data (31-0)", properties, bitmask_0_31, only_dpp);
      html += add_row("User registers/Write unfiltered data (63-32)", properties, bitmask_32_63, only_dpp);
      html += add_row("User registers/FIR filter coefficients", properties, fmt_coeff, only_dpp);

      html += begin_section("Veto", properties);
      html += add_row("Veto source", properties);
      html += add_row("Veto when source is high", properties, one_checkbox);
      html += add_row("Veto width (ns) (0=source len)", properties);
  
      html += begin_section("Run start", properties);
      html += add_row("Run start delay (cycles)", properties);
      html += add_row("Start acq on encoded CLKIN", properties, one_checkbox);
      html += add_row("Start acq on first trigger", properties, one_checkbox);
      html += add_row("Start acq on LVDS", properties, one_checkbox);
      html += add_row("Start acq on midas run start", properties, one_checkbox);
      html += add_row("Start acq on P0", properties, one_checkbox);
      html += add_row("Start acq on SIN edge", properties, one_checkbox);
      html += add_row("Start acq on SIN level", properties, one_checkbox);
  
      html += begin_section("Front panel", properties);
      html += add_row("Use NIM IO", properties, one_checkbox);
      html += add_row("Use external clock", properties, one_checkbox);
      html += add_row("Enable clock out", properties, one_checkbox);
      html += add_row("GPIO mode", properties);
      html += add_row("Busy in source", properties);
      html += add_row("Sync out mode", properties);
      html += add_row("LVDS quartet is input", properties, fmt_lvds_input);
      html += add_row("LVDS quartet mode", properties, fmt_lvds_mode);
      html += add_row("User registers/Enable LVDS loopback", properties, one_checkbox, only_dpp);
      html += add_row("User registers/Enable LVDS pair 12 trigger", properties, one_checkbox, only_dpp);
      html += add_row("LVDS IO register", properties, bitmask_0_15);
      html += add_readback_row("LVDS IO register", properties, bitmask_0_15);
      html += add_row("User registers/LVDS output", properties, bitmask_0_15, only_dpp);
      html += add_readback_row("User registers/LVDS output", properties, bitmask_0_15, only_dpp);
      html += add_readback_row("User registers/LVDS input", properties, bitmask_0_15, only_dpp);
      html += add_row("LVDS trigger mask (31-0)", properties, fmt_lvds_mask);
      html += add_row("LVDS trigger mask (63-32)", properties, fmt_lvds_mask);
  
      html += begin_section("Signal conditioning", properties);
      html += add_row("Enable DC offsets", properties, one_checkbox);
      html += add_row("DC offset (pct)", properties, chan_arr);
      html += add_row("VGA gain", properties, fmt_vga_gain);
  
      html += begin_section("Test pulse", properties);
      html += add_row("Test pulse period (ms)", properties);
      html += add_row("Test pulse width (ns)", properties);
      html += add_row("Test pulse low level (ADC)", properties);
      html += add_row("Test pulse high level (ADC)", properties);
    }
    
    html += '</tbody>';
    document.getElementById("board_settings").innerHTML = html;
    
    // Highlight rows with differences
    for (let i in group_board_row_ids_with_override) {
      document.getElementById(group_board_row_ids_with_override[i]).className += " has_override";
    }
    for (let i in group_board_row_ids_without_override) {
      document.getElementById(group_board_row_ids_without_override[i]).className += " no_override";
    }

  };

  var begin_section = function(name, properties, mode_options=new ShowInModes()) {
    let row_class = row_class_for_mode_options(mode_options);
    let html = '<tr class="' + row_class + '"><th colspan="' + (properties["num_boards"] + 2) + '" class="mtableheader" style="font-size:smaller">' + name + '<a name="' + name + '"></a></th></tr>';
    
    let link_text = name;

    if (mode_options.open_dpp && !mode_options.scope) {
      link_text += " (DPP only)";
    }
    if (!mode_options.open_dpp && mode_options.scope) {
      link_text += " (Scope only)";
    }
    
    document.getElementById("links").innerHTML += '<span style="display: inline-block; padding-right:30px; white-space: nowrap;"><a href="#' + name + '">' + link_text + '</a></span>';
    
    return html;
  };

  var row_class_for_mode_options = function(mode_options) {
    let row_class = "";

    if (mode_options.open_dpp && !mode_options.scope) {
      row_class = "dpp_row";
    }
    if (!mode_options.open_dpp && mode_options.scope) {
      row_class = "scope_row";
    }

    return row_class;
  }

  /**
   * Add a row to the main table of parameters.
   * 
   * @param {string} name - Name of this parameter in the ODB
   * @param {Object} properties - 
   * @return {FormatOptions} format_options 
   * @return {ShowInModes} mode_options 
   */
  var add_row = function(name, properties, format_options, mode_options=new ShowInModes()) {
    num_group_board_rows += 1;

    let row_id = "board_row_" + num_group_board_rows;
    let help_html = help_texts.hasOwnProperty(name) ? "<br><div style='font-size:smaller;max-width:350px;font-style:italic'>" + help_texts[name] + "</small>" : "";
    let defaults_full_path = properties["defaults_odb_path"] + "/" + name;

    let display_name = name.replace("User registers/", "");

    if (display_names.hasOwnProperty(name)) {
      display_name = display_names[name];
    }

    let row_class = row_class_for_mode_options(mode_options);

    let html = '<tr id="' + row_id  + '" class="' + row_class + '">';
    html += '<td style="vertical-align:top">' + display_name + help_html + '</td>';
    
    if (properties["show_defaults_column"]) {
      html += '<td style="vertical-align:top">';
      html += modb_html(defaults_full_path, "defaults", format_options, mode_options);
      html += '</td>';
    }

    for (let board_odb_path in properties["board_odb_paths"]) {
      let full_path = board_odb_path + "/" + name;
      let extra_class = properties["board_odb_classes"][board_odb_path];
      let highlight = false;

      html += '<td style="vertical-align:top">';

      if (properties["show_defaults_column"]) {
        if (properties["overridden_odb_paths"].indexOf(full_path) != -1) {
          // Setting is overridden at board-level
          html += '<button role="button" class="mbutton" onclick="ds.vx2740.del_override(\'' + full_path + '\')" style="font-size:small; float:right; padding: 0px 2px 0px 2px;">&minus;</button>';
          html += modb_html(full_path, extra_class, format_options, mode_options);
          highlight = true;
        } else {
          // Setting is not overridden at board-level.
          // Still want to show/hide the + button if not relevant for this board's mode.
          html += mode_html_start(extra_class, mode_options);
          html += '<button role="button" class="mbutton" onclick="ds.vx2740.add_override(\'' + full_path + '\', \'' + defaults_full_path + '\')" style="font-size:small; float:right; padding: 0px 2px 0px 2px;">&plus;</button>';
          html += mode_html_end(mode_options);
        }
      } else {
        // Default settings don't exist, so no overriding logic
        html += modb_html(full_path, extra_class, format_options, mode_options);
      }

      html += '</td>';

      if (highlight) {
        group_board_row_ids_with_override.push(row_id);
      } else {
        group_board_row_ids_without_override.push(row_id);
      }
    }

    return html;
  };

  var add_group_row = function(name, properties, format_options) {
    let html = "<tr><td>" + name + "</td>";

    for (let odb_path in properties["group_odb_paths"]) {
      html += '<td>' + modb_html(odb_path + "/" + name, "", format_options) + '</td>';// !
    }

    html += '</tr>';
    return html;
  };

  var add_readback_row = function(name, properties, format_options, mode_options=new ShowInModes()) {
    num_group_board_rows += 1;

    let row_id = "board_row_" + num_group_board_rows;
    let help_html = rdb_help_texts.hasOwnProperty(name) ? "<br><div style='font-size:smaller;max-width:350px;font-style:italic'>" + rdb_help_texts[name] + "</small>" : "";

    let display_name = name.replace("User registers/", "");

    if (display_names.hasOwnProperty(name)) {
      display_name = display_names[name];
    }

    let row_class = row_class_for_mode_options(mode_options);

    let html = '<tr id="' + row_id  + '" class="' + row_class + '">';
    html += '<td style="vertical-align:top">' + display_name + " (readback)" + help_html + '</td>';
    
    if (properties["show_defaults_column"]) {
      html += '<td style="vertical-align:top"></td>';
    }

    for (let board_odb_path in properties["board_odb_paths"]) {
      let full_path = board_odb_path.replaceAll("Settings", "Readback") + "/" + name;
      let extra_class = properties["board_odb_classes"][board_odb_path];

      html += '<td style="vertical-align:top">';
      html += modb_html(full_path, extra_class, format_options, mode_options, true);
      html += '</td>';

      // Don't highlight readback rows
      group_board_row_ids_without_override.push(row_id);
    }

    return html;
  };

  /**
   * Zero-pad an integer. E.g. `pad_zero(1, 3)` returns "001".
   * 
   * @param {int} num - The number
   * @param {int} len - The string length to create
   * @return {string} - The 0-padded representation
   */
  var pad_zero = function(num, len) {
    let zeros = "";
    for (let i = 0; i < len; i++) {
      zeros += "0";
    }
    return (zeros + num).slice(-1 * len);
  };

  /**
   * Create HTML for wrapping content with a div that can be shown/hidden based
   * on whether a board is in scope/DPP mode and/or expert mode.
   * 
   * @param {string} extra_class 
   * @param {ShowInModes} mode_options 
   * @returns {string} - HTML
   */
  var mode_html_start = function(extra_class, mode_options) {
    let retval = "";

    if (mode_options.scope && !mode_options.open_dpp) {
      retval += '<div style="display:none; font-style:italic; font-size:smaller; width:100%;" class="dpp ' + extra_class + '">Scope FW only</div><div style="display:inline-block; width:100%;" class="scope ' + extra_class + '">';
    } else if (!mode_options.scope && mode_options.open_dpp) {
      retval += '<div style="display:none; font-style:italic; font-size:smaller; width:100%;" class="scope ' + extra_class + '">DPP FW only</div><div style="display:inline-block; width:100%;" class="dpp ' + extra_class + '">';
    }

    if (mode_options.expert && !mode_options.simple) {
      retval += '<div style="display:none; font-style:italic; font-size:smaller; width:100%;" class="simple ' + extra_class + '">Expert mode only</div><div style="display:inline-block; width:100%;" class="expert ' + extra_class + '">';
    } else if (!mode_options.expert && mode_options.simple) {
      retval += '<div style="display:none; font-style:italic; font-size:smaller; width:100%;" class="expert ' + extra_class + '">Non-expert mode only</div><div style="display:inline-block; width:100%;" class="simple ' + extra_class + '">';
    }

    return retval;
  }

  /**
   * Create HTML for ending any div that was created with `mode_html_start()`.
   * 
   * @param {ShowInModes} mode_options 
   * @returns {string} - HTML
   */
  var mode_html_end = function(mode_options) {
    let retval = "";

    if (mode_options.scope && !mode_options.open_dpp) {
      retval += '</div>';
    } else if (!mode_options.scope && mode_options.open_dpp) {
      retval += '</div>';
    }

    if (mode_options.expert && !mode_options.simple) {
      retval += '</div>';
    } else if (!mode_options.expert && mode_options.simple) {
      retval += '</div>';
    }

    return retval;
  }
  
  /**
  * Create HTML for displaying a checkbox or field for editing an ODB value.
  *
  * @protected
  * @param {string} odb_path - ODB location
  * @param {string} extra_class
  * @param {FormatOptions} format_options - 
  * @param {ShowInModes} mode_options - 
  * @param {boolean} readonly - 
  * @return {string} - HTML
  */
  var modb_html = function(odb_path, extra_class, format_options=new FormatOptions(), mode_options=new ShowInModes(), readonly=false) {
    // Outer div for showing hiding if in scope mode.
    let html = mode_html_start(extra_class, mode_options);
    let editable_str = readonly ? '' : ' data-odb-editable="1" ';
  
    if (format_options.is_array) {
      let elem_options = new FormatOptions();
      Object.assign(elem_options, format_options);
      elem_options.is_array = false;
      elem_options.pad_right = true;

      for (let i = 0; i < format_options.array_len; i++) {
        let idx = '[' + pad_zero(i, 2) + ']';
        html += idx + " ";
        html += modb_html(odb_path + idx, "", elem_options);
        if (i % format_options.array_width == format_options.array_width-1) {
          html += '<br>';
        }
      }
    } else if (format_options.is_boolean) {
      let classes = "modbcheckbox";
      html += '<input type="checkbox" data-odb-path="' + odb_path + '" ' + editable_str;
      
      if (format_options.pad_right) {
        html += ' style="margin-right:15px" ';
      }

      if (format_options.changes_scope_mode) {
        html += ' onchange="ds.vx2740.scope_mode_changed(\'' + extra_class + '\')" onload="onchange()" ';
        html += ' id="scope_mode_cb_' + extra_class + '"';
        classes += ' scope_mode_cb';
      }

      if (format_options.changes_expert_mode) {
        html += ' onchange="ds.vx2740.expert_mode_changed(\'' + extra_class + '\')" onload="onchange()" ';
        html += ' id="expert_mode_cb_' + extra_class + '"';
        classes += ' expert_mode_cb';
      }

      html += ' class="' + classes + '" ';

      html += '/>';
    } else {
      modb_idx += 1;
      let fmt_str = "";
      if (format_options.modb_format !== undefined) {
        fmt_str = ' data-format="' + format_options.modb_format + '"';
      }

      let onchange = "";

      if (format_options.show_ns) {
        onchange = ' onchange="ds.vx2740.convert_to_us(' + modb_idx + ')" onload="onchange()" ';
      } else if (format_options.show_channels_0_15) {
        onchange = ' onchange="ds.vx2740.convert_to_checkboxes(' + modb_idx + ', 16)" onload="onchange()" ';
      } else if (format_options.show_channels_0_31) {
        onchange = ' onchange="ds.vx2740.convert_to_checkboxes(' + modb_idx + ', 32)" onload="onchange()" ';
      } else if (format_options.show_channels_32_63) {
        onchange = ' onchange="ds.vx2740.convert_to_checkboxes(' + modb_idx + ', 32)" onload="onchange()" ';
      } 

      if (format_options.pad_right) {
        html += '<span style="margin-right:15px">';
      }

      html += '<span id="modb_' + modb_idx + '" class="modbvalue" data-odb-path="' + odb_path + '" ' + editable_str + fmt_str + onchange + '></span>';
    
      if (format_options.show_ns) {
        html += ' samples (<input id="us_' + modb_idx + '" style="max-width:50px" onblur="ds.vx2740.convert_from_us(' + modb_idx + ')"' + (readonly ? "disabled" : "") + '>&mu;s)';
      } else if (format_options.show_channels_0_15 || format_options.show_channels_0_31 || format_options.show_channels_32_63) {
        let offset = format_options.show_channels_32_63 ? 32 : 0;
        let len = format_options.show_channels_0_15 ? 16 : 32;
        html += '<br>';
        for (let chan = 0; chan < len; chan++) {
          html += '[' + pad_zero(chan + offset ,2) + '] <input style="margin-right:15px;" type="checkbox" id="cb_' + modb_idx + '_' + chan + '" onclick="ds.vx2740.convert_from_checkboxes(' + modb_idx + ', ' + len + ')" ' + (readonly ? "disabled" : "") + '>';
          if (chan % format_options.array_width == format_options.array_width - 1) { html += '<br>';}
        }
      }

      if (format_options.pad_right) {
        html += '</span>';
      }
    }

    html += mode_html_end(mode_options);

    return html;    
  };

  /**
   * Convert the value in an input file (specified in us) to a number of samples,
   * then set that value in the ODB.
   * 
   * @param {int} modb_idx - Suffix of element IDs for input and modbvalue (modb_X and us_X).
   */
  var convert_from_us = function(modb_idx) {
    let val_us = parseFloat(document.getElementById("us_" + modb_idx).value);
    let val_samples = Math.floor(val_us * 125);
    let odb_path = document.getElementById("modb_" + modb_idx).dataset.odbPath;

    mjsonrpc_db_set_value(odb_path, val_samples).then(function(rpc) {
    }).catch(function(error) {
      mjsonrpc_error_alert(error);
    });
  };

  /**
   * Convert an ODB value (stored in samples) to a time in us shown in an input field..
   * 
   * @param {int} modb_idx - Suffix of element IDs for input and modbvalue (modb_X and us_X).
   */
  var convert_to_us = function(modb_idx) {
    let val_samples = parseInt(document.getElementById("modb_" + modb_idx).innerText);
    let val_us = val_samples/125;
    document.getElementById("us_" + modb_idx).value = val_us;
  };

  /**
   * Convert the state of 32 checkboxes to a 32-bit bitmask, and store that value in the ODB.
   * 
   * @param {int} modb_idx - Suffix of element IDs for checkboxes and modbvalue (modb_X and cb_X_Y).
   * @param {int=} len - Number of checkboxes.
   */
  var convert_from_checkboxes = function(modb_idx, len) {
    if (len === undefined) {
      len = 32;
    }
    let mask = 0;
    for (let i = 0; i < len; i++) {
      let el = document.getElementById("cb_" + modb_idx + "_" + i);
      if (el.checked) {
        mask |= 1 << i;
      }
    }

    // Avoid negative hex values in JS.
    mask = mask >>> 0;

    // Set new value in ODB
    let odb_path = document.getElementById("modb_" + modb_idx).dataset.odbPath;

    mjsonrpc_db_set_value(odb_path, mask).then(function(rpc) {
    }).catch(function(error) {
      mjsonrpc_error_alert(error);
    });
  };

  /**
   * Convert an ODB value (bitmask) into checked state of 32 checkboxes.
   * 
   * @param {int} modb_idx - Suffix of element IDs for checkboxes and modbvalue (modb_X and cb_X_Y).
   * @param {int=} len - Number of checkboxes.
   */
  var convert_to_checkboxes = function(modb_idx, len) {
    if (len === undefined) {
      len = 32;
    }
    let mask = parseInt(document.getElementById("modb_" + modb_idx, 16).innerText);
    
    for (let i = 0; i < len; i++) {
      document.getElementById("cb_" + modb_idx + "_" + i).checked = (mask & (1<<i));
    }
  };

  var scope_mode_changed = function(changed_board_class) {
    let new_is_scope = document.getElementById("scope_mode_cb_" + changed_board_class).checked;

    // Figure out which boards are in scope/dpp mode
    let all_scope_cbs = document.getElementsByClassName('scope_mode_cb');
    let scope_default = true;
    let any_scope = false;
    let any_dpp = false;
    let scope_boards = {};

    for (let i = 0; i < all_scope_cbs.length; i++) {
      let el = all_scope_cbs[i];
      let is_scope = el.checked;

      any_scope = any_scope | is_scope;
      any_dpp = any_dpp | !is_scope;

      let board_class = el.id.replace("scope_mode_cb_", "");

      if (board_class == "defaults") {
        scope_default = is_scope;
      } else {
        scope_boards[board_class] = is_scope;
      }
    }

    // Determine scope/dpp for each board based on per-board and default settings
    for (let i in all_board_odb_classes) {
      let cl = all_board_odb_classes[i];
      if (!scope_boards.hasOwnProperty(cl)) {
        scope_boards[cl] = scope_default;
      }
    }

    // Show/hide rows based on whether any boards are in scope/dpp
    let dpp_elems = document.getElementsByClassName("dpp_row");
    for (let i = 0; i < dpp_elems.length; i++) {
      dpp_elems[i].style.display = any_dpp ? "table-row" : "none";
    }

    let scope_elems = document.getElementsByClassName("scope_row");
    for (let i = 0; i < scope_elems.length; i++) {
      scope_elems[i].style.display = any_scope ? "table-row" : "none";
    }

    // Show/hide defaults based on whether any boards are in scope/dpp
    if (scope_default !== undefined) {
      let elems = document.getElementsByClassName("defaults");
  
      for (let i = 0; i < elems.length; i++) {
        let el = elems[i];

        if (el.className.indexOf("scope") != -1) {
          el.style.display = any_scope ? 'inline-block' : 'none';
        } 
        if (el.className.indexOf("dpp") != -1) {
          el.style.display = any_dpp ? 'inline-block' : 'none';
        } 
      }
    }

    // Show/hide per-board settings based on whether that board is in scope/dpp
    for (let board_class in scope_boards) {
      let is_scope = scope_boards[board_class];
      let elems = document.getElementsByClassName(board_class);

      for (let i = 0; i < elems.length; i++) {
        let el = elems[i];

        if (is_scope) {
          if (el.className.indexOf("scope") != -1) {
            el.style.display = 'inline-block';
          } 
          if (el.className.indexOf("dpp") != -1) {
            el.style.display = 'none';
          } 
        } else {
          if (el.className.indexOf("scope") != -1) {
            el.style.display = 'none';
          } 
          if (el.className.indexOf("dpp") != -1) {
            el.style.display = 'inline-block';
          } 
        }
      }
    }
  };

  var expert_mode_changed = function(changed_board_class) {
    let new_is_expert = document.getElementById("expert_mode_cb_" + changed_board_class).checked;

    // Figure out which boards are in expert/non-expert mode
    let all_expert_cbs = document.getElementsByClassName('expert_mode_cb');
    let expert_default = true;
    let any_expert = false;
    let any_simple = false;
    let expert_boards = {};

    for (let i = 0; i < all_expert_cbs.length; i++) {
      let el = all_expert_cbs[i];
      let is_expert = el.checked;

      any_expert = any_expert | is_expert;
      any_simple = any_simple | !is_expert;

      let board_class = el.id.replace("expert_mode_cb_", "");

      if (board_class == "defaults") {
        expert_default = is_expert;
      } else {
        expert_boards[board_class] = is_expert;
      }
    }

    if (expert_default === undefined) {
      // Single-FE mode. Just change the board that changed.
      expert_boards = {changed_board_class: new_is_expert};
    } else {
      // Determine expert/simple for each board based on per-board and default settings
      for (let i in all_board_odb_classes) {
        let cl = all_board_odb_classes[i];
        if (!expert_boards.hasOwnProperty(cl)) {
          expert_boards[cl] = expert_default;
        }
      }
    }

    if (expert_default !== undefined) {
      // Show/hide defaults based on whether any boards are in expert/simple
      let elems = document.getElementsByClassName("defaults");
  
      for (let i = 0; i < elems.length; i++) {
        let el = elems[i];

        if (el.className.indexOf("expert") != -1) {
          el.style.display = any_expert ? 'inline-block' : 'none';
        }
        if (el.className.indexOf("simple") != -1) {
          el.style.display = any_simple ? 'inline-block' : 'none';
        }
      }
    }

    // Show/hide per-board settings based on whether that board is in scope/dpp
    for (let board_class in expert_boards) {
      let is_expert = expert_boards[board_class];
      let elems = document.getElementsByClassName(board_class);

      for (let i = 0; i < elems.length; i++) {
        let el = elems[i];

        if (is_expert) {
          if (el.className.indexOf("expert") != -1) {
            el.style.display = 'inline-block';
          } 
          if (el.className.indexOf("simple") != -1) {
            el.style.display = 'none';
          } 
        } else {
          if (el.className.indexOf("expert") != -1) {
            el.style.display = 'none';
          } 
          if (el.className.indexOf("simple") != -1) {
            el.style.display = 'inline-block';
          } 
        }
      }
    }
  };

  var build_force_update_buttons = function(properties) {
    let html = "";

    for (let k in properties["group_odb_paths"]) {
      let idx = properties["group_odb_paths"][k];
      html += '<button role="button" class="mbutton" id="force_update_' + idx + '" onclick="ds.vx2740.force_write(\'' + idx + '\')">Apply settings to group ' + idx + ' now</button>';
    }

    document.getElementById("force_update_buttons").innerHTML = html;
  };

  var force_write = function(group_idx) {
    let params = Object();
    let zero_pad;

    if (is_fep_mode) {
      zero_pad = ("000" + group_idx).slice(-3);
      params.client_name = "FEP_" + zero_pad;
    } else {
      zero_pad = ("00" + group_idx).slice(-2);
      params.client_name = "VX2740_Group_" + zero_pad;
    }
    
    params.cmd = "force_write";
    params.args = "";

    document.getElementById("force_update_" + group_idx).disabled = true;
    let orig_text = document.getElementById("force_update_" + group_idx).innerText;
    document.getElementById("force_update_" + group_idx).innerText = "Applying settings...";

    mjsonrpc_call("jrpc", params).then(function(rpc) {
      if (rpc.result.status == 103) {
        throw("Failed to run update! Client " + params.client_name + " isn't running!");
      }
      if (rpc.result.status != 1) {
        if (rpc.result.reply === "" || rpc.result.reply === undefined) {
          throw("Failed to run update! Response code was " + rpc.result.status + ". See Messages page for more.");
        } else {
          throw("Failed to run update! Response code was " + rpc.result.status + ": " + rpc.result.reply);
        }
      }

      document.getElementById("force_update_" + group_idx).disabled = false;
      document.getElementById("force_update_" + group_idx).innerText = "Settings applied!";
      setTimeout(function(){document.getElementById("force_update_" + group_idx).innerText = "Apply settings to group " + group_idx + " now";}, 1000);
    }).catch(function(error) {
      document.getElementById("force_update_" + group_idx).disabled = false;
      document.getElementById("force_update_" + group_idx).innerText = "Failed to apply settings!";
      setTimeout(function(){document.getElementById("force_update_" + group_idx).innerText = "Apply settings to group " + group_idx + " now";}, 1000);
      mjsonrpc_error_alert(error);
    });
  };

  var add_override = function(new_odb_path, copy_odb_path) {
    // Both db_key and db_get_values use the "paths" parameter
    let params = Object();
    params.paths = [copy_odb_path];
    let key_req = mjsonrpc_make_request("db_key", params);
    let val_req = mjsonrpc_make_request("db_get_values", params);
  
    mjsonrpc_send_request([key_req, val_req]).then(function(rpc) {
      let key = rpc[0].result.keys[0];
      let val = rpc[1].result.data[0];
      
      let new_odb = Object();
      new_odb.path = new_odb_path;
      new_odb.type = key.type;
      new_odb.array_length = key.num_values;
      
      if (new_odb.type == 12) {
        // String types
        new_odb.string_length = key.item_size;
      }
      
      mjsonrpc_db_create([new_odb]).then(function(rpc) {
        mjsonrpc_db_set_value(new_odb_path, val).then(function(rpc) {
          location.reload();
        });
      });
    });

  };
  
  var del_override = function(odb_path) {
    mjsonrpc_db_delete([odb_path]).then(function(rpc) {
      location.reload();
    });
  };

  return {
    init: init,
    calc_table_properties: calc_table_properties,
    add_override: add_override,
    del_override: del_override,
    convert_from_us: convert_from_us,
    convert_to_us: convert_to_us,
    convert_from_checkboxes: convert_from_checkboxes,
    convert_to_checkboxes: convert_to_checkboxes,
    scope_mode_changed: scope_mode_changed,
    expert_mode_changed: expert_mode_changed,
    build_force_update_buttons: build_force_update_buttons,
    force_write: force_write,
    add_row: add_row,
    add_readback_row: add_readback_row,
    all_board_odb_classes: all_board_odb_classes,
    group_board_row_ids_with_override: group_board_row_ids_with_override,
    group_board_row_ids_without_override: group_board_row_ids_without_override,
    FormatOptions: FormatOptions,
    ShowInModes: ShowInModes
  };
})();
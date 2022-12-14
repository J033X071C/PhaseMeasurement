var ds = ds || {};

/**
* Functions for the vslice.html page.
*/ 

ds.lvds = (function() {

  /**
  * Entry function that builds dynamic bits of webpage, then registers
  * with midas.
  */
  var init = function() {
    mjsonrpc_db_get_values(["/Equipment", "/VX2740 defaults"]).then(function(rpc_result) {
      let equip = rpc_result.result.data[0];
      let defaults = rpc_result.result.data[1];

      let properties = ds.vx2740.calc_table_properties(equip, defaults);

      for (let k in properties["board_odb_classes"]) {
        ds.vx2740.all_board_odb_classes.push(properties["board_odb_classes"][k]);
      }

      ds.vx2740.build_force_update_buttons(properties);
      build_parameters_table(properties);
      
      mhttpd_init('VX2740 LVDS settings');
    });
  };

  var build_parameters_table = function(properties) {
    let only_scope = new ds.vx2740.ShowInModes();
    only_scope.open_dpp = false;

    let only_dpp = new ds.vx2740.ShowInModes();
    only_dpp.scope = false;

    let only_dpp_expert = new ds.vx2740.ShowInModes();
    only_dpp_expert.scope = false;
    only_dpp_expert.simple = false;

    let only_dpp_simple = new ds.vx2740.ShowInModes();
    only_dpp_simple.scope = false;
    only_dpp_simple.expert = false;

    let fmt_default = new ds.vx2740.FormatOptions();

    let chan_arr = new ds.vx2740.FormatOptions();
    chan_arr.is_array = true;
    chan_arr.array_len = 64;

    let chan_arr_ns = new ds.vx2740.FormatOptions();
    chan_arr_ns.is_array = true;
    chan_arr_ns.array_len = 64;
    chan_arr_ns.array_width = 1;
    chan_arr_ns.show_ns = true;
    
    let one_checkbox = new ds.vx2740.FormatOptions();
    one_checkbox.is_boolean = true;

    let arr_checkbox = new ds.vx2740.FormatOptions();
    arr_checkbox.is_boolean = true;
    arr_checkbox.is_array = true;
    arr_checkbox.array_len = 64;

    let bitmask_0_15 = new ds.vx2740.FormatOptions();
    bitmask_0_15.show_channels_0_15 = true;
    bitmask_0_15.modb_format = "x";

    let bitmask_0_31 = new ds.vx2740.FormatOptions();
    bitmask_0_31.show_channels_0_31 = true;
    bitmask_0_31.modb_format = "x";

    let bitmask_32_63 = new ds.vx2740.FormatOptions();
    bitmask_32_63.show_channels_32_63 = true;
    bitmask_32_63.modb_format = "x";

    let bitmask_32_63_mirror_raw = new ds.vx2740.FormatOptions();
    bitmask_32_63_mirror_raw.show_channels_32_63 = true;
    bitmask_32_63_mirror_raw.modb_format = "x";
    bitmask_32_63_mirror_raw.hide_if_upper_chans_mirror_lower = true;

    let convert_ns = new ds.vx2740.FormatOptions();
    convert_ns.show_ns = true;

    let fmt_scope_mode = new ds.vx2740.FormatOptions();
    fmt_scope_mode.is_boolean = true;
    fmt_scope_mode.changes_scope_mode = true;

    let fmt_expert_mode = new ds.vx2740.FormatOptions();
    fmt_expert_mode.is_boolean = true;
    fmt_expert_mode.changes_expert_mode = true;

    let fmt_lvds_input = new ds.vx2740.FormatOptions();
    fmt_lvds_input.is_boolean = true;
    fmt_lvds_input.is_array = true;
    fmt_lvds_input.array_len = 4;
    fmt_lvds_input.array_width = 1;

    let fmt_lvds_mode = new ds.vx2740.FormatOptions();
    fmt_lvds_mode.is_array = true;
    fmt_lvds_mode.array_len = 4;
    fmt_lvds_mode.array_width = 1;

    let fmt_lvds_mask = new ds.vx2740.FormatOptions();
    fmt_lvds_mask.is_array = true;
    fmt_lvds_mask.array_len = 16;
    fmt_lvds_mask.array_width = 2;
    fmt_lvds_mask.modb_format = "x";

    let fmt_coeff = new ds.vx2740.FormatOptions();
    fmt_coeff.is_array = true;
    fmt_coeff.array_len = 48;
    fmt_coeff.array_width = 4;

    let tot_cols = properties["num_boards"] + 1;

    if (properties["show_defaults_column"]) {
      tot_cols += 1;
    }

    let html = '<thead><tr><th class="mtableheader" colspan="' + tot_cols + '">Per-board LVDS settings</th></tr></thead>';
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
      html += ds.vx2740.add_row("Hostname (restart on change)", properties);
      html += ds.vx2740.add_readback_row("Firmware version", properties);
      html += ds.vx2740.add_row("Enable", properties, one_checkbox);
      html += ds.vx2740.add_row("Scope mode (restart on change)", properties, fmt_scope_mode);

      html += ds.vx2740.add_row("LVDS quartet is input", properties, fmt_lvds_input);
      html += ds.vx2740.add_row("LVDS quartet mode", properties, fmt_lvds_mode);
      html += ds.vx2740.add_row("User registers/Enable LVDS loopback", properties, one_checkbox, only_dpp);
      html += ds.vx2740.add_row("User registers/Enable LVDS pair 12 trigger", properties, one_checkbox, only_dpp);
      html += ds.vx2740.add_row("LVDS IO register", properties, bitmask_0_15);
      html += ds.vx2740.add_readback_row("LVDS IO register", properties, bitmask_0_15);
      html += ds.vx2740.add_row("User registers/LVDS output", properties, bitmask_0_15, only_dpp);
      html += ds.vx2740.add_readback_row("User registers/LVDS output", properties, bitmask_0_15, only_dpp);
      html += ds.vx2740.add_readback_row("User registers/LVDS input", properties, bitmask_0_15, only_dpp);
      html += ds.vx2740.add_row("LVDS trigger mask (31-0)", properties, fmt_lvds_mask);
      html += ds.vx2740.add_row("LVDS trigger mask (63-32)", properties, fmt_lvds_mask);
  
    }
    
    html += '</tbody>';
    document.getElementById("board_settings").innerHTML = html;
    
    // Highlight rows with differences
    for (let i in ds.vx2740.group_board_row_ids_with_override) {
      document.getElementById(ds.vx2740.group_board_row_ids_with_override[i]).className += " has_override";
    }
    for (let i in ds.vx2740.group_board_row_ids_without_override) {
      document.getElementById(ds.vx2740.group_board_row_ids_without_override[i]).className += " no_override";
    }
  };

  return {
    init: init
  };
})();
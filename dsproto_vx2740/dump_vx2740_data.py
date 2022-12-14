"""
Simple tool that will print VX2740 data to screen.

If an argument is provided, we open that midas file.
If no argument is provided, we connect to the live midas experiment.

You can also use the `midas_to_vx2740()` function in your own code to
parse events from a midas file.

    import midas.file_reader
    import dump_vx2740_data
    
    midas_file = midas.file_reader.MidasFile("/path/to/file.mid.lz4")
    
    for ev in midas_file:
        all_vx_data = dump_vx2740_data.midas_to_vx2740(ev)

        if all_vx_data is not None:
            for vx in all_vx_data:
                if vx.format == 0x10:
                    print("Board %d has waveforms from channels %s" % (vx.board_id, ",".join(str(x) for x in vx.channels_enabled)))
"""

try:
    import midas.client
    have_midas = True
except ImportError:
    have_midas = False

import midas.file_reader
import argparse

class VX2740Data:
    """
    Encapsulates data from the VX2740 data acquired in Scope Mode format.
    Note that the dsproto_vx2740 frontend manipulates User Mode data to look
    like it was acquired in Scope Mode, to simplify analysis comparisons.

    Members:
        * fe_id (int) - Frontend index of the program that acquired the data
        * board_id (int) - Board index within the frontend index
        * format (int) - CAEN format flags (0x10 means regular data, others are other events)

    Members if format is 0x10 (regular data):
        * event_counter (int) - Increments with either each trigger or each readout, depending
            on "Trigger ID mode" parameter.
        * size_64bit_words (int) - Size of this event
        * flags (int) - Any extra flags set (see CAEN manual)
        * overlap (int) - Whether scope mode firmware detected this waveform overlaps with another
        * trigger_time_ticks (int) - Time since start of run in ticks of the 125MHz clock
        * trigger_time_secs (float) - Time since start of run in seconds
        * channels_enabled (list of int) - Which channels were read out in this event
        * waveforms (dict of {int: list of int}) - Map from channel number to waveform data

    Members if format is NOT 0x10 (special events):
        * data (list of int) - The raw data
    
    """
    def __init__(self, fe_id, board_id, data):
        self.fe_id = fe_id
        self.board_id = board_id
        self.format = (data[0] >> 56) & 0xFF
        
        if self.format == 0x10:
            self.event_counter = (data[0] >> 32) & 0xFFFFFF
            self.size_64bit_words = data[0] & 0xFFFFFFFF
            self.flags = data[1] >> 52
            self.overlap = (data[1] >> 48) & 0xF
            self.trigger_time_ticks = data[1] & 0xFFFFFFFFFFFF
            self.trigger_time_secs = self.trigger_time_ticks / 1.25e8
            
            chan_enable_mask = data[2]
            self.channels_enabled = [c for c in range(64) if chan_enable_mask & (0x1<<c)]
            
            num_header_words = 3
            num_samples_per_chan = (self.size_64bit_words - num_header_words) * 4
            
            self.waveforms = {c: [0] * num_samples_per_chan for c in self.channels_enabled}
            
            # This is a very slow way to unpack the data, but makes it clear what is happening.
            for w in range (num_header_words, self.size_64bit_words):
                # Get 1 64-bit word per channel, then moves to next channel
                c_idx = (w - num_header_words) % len(self.channels_enabled)
                samp = int((w - num_header_words) / len(self.channels_enabled)) * 4
                chan = self.channels_enabled[c_idx]
                
                self.waveforms[chan][samp] = (data[w] & 0xFFFF)
                self.waveforms[chan][samp + 1] = ((data[w] >> 16) & 0xFFFF)
                self.waveforms[chan][samp + 2] = ((data[w] >> 32) & 0xFFFF)
                self.waveforms[chan][samp + 3] = ((data[w] >> 48) & 0xFFFF)
        else:
            self.data = data
        
    def dump_summary(self):
        if self.format == 0x10:
            first_chan = self.channels_enabled[0]
            print("  Trigger # %d for board %03d/%02d"  % (self.event_counter, self.fe_id, self.board_id))
            print("    Format: 0x%x, flags: 0x%x, overlap: 0x%x" % (self.format, self.flags, self.overlap))
            print("    Trigger time: %.6fs" % self.trigger_time_secs)
            print("    First 10 samples on chan %d: %s" % (first_chan, self.waveforms[first_chan][:10]))
        elif self.format == 0x30:
            print("  Begin of run event for board %03d/%02d" % (self.fe_id, self.board_id))
            print("    Waveform width: %d samples" % ((self.data[1] & 0x1FFFFFF) * 4))
            print("    Channels enabled (31-00): 0x%08x" % (self.data[2] & 0xFFFFFFFFF))
            print("    Channels enabled (63-32): 0x%08x" % (self.data[3] & 0xFFFFFFFFF))
        elif self.format == 0x32:
            print("  End of run event for board %03d/%02d" % (self.fe_id, self.board_id))
            print("    Realtime: %d" % (self.data[1] & 0xFFFFFFFFFFFF))
            print("    Deadtime: %d" % (self.data[2] & 0xFFFFFFFF))
        else:
            print("  Event of unhandled format 0x%x for board %03d/%02d" % (self.format, self.fe_id, self.board_id))

def midas_to_vx2740(ev):
    """
    Args:
        
    * ev (`midas.event.MidasEvent`)

    Returns:
        list of `VX2740Data` objects, one per board.
    """
    if ev is None or ev.header.is_midas_internal_event():
        # Not a data event
        return None
    
    vx_data = []
    fe_id = ev.header.trigger_mask
    
    for bank in ev.banks.values():
        if bank.name.startswith("D"):
            try:
                # Extract board ID for banks named D001 etc
                board_id = int(bank.name.replace("D", ""))
            except ValueError:
                # Some other bank starting with D...
                continue
            
            parsed_data = VX2740Data(fe_id, board_id, bank.data)
            vx_data.append(parsed_data)
    
    return vx_data

def parse_and_print_event(ev):
    """
    Args:
        
    * ev (`midas.event.MidasEvent`)
    """
    vx_data = midas_to_vx2740(ev)

    if vx_data is None:
        # Not a data event
        return
        
    print("Midas event serial # %d" % ev.header.serial_number)
    
    for board_data in vx_data:
        board_data.dump_summary()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Dump VX2740 data to screen')
    parser.add_argument('filename', nargs="?", metavar="midas_file", help="If midas file not provided, connects to live experiment")
    args = parser.parse_args()
    
    if args.filename is not None:
        # Print from file
        mfile = midas.file_reader.MidasFile(args.filename)
        
        for ev in mfile:
            parse_and_print_event(ev)
    else:
        # Print live events
        if not have_midas:
            print("Wasn't able to import midas.client package, so not able to run in live mode!")

        client = midas.client.MidasClient("vx2740_data_dumper")
        buffer_handle = client.open_event_buffer("SYSTEM")
        client.register_event_request(buffer_handle, sampling_type=midas.GET_RECENT)
        
        while True:
            ev = client.receive_event(buffer_handle)
            parse_and_print_event(ev)
            client.communicate(1)
            
            
            
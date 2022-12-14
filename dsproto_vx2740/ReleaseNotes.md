
Release v0.2.0
==============

#### Overview:
* Support the latest CAEN Scope firmware and software.

#### New features:
* Add support for self-trigger settings (ITLA/ITLB).
* Add support for LVDS settings.
* Add a `vx2740_poke` program for getting/setting a single parameter of a board. Useful for debugging.
* Allow read-only programs to connect as "monitoring" clients of the board.
* Functionality for saving/loading bits of the ODB moved to https://bitbucket.org/ttriumfdaq/odb_save_load.
* Improvements to settings webpage, including grouping parameters into human-readable groups, and adding help text.

#### Bug Fixes:
* Fix byte-ordering of Open data. 

#### Compatibility constraints:
* Requires CAEN_FELib >= v1.1.5
* Requires CAEN_Dig2 >= v1.4.1
* Recommend Scope firmware >= 2021092300

Release v0.1.0
==============

#### Overview:
* Support the latest CAEN Scope firmware and software.
* Add preliminary support for DPP_OPEN firmware.

#### New features:
* Basic support of open FPGA boards. Users can specify register values in "User registers" subdirectory of board settings. Data is packaged to look like it was taken in Scope mode.
* Allow other code (e.g. the vertical slice frontend) to more easily integrate code from this package.

#### Bug Fixes:
* Fix encoding/decoding of "flags" member of the event header.
* Work around a gcc bug that occurs in some OSes that prevented us from stably loading CAEN's Dig2 library.

#### Compatibility constraints:
* Requires CAEN_Dig2 >= v1.2.0
* Recommend Scope firmware 2021040200
* Tested with CAEN_FELib v1.1.1, CAEN_Dig2 v1.2.4, Scope FW 2021040200


Release v0.0.3
==============

#### Overview:
* Allow compilation with gcc.

#### New features:
* N/A

#### Major bug Fixes:
* Allow compilation with gcc 7.5; previous tests were only with clang.

#### Compatibility constraints:
* Tested with CAEN_FELib v1.0.3 and CAEN_Dig2 v1.0.3


Release v0.0.2
==============

#### Overview:
* Add tool for saving/loading bits of the ODB.

#### New features:
* save_load.py midas client, plus custom webpage.

#### Major bug Fixes:
* N/A

#### Compatibility constraints:
* Same as v0.0.1


Release v0.0.1
==============

#### Overview:
* First release of frontends for VX2740 digitizers.
* Only supports "scope" mode firmware.

#### New features:
* "Single" and "group" frontends.
* Helper tool for generic CAEN_FELib access and VX2740-specific access
* Python program for parsing and dumping data.

#### Major bug Fixes:
* N/A

#### Compatibility constraints:
* Tested with CAEN_FELib v1.0.3 and CAEN_Dig2 v1.0.3
* Only works with clang, not gcc.

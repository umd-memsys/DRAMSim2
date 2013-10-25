DRAMSim2: A cycle accurate DRAM Simulator
================================================================================
* Elliott Cooper-Balis
* Paul Rosenfeld
* Bruce Jacob
* University of Maryland
* dramninjas [at] gmail [dot] com


About DRAMSim2
---------------
DRAMSim2 is a cycle accurate model of a DRAM memory controller, the DRAM modules which comprise
system storage, and the buses by which they communicate.
The overarching goal is to have a simulator that is small, portable, and accurate. The simulator core has a
simple interface which allows it to be CPU simulator agnostic and should to work with any simulator (see
section 4.2). This core has no external run time or build time dependencies and has been tested with g++ on
Linux as well as g++ on Cygwin on Windows.

Getting, Building, Documentation, etc.
----------------------------------
Please see the [DRAMSim2 wiki](https://wiki.umd.edu/DRAMSim2/index.php?title=Main_Page)




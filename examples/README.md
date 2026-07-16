# Examples

## TCP/IP virtual instrument server

The TCP/IP in this directory is compiled with ``make`` to produce
``tcp_server``. Launch ``tcp_server`` in a terminal, and in GNU Radio
Companion ``gnuradio310_tcpip.grc`` leading the the following output:

<img src="gnuradio310_tcpip.png">

The server generates as any sine waves as declared channel, first argument
sent by the client to the server. Tested July 2026 with GNU Radio 3.10.12.0
(Python 3.13.12)

## Agilent oscilloscope

Tested on a DSOS404A:

<img src="gnuradio310_DSOS404A.png">

<img src="DSOS404A_scrot.png">

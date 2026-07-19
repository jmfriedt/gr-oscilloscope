# Examples

## TCP/IP virtual instrument server

The TCP/IP server in this directory is compiled with ``make`` to produce
``tcp_server``. Launch ``tcp_server`` in a terminal (top-right on the
screenshot below), and in GNU Radio Companion ``gnuradio310_tcpip.grc`` 
leading the the following output:

<img src="gnuradio310_tcpip.png">

The server generates as many sine waves as declared channel, first argument
sent by the client to the server. Tested July 2026 with GNU Radio 3.10.12.0
(Python 3.13.12). The vector to stream is needed since the gr-oscilloscope 
block naturally generates vector with ``duration * samp_rate`` samples on each
channel

<img src="gnuradio310_tcpip_scrot.png">

## Agilent oscilloscope

Tested on a DSOS404A, with the IP address defined in the MS-Windows Network
Interface IPv4 configuration menu.

Flowchart, including the cross-correlation in the Fourier domain, emphasizing
the benefit of the vector output avoiding the ``Stream to Vector`` block before
each FFT (but requiring the ``Vector to stream`` before each oscilloscope/``Time Sink`` input): 

<img src="gnuradio310_DSOS404A.png">

Time sink outputs with the FFT blocks commented:

<img src="DSOS404A_scrot.png">

Time sink outputs with the FFT blocks running:

<img src="DSOS404A_scrot_FFT.png">

Screenshot of the oscilloscope showing how channel 1 is connected to the
test signal to trigger the acquisitions, while channels 2 and 3 are floating:

<img src="260717_AgilentDSOS404A.jpg">

## Rigol oscilloscope

Tested on the DHO814, setting the IP address by hitting on the bottom left
Rigol icon of the touchscreen, Utility and Static IP. After setting the static
IP address and disabling DHCP and Auto IP, **drag the window** up and click
on "Apply" hidden at the bottom of the menu.

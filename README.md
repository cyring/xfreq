# XFreq
## Purpose
XFreq provides a GUI to monitor the Processor frequencies (including Turbo Boost), the temperatures, C-States, and to alter the operational features of the Intel Core i7 processor.  
XFreq is also programmed for Core 2 and other Intel 64-bits architectures.

![alt text](http://blog.cyring.free.fr/images/XFreq-core-R2151.png "CORE")

![alt text](http://blog.cyring.free.fr/images/XFreq-temps-R2151.png "TEMPS")

![alt text](http://blog.cyring.free.fr/images/XFreq-cstates-R2151.png "CSTATES")


### Features
XFreq provides functionalities to play with:
 * Core Frequencies, Ratios and *Turbo Boost*
 * CPU Cycles, IPS, IPC, CPI
 * CPU C-States `C0 C1 C3 C6 C7`
 * Discrete Temperature per thread core
 * Processor features `[Yes]|[No]` and their activation state `[ON]|[OFF]`
 * Read and write MSR registers
 * DRAM controller & timings
 * Base Clock (from TSC, BIOS, factory value or a user defined memory address)
 * State of `[de]activated` Core by the OS or the BIOS
 * The xAPIC topology map
 * The Intel Performance counters
 * The Linux tasks scheduling monitoring.

### The GUI
 * The GUI Gadgets can be started as one MDI window or spanned in several Widgets, using the built-in window manager:
  * Right mouse button to move the Widgets on screen.
  * Left, to scroll Widgets, or push the GUI buttons and run the associated command.
 * A commands set allows to drive the Processor features, such as enabling, disabling `SpeedStep`, `Turbo Boost`, `Thermal Control`;  
   but also, miscellaneous XFreq functionalities, such as setting the RGB colors, font and layout; save and load from the configuration file.
 * The Widgets disposal is specified with the program options. See the help below.
 * The tasks scheduled by the kernel among the CPU Cores are shown in real time.
![alt text](http://blog.cyring.free.fr/images/xfreq-task-scheduling.png "TASKS, lets you know what's going on in your system !")

 * XFreq's editor.
![alt text](http://blog.cyring.free.fr/images/xfreq-main-R2143.png "Full editor with a command line history")

 * Enter an address and a CPU Core number, XFreq will monitor its register bits.
![alt text](http://blog.cyring.free.fr/images/xfreq-dump-R2143.png "REGISTERS")

## Build & Run
 *Prerequisites*: as root, load the MSR Kernel driver.
 - if _Linux_ then enter: 
```
modprobe msr
```
 - if _FreeBSD_ then enter:
```
kldload cpuctl
```
 1- Download the [source code](https://github.com/cyring/xfreq/archive/master.zip) into a working directory
 
 2- Build the programs:
```
make
```
 3- Run the server, as root:
```
./XFreq/svr/bin/xfreq-intel
```
 4- ... and the GUI client, as a regular user:
```
./XFreq/gui/bin/xfreq-gui
```
 5- start the console client with:
```
./XFreq/cli/bin/xfreq-cli
```

 6- To display the help, enter:
```
$ xfreq-gui -h
XFreq-Gui usage:
        xfreq-gui [-option argument] .. [-option argument]

where options include:
        -C      Path & file configuration name (String)
                  this option must be first
                  default is '$HOME/.xfreq'
        -D      Run as a MDI Window (Bool) [0/1]
        -U      Bitmap of unmap Widgets (Hex) eq. 0b00111111
                  where each bit set in the argument is a hidden Widget
        -u      Set the cursor shape (Bool) [0/1]
        -F      Font name (String)
                  default font is 'Fixed'
        -x      Enable or disable the X ACL (Char) ['Y'/'N']
        -g      Widgets geometries (String)
                  argument is a series of '#:[cols]x[rows]+[x]+[y], .. ,'
        -b      Background color (Hex) {RGB}
                  argument is coded with primary colors 0xRRGGBB
        -f      Foreground color (Hex) {RGB}
        -l      Fill or not the graphics (Bool) [0/1]
        -z      Show the Core frequency (Bool) [0/1]
        -y      Show the Core Cycles (Bool) [0/1]
        -j      Show the Instructions Per Second (Bool) [0/1]
        -J      Show the Instructions Per Cycle (Bool) [0/1]
        -i      Show the Cycles Per Instructions (Bool) [0/1]
        -r      Show the Core Ratio (Bool) [0/1]
        -p      Show the Core C-State percentage (Bool) [0/1]
        -w      Scroll the Processor brand wallboard (Bool) [0/1]
        -o      Keep the Widgets always on top of the screen (Bool) [0/1]
        -n      Remove the Window Manager decorations (Bool) [0/1]
        -N      Remove the Widgets title name from the WM taskbar (Bool) [0/1]
        -I      Splash screen attributes 0x{H}{NNN} (Hex)
                  where {H} bit:13 hides Splash and {NNN} (usec) defers start-up
        -S      Screenshot path (String)
                  default is '$HOME'
        -v      Print version information
        -h      Print out this message

Exit status:
0       if OK,
1       if problems,
2       if serious trouble.
```
```
# xfreq-intel -h
XFreq-Intel usage:
        xfreq-intel [-option argument] .. [-option argument]

where options include:
        -c      Pick up an architecture # (Int)
                  refer to the '-A' option
        -S      Clock source (Int)
                  argument is one of the [0]TSC [1]BIOS [2]SPEC [3]ROM [4]USER
        -M      ROM address of the Base Clock (Hex)
                  argument is the BCLK memory address to read from
        -s      Idle time multiplier (Int)
                  argument is a coefficient multiplied by 50000 usec
        -d      Registers dump enablement (Bool)
        -t      Task scheduling monitoring sorted by 0x{R}0{N} (Hex)
                  where {R} bit:8 is set to reverse sorting
                  and {N} is one '/proc/sched_debug' field# from [0x1] to [0xb]
        -z      Reset the MSR counters (Bool)
        -B      Enable SmBIOS (Bool)
        -A      Print out the built-in architectures
        -v      Print version information
        -h      Print out this message

Exit status:
0       if OK,
1       if problems,
2       if serious trouble.
```
### Algorithms
The algorithms employed are introduced in the [Algorithms Wiki]

### Architectures
`xfreq-intel -A` lists the built-in architectures and their default clock.
```
     Architecture              Family         CPU        FSB Clock
  #                            _Model        [max]        [ MHz ]

  0  GenuineIntel              00_00            2         100.00
  1  Core/Yonah                06_0E            2         100.00
  2  Core2/Conroe              06_0F            2         266.67
  3  Core2/Kentsfield          06_15            4         266.67
  4  Core2/Conroe/Yonah        06_16            4         266.67
  5  Core2/Yorkfield           06_17            4         266.67
  6  Xeon/Dunnington           06_1D            6         266.67
  7  Atom/Bonnell              06_1C            2         100.00
  8  Atom/Silvermont           06_26            8         100.00
  9  Atom/Lincroft             06_27            1         100.00
 10  Atom/Clovertrail          06_35            2         100.00
 11  Atom/Saltwell             06_36            2         100.00
 12  Silvermont                06_37            4          83.30
 13  Silvermont                06_4D            4          83.30
 14  Atom/Airmont              06_4C            4         100.00
 15  Atom/Goldmont             06_5C            4         100.00
 16  Atom/Sofia                06_5D            4         100.00
 17  Atom/Merrifield           06_4A            2         100.00
 18  Atom/Moorefield           06_5A            4         100.00
 19  Nehalem/Bloomfield        06_1A            4         133.33
 20  Nehalem/Lynnfield         06_1E            4         133.33
 21  Nehalem/Mobile            06_1F            2         133.33
 22  Nehalem/eXtreme.EP        06_2E            8         133.33
 23  Westmere                  06_25            2         133.33
 24  Westmere/EP               06_2C            6         133.33
 25  Westmere/eXtreme          06_2F           10         133.33
 26  SandyBridge               06_2A            4         100.00
 27  SandyBridge/eXtreme.EP    06_2D            6         100.00
 28  IvyBridge                 06_3A            4         100.00
 29  IvyBridge/EP              06_3E            6         100.00
 30  Haswell/Desktop           06_3C            4         100.00
 31  Haswell/Mobile            06_3F            4         100.00
 32  Haswell/Ultra Low TDP     06_45            2         100.00
 33  Haswell/Ultra Low eXtreme 06_46            2         100.00
 34  Broadwell/Mobile          06_3D            2         100.00
 35  Broadwell/EP              06_56           22         100.00
 36  Broadwell/H               06_47            4         100.00
 37  Broadwell/EX              06_4F           24         100.00
 38  Skylake/UY                06_4E            2         100.00
 39  Skylake/S                 06_5E            4         100.00
 40  Skylake/E                 06_55            4         100.00
```
_Tips_: you can force an architecture with the `-c` option.

#### Notes
 * XFreq is programmed in C and inline ASM, using the  [Code::Blocks](http://www.codeblocks.org) IDE and the _complete_ X11 developer packages and this [Workstation]
 * The very last development snapshot can be cloned from the git [repository](https://github.com/cyring/xfreq.git)
 * Based on the Xlib and the GNU libc, XFreq runs with any desktop environment.
 * I will be thankful for your testing results, including your processor specs and the facing XFreq data.

### Development
XFreq is a Clients-Server model based software.

#### API
  A shared memory interface `[SHM]` to exchange data with the server.

#### Server
  A daemon program which monitors the hardware and fills the `SHM` with the collected data.

  The server which requires the superuser privileges of root, receives and processes Client commands.

#### Clients
  Gui and text programs which connect to the server through the `SHM` and display the hardware activity.

  Clients *do not* require the root permissions.

### Screenshots
 * Intel Processors Server

![alt text](http://blog.cyring.free.fr/images/XFreq-Intel.png "Server")

 * Console Client

![alt text](http://blog.cyring.free.fr/images/XFreq-Client.png "Console")

 * GUI Client

![alt text](http://blog.cyring.free.fr/images/xfreq-core.png "GUI")

### v2.1
The files repository is the following:
```
./Makefile

./api/Makefile
./api/xfreq-api.h
./api/xfreq-api.c
./api/xfreq-smbios.h
./api/xfreq-smbios.c
./api/xfreq-types.h

./cli/Makefile
./cli/XFreq-Cli.cbp
./cli/XFreq-Cli-FreeBSD.cbp
./cli/xfreq-cli.c
./cli/xfreq-cli.h

./gui/Makefile
./gui/XFreq-Gui.cbp
./gui/XFreq-Gui-FreeBSD.cbp
./gui/xfreq-gui.c
./gui/xfreq-gui.h

./svr/Makefile
./svr/XFreq-Intel.cbp
./svr/XFreq-Intel-FreeBSD.cbp
./svr/xfreq-intel.c
./svr/xfreq-intel.h
```
### Build
The v2.1 is delivered with [Code::Blocks](http://www.codeblocks.org) project files.

An Arch Linux PKGBUILD is available in the [aur](http://aur.archlinux.org/packages/xfreq-git) repository.

Package is systemd compliant.

### Run
Processes must be run in the following order ...
- as root
```
# xfreq-intel
````
- as a user
```
$ xfreq-gui
```
... and in the reverse order to shutdown.

## News
 * Added a [Ctrl]+[s] hotkey to screen shot the focused widget.
 * Improved the fonts and colors management, and the integrated help.
 * New architectures implemented: Skylake, Broadwell
 * Web UI alpha release preview.  [_Canceled_]
 * ![alt text](http://blog.cyring.free.fr/images/XFreq-WebUI.png "XFreq Web UI")
 * Bit set calculator.
 * Added Performance Monitor Features.
 * New commands to enable, disable C3 and C1 auto demotion.
 * C7 state is implemented.
 * ![alt text](http://blog.cyring.free.fr/images/xfreq-cstates-R2148.png "CSTATES")
 * 3 performance ratios:
  * IPS (Instructions Per Second)
  * IPC (Instructions Per Cycle),
  * CPI (Cycles Per Instruction),
 * C1 state is now available .
 * 3 new commands to "safely" enable, disable:
  * EIST ( `SpeedStep` )
  * C1E ( `Extended Halt` )
  * TCC ( `Thermal Control` )
 * 5 new commands:
  * to enable, disable, the Intel Turbo Boost.
  * to read, write, and dump msr registers from any Core.
 * Integrated SmBIOS.
 * ![alt text](http://blog.cyring.free.fr/images/XFreq-SmBIOS.png "System Management BIOS")
 * Ported to FreeBSD.
 * Up to 64 simultaneous clients.
 * ![alt text](http://blog.cyring.free.fr/images/XFreq-64cli.png "64 Clients in action !")
# Regards
_`CyrIng`_

 Paris ;-)
 

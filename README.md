# LiveDiff
LiveDiff is a portable Windows differencing tool to perform system-level reverse engineering. LiveDiff is specifically developed to perform reverse engineering of application software to aid creating application profiles. LiveDiff is a portable tool designed for use on the Microsoft Windows operating system. 

## LiveDiff Availability

The source code for LiveDiff is distributed on GitHub: 

`https://github.com/thomaslaurenson/LiveDiff`

Precompiled binaries for Microsoft Windows can be found on the GitHub releases page:

`https://github.com/thomaslaurenson/LiveDiff/releases`

## LiveDiff Usage

LiveDiff is a console application. It needs to be run using the Command Prompt. Running as administrator is preferred as some file system and Windows Registry entries can only be accessed with administrator rights. On Windows 7 you can load the Command Prompt as administrator using the following actions:

`Start Menu > All Programs > Accessories > Command Prompt > Right Click > Run as Administrator`

To run LiveDiff and view the help menu use the following command (given that you are in the correct directory with the LiveDiff executable):

`LiveDiff-1.0.0.exe -h`

To capture and compare a system changes use LiveDiff without any arguments. This will capture one system snapshot, prompt you to perform an action (e.g., install an application), capture another snapshot, compare the two snapshots and output an APXML report. The snapshot process is looped until the user decide to exit the program.

#### Dynamic Blacklisting

Dynamic blacklisting is a novel technique used to filter irrelevant file system and Registry entries from differential analysis. Dynamic blacklisting works by performing a system snapshot before any differential analysis/reverse engineering is performed. The initial system snapshot is used to populate an in-memory blacklist of known file system and Registry paths (the full, or absolute, logical file system location). Any data files or Regitry values that are encountered are subjected to a blacklist lookup using the full path, if a match is found the entry is ignored. To include dynamic blacklisting to filter operating system files and Registry values when collecting snapshots use LiveDiff with the -d (for Dynamic blacklist) argument:

`LiveDiff-1.0.0.exe -d`

LiveDiff can be used to collect and perform differencing on multiple chronological runs. This is known as profile mode, which generates an Application Profile XML (APXML) document. This can be invoked using the profile mode of operation:

`LiveDiff-1.0.0.exe –profile`

## Supported Windows Versions

The following Windows operating system versions have been tested:

1. Microsoft Windows 7 Service Pack 1 (32-bit)

## License

LiveDiff is based on the Regshot project (http://sourceforge.net/projects/regshot/). The fileshot.c and regshot.c source code files were used as the foundation of the LiveDiff tool. LiveDiff implements the same GNU Lesser General Public License version 2.0 (LGPLv2). Please find the license details in the License.txt file distributed with this project.

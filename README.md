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

LiveDiff can be executed without any command line arguments to begin a looped snapshot collection and comparison cycle. The general operation is: 1) A snapshot is collected of the file system and Windows Registry; 2) An action is performed by the user (e.g., install an application); 3) Another snapshot is collected; 4) The before and after snapshots are compared and differential analysis performed to determine system level changes (e.g., files that have been created during application installation). This snapshot and comparison process is looped until the user decide to exit the program.

To start a looped snapshot and comparison process, execute LiveDiff without any command line arguments:

`LiveDiff-1.0.0.exe`

LiveDiff has a variety of command line arguments to provide additional functionality when performing system-level reverse engineering. This functionality and the associated command line arguments are discussed in the following subsections.

#### Dynamic Blacklisting

Dynamic blacklisting is a novel technique used to filter irrelevant file system and Registry entries from differential analysis. Dynamic blacklisting works by performing a system snapshot before any differential analysis/reverse engineering is performed. The initial system snapshot is used to populate an in-memory blacklist of known file system and Registry paths (the full, or absolute, logical file system location). Any data files or Regitry values that are encountered are subjected to a blacklist lookup using the full path, if a match is found the entry is ignored. To include dynamic blacklisting to filter operating system files and Registry values when collecting snapshots use LiveDiff with the -d (for Dynamic blacklist) argument:

`LiveDiff-1.0.0.exe -d`

#### LiveDiff Best Practices

LiveDiff is designed to be run on a minimal operating system, preferably a system which has just had a fresh installation without any additional applications installed. This is prefereable as the number of file system entries and Registry entries will only increase with system usage. An increase in the number of entries to process will understandably also increase tool processing time (decreasing computational efficiency).

It is also recommended that LiveDiff be run in a virtualised environment. All testing and research using LiveDiff has been performed using Virtual Box (https://www.virtualbox.org/wiki/Downloads). Pre-installed virtual machines for a variety of Windows operating system versions are freely available from Microsoft (https://dev.windows.com/en-us/microsoft-edge/tools/vms/linux/) for both Windows and Linux platforms.

## LiveDiff Output Format: Application Profile XML (APXML)

Application Profile XML is a hybrid data abstraction which amalgamates the Digital Forensic XML (DFXML) and Registry XML (RegXML) forensic data abstractions. The overall structure of an APXML document displayed in the following code snippet:

```
<apxml>
  <metadata/>
  <creator/>
   <!-- DFXML FileObjects -->
   <!-- RegXML CellObjects -->
   <rusage/>
</apxml>
```

Basically, APXML documents store file system entries as DFXML FileObjects and Windows Registry entries as RegXML CellObjects. Additional metadata, creator and rusage XML elements are also stored in the application profile. The following XML snippet displays an example of a populated FileObject for the TrueCrypt Windows installer file:

```
  <fileobject delta:new_file="1">
    <filename>C:\Program Files\TrueCrypt\TrueCrypt Setup.exe</filename>
    <filename_norm>%PROGRAMFILES%/TrueCrypt/TrueCrypt Setup.exe</filename_norm>
    <basename>TrueCrypt Setup.exe</basename>
    <basename_norm>TrueCrypt Setup.exe</basename_norm>
    <filesize>3466248</filesize>
    <alloc_inode>1</alloc_inode>
    <alloc_name>1</alloc_name>
    <meta_type>1</meta_type>
    <hashdigest type="sha1">7689d038c76bd1df695d295c026961e50e4a62ea</hashdigest>
    <app_name>TrueCrypt</app_name>
    <app_state>install</app_state>
  </fileobject>
```

The following XML snippet displays an example of a populated CellObject for the TrueCrypt .tc file extension in the Windows Registry SOFTWARE hive file:

```
  <cellobject delta:new_cell="1">
    <cellpath>HKLM\SOFTWARE\Classes\.tc\(Default)</cellpath>
    <cellpath_norm>SOFTWARE\Classes\.tc\(Default)</cellpath_norm>
    <basename>(Default)</basename>
    <name_type>v</name_type>
    <alloc>1</alloc>
    <data_type>REG_SZ</data_type>
    <data>TrueCryptVolume</data>
    <data_raw>54 00 72 00 75 00 65 00 43 00 72 00 79 00 70 00 74 00 56 00 6F 00 6C 00 75 00 6D 00 65 00 00 00</data_raw>
    <app_name>TrueCrypt</app_name>
    <app_state>install</app_state>
  </cellobject>
```

## Supported Windows Versions

The following Windows operating system versions have been tested:

1. Microsoft Windows 7 Service Pack 1 (32-bit)
2. Microsoft Windows Vista Service Pack 1 (64-bit)

## License

LiveDiff is based on the Regshot project (http://sourceforge.net/projects/regshot/). The fileshot.c and regshot.c source code files were used as the foundation of the LiveDiff tool. LiveDiff implements the same GNU Lesser General Public License version 2.0 (LGPLv2). Please find the license details in the License.txt file distributed with this project.

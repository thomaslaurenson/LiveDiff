/*
LiveDiff: A portable system-level differencing tool
thomas@thomaslaurenson.com
https://github.com/thomaslaurenson/LiveDiff

Copyright(C) 2018 Thomas Laurenson 

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.If not, see<http://www.gnu.org/licenses/>.
*/

using System;
using System.Text;
using System.Threading;
using System.Diagnostics;

using CommandLine;
using System.Collections.Generic;

namespace LiveDiff
{
    public class Options
    {
        [Option('t', "target", Required = true, HelpText = "Target directory to monitor.")]
        public string TargetDirectory { get; set; }

        [Option('v', "verbose", Default = 0, Required = false,
            HelpText = "Log file verbosity level. 0 = Info, 1 = Debug, 2 = Trace")]
        public int VerboseLevel { get; set; }
    }

    class LiveDiff
    {
        static void Main(string[] args)
        {
            Console.WriteLine(".____    .__             ________  .__  _____  _____ ");
            Console.WriteLine("|    |   |__|__  __ ____ \\______ \\ |__|/ ____\\/ ____\\");
            Console.WriteLine("|    |   |  \\  \\/ // __ \\ |    |  \\|  \\   __\\   __\\ ");
            Console.WriteLine("|    |___|  |\\   /\\  ___/ |    `   \\  ||  |   |  |   ");
            Console.WriteLine("|_______ \\__| \\_/  \\___  >_______  /__||__|   |__|   ");
            Console.WriteLine("        \\/             \\/        \\/                  ");
            Console.WriteLine("                                  By Thomas Laurenson");
            Console.WriteLine("                                  thomaslaurenson.com\n");

            string path = null;

            // Parse command line arguments
            Parser.Default.ParseArguments<Options>(args)
           .WithParsed<Options>(options =>
            {
                path = options.TargetDirectory;

            })
            .WithNotParsed<Options>(errors =>
            {
                Environment.Exit(1);
            });

            Console.WriteLine(">>> Processing: {0}", path.ToString());

            // SNAPSHOT ONE:

            Console.WriteLine(">>> Press any key to take Snapshot1...");
            Console.ReadKey();

            Stopwatch watch = new Stopwatch();
            watch.Start();
            
            // Enumerate the file system
            FileSystemSnapshot fsSnapshot1 = new FileSystemSnapshot(path);
            while (!fsSnapshot1.GetFileSystemSnapshot(path))
            {
                Thread.Sleep(1000);
            }
            
            watch.Stop();
            Console.WriteLine("  > Time elapsed: {0}", watch.Elapsed);

            Console.WriteLine("  > Dir count: {0}", fsSnapshot1.AllDirs.Count);
            Console.WriteLine("  > Fis count: {0}", fsSnapshot1.AllFiles.Count);

            //foreach (KeyValuePair<string, FileSystemEnumerator.FileInformation> kvp in FileSystemEnumerator.filesD)
            //{
            //    Console.WriteLine(kvp.Key);
            //    Console.WriteLine(kvp.Value.FileSize);
            //    Console.WriteLine();
            //}

            // SNAPSHOT TWO:

            Console.WriteLine(">>> Press any key to take Snapshot2...");
            Console.ReadKey();

            watch.Start();

            // Enumerate the file system
            FileSystemSnapshot fsSnapshot2 = new FileSystemSnapshot(path);
            while (!fsSnapshot2.GetFileSystemSnapshot(path))
            {
                Thread.Sleep(1000);
            }

            watch.Stop();
            Console.WriteLine("  > Time elapsed: {0}", watch.Elapsed);

            Console.WriteLine("  > Dir count: {0}", fsSnapshot2.AllDirs.Count);
            Console.WriteLine("  > Fis count: {0}", fsSnapshot2.AllFiles.Count);

            // Compared the two file system snapshots
            FileSystemSnapshot.CompareFileSystemSnapshots(fsSnapshot1, fsSnapshot2);
        }
    }
}

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
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace LiveDiff
{
    class LiveDiff
    {
        static void Main(string[] args)
        {
            Console.WriteLine(">>> FileSystemEnumerator starting...");

            // Start timer
            Stopwatch watch = new Stopwatch();
            watch.Start();

            string path = "C:\\Windows\\";

            while (!FileSystemEnumerator.FindNextFilePInvokeRecursiveParalleled(path))
            {
                // You can assume for all intents and purposes that drive C does exist and that you have access to it
                // which will cause this sleep to not get called.
                Thread.Sleep(1000);
            }
            watch.Stop();

            Console.WriteLine(">>> FileSystemEnumerator finished.");
        }
    }
}

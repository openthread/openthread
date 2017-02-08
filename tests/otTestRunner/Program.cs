/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace otTestRunner
{
    class Program
    {
        struct TestResults
        {
            public bool Pass;
            public List<string> Output;
            public string Error;
        }

        static string EscapeJson(string data)
        {
            return data.Replace("\\", "\\\\").Replace("\"", "\\\"");
        }

        /// <summary>
        /// Executes an exe with the given args and captures the output
        /// </summary>
        static async Task<TestResults> ExecuteAsync(string name, string args = null, int timeoutMilliseconds = -1, int instanceIndex = -1)
        {
            ProcessStartInfo startInfo = new ProcessStartInfo();
            startInfo.WindowStyle = ProcessWindowStyle.Hidden;
            startInfo.CreateNoWindow = true;
            startInfo.UseShellExecute = false;
            startInfo.RedirectStandardError = true;
            startInfo.RedirectStandardOutput = true;
            startInfo.FileName = name;
            startInfo.Arguments = args;

            startInfo.EnvironmentVariables["NODE_TYPE"] = "win-sim";

            if (instanceIndex != -1)
            {
                startInfo.EnvironmentVariables["INSTANCE"] = instanceIndex.ToString();
            }

            TestResults Results = new TestResults();
            Results.Pass = false;
            Results.Output = new List<string>();
            Results.Error = null;
            var Errors = new List<string>();

            Results.Output.Add(string.Format("> set NODE_TYPE=win-sim"));
            Results.Output.Add(string.Format("> {0} {1}", name, args));
            Results.Output.Add("----------------------------------------------------------------------");

            try
            {
                // Execute process
                using (Process process = Process.Start(startInfo))
                {
                    process.OutputDataReceived +=
                        (object sender, DataReceivedEventArgs e) => {
                            if (e.Data != null && e.Data.Length > 0)
                                lock (Results.Output) { Results.Output.Add(e.Data); }
                        };
                    process.ErrorDataReceived +=
                        (object sender, DataReceivedEventArgs e) => {
                            if (e.Data != null && e.Data.Length > 0)
                                lock (Results.Output) { Errors.Add(e.Data); }
                        };

                    process.BeginErrorReadLine();
                    process.BeginOutputReadLine();

#if DEBUG
                    Console.WriteLine("Starting {0} {1}", name, args);
#endif

                    // Wait for process to complete
                    await Task.Run(
                        () => {
                            if (timeoutMilliseconds == -1)
                                process.WaitForExit();
                            else if (!process.WaitForExit(timeoutMilliseconds))
                            {
                                process.Kill();
                                Results.Output.Add(string.Format("Killed {0} on execution timeout!", name));
                            }
                        });

                    // Wait a bit for any output to collect
                    await Task.Delay(1000);

                    process.CancelOutputRead();
                    process.CancelErrorRead();

                    Results.Output.AddRange(Errors);
                    Results.Pass = Errors.Count > 0 && Errors[Errors.Count - 1] == "OK";

                    if (!Results.Pass) Results.Error = EscapeJson(string.Join("\\r\\n", Errors));

                    // Make sure the process is killed
                    try { process.Kill(); } catch (Exception) { }

#if DEBUG
                    Console.WriteLine("Completed {0} {1}", name, args);
#endif
                }
            }
            catch (Exception e)
            {
                Results.Output.Add("Encountered exception: " + e.Message);
                Results.Output.Add(e.StackTrace);
            }

            return Results;
        }

        static bool VerboseOutput = false;
        static int RetiresOnFailure = 0;
        static bool AppVeyorMode = false;
        static string AppVeyorApiUrl = null;
        static string ResultsFolder = "Results_" + DateTime.Now.ToString("yyyyMMdd_HH.mm.ss");

        static void UploadAppVeyorTestResult(string name, bool passed, long durationMS, string error = null)
        {
            if (AppVeyorApiUrl == null) return;

            string jsonData = null;

            try
            {
                var request = (HttpWebRequest)WebRequest.Create(Path.Combine(AppVeyorApiUrl, "api/tests"));

                jsonData =
                    string.Format(
                        "{{" +
                            "\"testName\": \"{0}\", " +
                            "\"testFramework\": \"MSTest\", " +
                            "\"fileName\": \"{0}.py\", " +
                            "\"outcome\": \"{1}\", " +
                            "\"durationMilliseconds\": \"{2}\", " +
                            "\"ErrorMessage\": \"{3}\"" +
                        "}}",
                        name,
                        passed ? "Passed" : "Failed",
                        durationMS,
                        error == null ? "" : error
                        );

                var data = Encoding.UTF8.GetBytes(jsonData);

                request.Method = "POST";
                request.ContentType = "application/json";
                request.ContentLength = data.Length;

                using (var stream = request.GetRequestStream())
                {
                    stream.Write(data, 0, data.Length);
                }

                var response = (HttpWebResponse)request.GetResponse();
                var responseString = new StreamReader(response.GetResponseStream()).ReadToEnd();
            }
            catch (Exception e)
            {
                Console.WriteLine("Encountered exception for http post:");
                Console.WriteLine(e.Message);
                Console.WriteLine(e.StackTrace);

                Console.WriteLine("Json content:");
                Console.WriteLine(jsonData);
            }
        }

        enum TestResult
        {
            Fail,
            Pass,
            PassWithRetry
        }

        /// <summary>
        /// Runs a python test file and returns success/failure
        /// </summary>
        static async Task<TestResult> RunTest(string file, int index)
        {
            string pythonPath = "python.exe";
            if (AppVeyorMode)
            {
                if (Environment.GetEnvironmentVariable("Platform").ToLower() == "x64")
                {
                    pythonPath = @"c:\python35-x64\python.exe";
                }
                else
                {
                    pythonPath = @"c:\python35\python.exe";
                }
            }

            int tries = 0;
            Stopwatch Timer;
            TestResults Results;

            do
            {
                Timer = new Stopwatch();
                Timer.Start();

                Results = await ExecuteAsync(pythonPath, file, 30 * 60 * 1000, index);

                Timer.Stop();

            } while (++tries < RetiresOnFailure + 1 && Results.Pass == false);

            if (VerboseOutput)
            {
                lock (ResultsFolder)
                {
                    foreach (var line in Results.Output)
                        Console.WriteLine(line);
                }
            }

            UploadAppVeyorTestResult(Path.GetFileNameWithoutExtension(file), Results.Pass, Timer.ElapsedMilliseconds, Results.Error);

            // Write the output to a file
            var filePrefix = Results.Pass ? "P_" : "F_";
            var outputFilePath = Path.Combine(ResultsFolder, filePrefix + Path.GetFileNameWithoutExtension(file) + ".txt");
            try {
                File.WriteAllLines(outputFilePath, Results.Output);
            } catch (Exception e) { 
                Console.WriteLine("Exception while trying to write {0}:\n{1}!", outputFilePath, e.Message);
            }

            return Results.Pass ?
                (tries == 1 ? TestResult.Pass : TestResult.PassWithRetry) :
                TestResult.Fail;
        }

        /// <summary>
        /// Runs all the tests as indicated by input arguments
        /// </summary>
        static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("Usage: otTestRunner.exe [path] [search pattern] (parallel:n) (verbose)");
                return;
            }

            var files = Directory.GetFiles(args[0], args[1]);
            if (files.Length == 0)
            {
                Console.WriteLine("No tests found with that path & pattern!");
                return;
            }

            var NumberOfTestsToRunInParallel = 1;
            for (var i = 2; i < args.Length; i++)
            {
                if (args[i].StartsWith("parallel:"))
                    NumberOfTestsToRunInParallel = int.Parse(args[i].Substring(9));
                else if (args[i].StartsWith("retry:"))
                    RetiresOnFailure = int.Parse(args[i].Substring(6));
                else if (args[i].StartsWith("verbose"))
                    VerboseOutput = true;
                else if (args[i].StartsWith("appveyor"))
                {
                    AppVeyorMode = true;
                    AppVeyorApiUrl = Environment.GetEnvironmentVariable("APPVEYOR_API_URL");
                    //Console.WriteLine("AppVeyorApiUrl = {0}", AppVeyorApiUrl);
                }
            }

            var CurNumTestsRunning = 0;
            var ReadyToRunEvent = new ManualResetEvent(true);

            var TestPassCount = 0;
            Stopwatch Timer = new Stopwatch();

            Directory.CreateDirectory(ResultsFolder);
            Console.WriteLine("Test results saved: .\\{0}", ResultsFolder);

            Console.WriteLine("Running {0} tests, {1} at a time:", files.Length, NumberOfTestsToRunInParallel);
            /*for (var i = 0; i < files.Length; i++)
                Console.WriteLine(Path.GetFileName(files[i]));*/
            Console.WriteLine("");

            Timer.Start();
            for (var i = 0; i < files.Length; i++)
            {
                // Wait for the event to be set, if not already
                ReadyToRunEvent.WaitOne();

                if (i != 0)
                {
                    // Wait a bit to stagger the starts
                    Task.Delay(1000).Wait();
                }

                lock (ResultsFolder)
                {
                    if (++CurNumTestsRunning == NumberOfTestsToRunInParallel)
                        ReadyToRunEvent.Reset();
                }

                var index = i;
                var fileName = files[i];

                // Start the test, but don't wait for it
                Task.Run(
                    async () =>
                    {
                        var result = await RunTest(fileName, index);
                        lock (ResultsFolder)
                        {
                            var PrevColor = Console.ForegroundColor;
                            if (result != TestResult.Fail)
                            {
                                if (result == TestResult.Pass)
                                {
                                    Console.ForegroundColor = ConsoleColor.Green;
                                }
                                else
                                {
                                    Console.ForegroundColor = ConsoleColor.Yellow;
                                }
                                Console.Write("PASS");
                                TestPassCount++;
                            }
                            else
                            {
                                Console.ForegroundColor = ConsoleColor.Red;
                                Console.Write("FAIL");
                            }
                            Console.ForegroundColor = PrevColor;
                            Console.WriteLine(": {0}", Path.GetFileNameWithoutExtension(fileName));
                            
                            CurNumTestsRunning--;
                            ReadyToRunEvent.Set();
                        }
                    });
            }

            // Wait for all the tests to complete
            while (CurNumTestsRunning != 0)
                ReadyToRunEvent.WaitOne();

            Timer.Stop();
            TimeSpan ts = Timer.Elapsed;
            string elapsedTime = 
                String.Format("{0:00}:{1:00}:{2:00}.{3:00}",
                              ts.Hours, ts.Minutes, ts.Seconds, ts.Milliseconds / 10);

            Console.WriteLine("{0} tests run in {1}", files.Length, elapsedTime);
            Console.WriteLine("{0} passed and {1} failed", TestPassCount, files.Length - TestPassCount);

            if (!AppVeyorMode)
                Environment.ExitCode = files.Length == TestPassCount ? 0 : 1;
        }
    }
}

# Orpy HTTP Server

Implementation of a HTTP server in C++17 for Windows and Linux.
Updated code based on last complete version of Orpy.

## Features

- Handles multiple concurrent connections, tested up to 10k.
- Supports basic HTTP request and response GET and POST.
- Provides HTTP/1.1 support with a simple state machine parser.
- Includes a non-blocking file server.

## Quick start

```bash
mkdir build && cd build
cmake ..
```

Linux
```bash
make install
```

Use Visual Studio 2022 on Windows or Visual Studio Code on Linux.

## Design

The server program consists of:

- 1 main thread for quit command.
- 1 listener thread to accept incoming clients.
- N worker threads to process HTTP requests and sends response back to client based on CPU Threads.

## Config

At first start Orpy will run a wizard helping to configure all folder and files required.

If the URL is not specified in the "Configs/sites/domain.json" file, the server will respond with a 444 error. 

To set up the server, create a "Data" folder where you can place individual folders for each URL.
Create a file named "main.style" in the "Data/localhost/style" directory and add your desired HTML code for content.

The "urls" field in the configuration is used to add or remove parts of the website or allow specific files.

To create a minimal "localhost.json" configuration file for a server to work, you can include the following fields:
```json
{
  "path": "localhost/",
  "urls": [ "", "myfile.txt" ]
}
```

## Benchmark

I used a tool called [wrk](https://github.com/wg/wrk) to benchmark this HTTP server v1. 

My Specs:
```bash
OS: Windows 10 / Ubuntu 22.04 Kernel 6.0.0
CPU: AMD 2700x 8 Cores / 16 Threads
GPU: AMD 6900 XT
Memory: 64 Gb
```

Here are the results for two test runs. Each test ran for 1 minute, with 10 client threads. The first test had only 500 concurrent connections, while the second test had 10000.

## WINDOWS

```bash
$ wrk -t10 -c500 -d60s http://localhost:8080
Running 1m test @ http://localhost:8080
  10 threads and 500 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     4.38ms  844.50us  47.08ms   76.27%
    Req/Sec    11.02k     0.89k   14.56k    74.87%
  6581408 requests in 1.00m, 1.43GB read
Requests/sec: 109606.59
Transfer/sec:     24.36MB
```

```bash
$ wrk -t10 -c10000 -d60s http://localhost:8080
Running 1m test @ http://localhost:8080
  10 threads and 10000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     7.83ms    1.58ms  47.21ms   80.25%
    Req/Sec    12.25k     3.74k   21.86k    68.19%
  7302204 requests in 1.00m, 1.58GB read
Requests/sec: 121515.92
Transfer/sec:     27.00MB
```

## Linux

```bash
$ wrk -t10 -c500 -d60s http://localhost:8080
Running 1m test @ http://localhost:8080
  10 threads and 500 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     1.23ms  598.56us  52.30ms   96.33%
    Req/Sec    40.94k     3.03k   84.15k    74.20%
  24449376 requests in 1.00m, 5.31GB read
Requests/sec: 406961.05
Transfer/sec:     90.43MB
```

```bash
$ wrk -t10 -c10000 -d60s http://localhost:8080
Running 1m test @ http://localhost:8080
  10 threads and 10000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     2.86ms    0.98ms  41.03ms   89.98%
    Req/Sec    32.54k    21.26k   94.45k    68.76%
  19430254 requests in 1.00m, 4.22GB read
Requests/sec: 323344.07
Transfer/sec:     71.85MB
```

## Credits
Original code is inspired by [trungams's http-server](https://github.com/trungams/http-server)

# Sauer Proxy

This program acts as a generic proxy server for Sauerbraten servers with extra features.

Please note that this program is still **experimental** and **unstable**. Use at your own risk.

## Features

* Relay all ENet packets at supplied port between clients and a remote host.
* Extinfo relay at port +1 with caching.
* Optional delay for packets in the server->client direction.
* Master server registration.
* Player ping reply (pong).
* Real IP address forwarding (see below).

## Building

You will need CMake, a C compiler, and a C++ compiler.

```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Usage

```
Usage:
    ./SauerProxy [options] remote_host

Sauerbraten proxy server.

Positional arguments:
  remote_host   IP address of remote server

Optional arguments:
  -h        Show this help message and exit
  -p        Port on which to listen (default: 28785)
  -r        Port of remote server (default: 28785)
  -d        Delay server->client packets by this many milliseconds (default: 0)
  -o        Add this many milliseconds to the pings of all connected players
            (default: 0)
  -m        Register this server with the master server (default: false)
  -f        Forward real player IP addresses to server (requires compatible
            server mod, default: false)
```

## Real IP Address Forwarding

To enable this feature, you must use a compatible Sauerbraten server such as the [Sauerduels Sauerbraten Server](https://github.com/sauerduels/sauer-server). This allows display of a player's real country name instead of the proxy's, and prevents banning the proxy server.

In the server's configuration file (*server-init.cfg*), for every unique proxy server host you wish to use, add the following line:

```
addtrustedhost x.x.x.x
```

Where `x.x.x.x` is the IP address of the proxy server.

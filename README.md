# Sauer Proxy

This program acts as a generic proxy server for Sauerbraten servers with extra features.

Please note that this program is still **experimental** and **unstable**. Use at your own risk.

## Features

* Relay all ENet packets at supplied port between clients and a remote host.
* Relay all UDP packets at supplied port +1 for extinfo.
* Optional delay for most packet types in the server->client direction.
* Master server registration.
* Player ping reply (pong).
* Real IP address forwarding (see below).

## Building

1. Install Rust and Clang.
2. Run `cargo build` or `cargo build --release`
3. Start the executable as `./target/debug/sauer-proxy` or `./target/release/sauer-proxy`.

## Usage

```
Usage:
    ./sauer-proxy [OPTIONS] [REMOTE_HOST]

Sauerbraten proxy server.

positional arguments:
  remote_host           IP address of remote server

optional arguments:
  -h,--help             show this help message and exit
  -p,--port PORT        port on which to listen (default: 28785)
  -r,--remote-port REMOTE_PORT
                        port of remote server (default: 28785)
  -d,--delay DELAY      delay server->client packets by this many milliseconds
                        (default: 5000)
  -m,--register-master  register this server with the master server (default:
                        false)
```

## Real IP Address Forwarding

To enable this feature, you must use a compatible Sauerbraten server such as the [Sauerduels Sauerbraten Server](https://github.com/sauerduels/sauer-server). This allows display of a player's real country name instead of the proxy's, and prevents banning the proxy server.

In the server's configuration file (*server-init.cfg*), for every unique proxy server host you wish to use, add the following line:

```
addtrustedhost x.x.x.x
```

Where `x.x.x.x` is the IP address of the proxy server.

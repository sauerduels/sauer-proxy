extern crate argparse;
extern crate enet_sys;

use argparse::{ArgumentParser, Store, StoreTrue};

mod server;
mod client;
mod master;
mod enet_server;
mod enet_client;

fn main() {
    let mut port = 28785;
    let mut delay = 5000;
    let mut remote_port = 28785;
    let mut remote_host: String = String::from("");
    let mut register_master = false;
    let mut forward_ips = false;
    {
        let mut ap = ArgumentParser::new();
        ap.set_description("Sauerbraten proxy server.");
        ap.refer(&mut port)
            .add_option(&["-p", "--port"], Store,
            "port on which to listen (default: 28785)");
        ap.refer(&mut remote_port)
            .add_option(&["-r", "--remote-port"], Store,
            "port of remote server (default: 28785)");
        ap.refer(&mut delay)
            .add_option(&["-d", "--delay"], Store,
            "delay server->client packets by this many milliseconds (default: 5000)");
        ap.refer(&mut remote_host)
            .add_argument("remote_host", Store,
            "IP address of remote server");
        ap.refer(&mut register_master)
            .add_option(&["-m", "--register-master"], StoreTrue,
            "register this server with the master server (default: false)");
        ap.refer(&mut forward_ips)
            .add_option(&["-f", "--forward-ips"], StoreTrue,
            "forward real player IP addresses to server (requires compatible server mod, default: false)");
        ap.parse_args_or_exit();
    }

    if remote_host.len() == 0 {
        println!("Error: remote_host argument is required");
        return;
    }

    // let host_addr = format!("0.0.0.0:{}", port);
    // let remote_addr = format!("{}:{}", remote_host, remote_port);
    // let server = server::Server::new(host_addr.as_str(), remote_addr.as_str(), delay);
    
    enet_server::initialize();
    let server = enet_server::ENetServer::create(port, remote_host.as_str().into(), remote_port, delay, forward_ips);

    let ext_host_addr = format!("0.0.0.0:{}", port+1);
    let ext_remote_addr = format!("{}:{}", remote_host, remote_port+1);
    let ext_server = server::Server::new(ext_host_addr.as_str(), ext_remote_addr.as_str(), 0, 0);

    if register_master {
        master::register_master(port);
    }

    server.join().unwrap();
    ext_server.join().unwrap();
    enet_server::deinitialize();
}

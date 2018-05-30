#![allow(non_upper_case_globals)]

use enet_sys::{enet_initialize, enet_deinitialize};
use enet_sys::{ENetHost, ENetAddress, ENetEvent, ENetPeer, ENetPacket};
use enet_sys::{_ENetEventType_ENET_EVENT_TYPE_CONNECT, _ENetEventType_ENET_EVENT_TYPE_RECEIVE, _ENetEventType_ENET_EVENT_TYPE_DISCONNECT, _ENetEventType_ENET_EVENT_TYPE_NONE};
use enet_sys::{enet_host_create, enet_host_check_events, enet_host_service, enet_packet_destroy};

use std::time::{Duration, SystemTime};
use std::thread;
use std::collections::{HashMap};
use std::collections::hash_map::Entry;
use std::net::{SocketAddr, IpAddr, Ipv4Addr};

use enet_client;

pub fn initialize() {
    if unsafe {enet_initialize()} < 0 {
        panic!("Error initializing enet");
    }
}

pub fn deinitialize() {
    unsafe {enet_deinitialize();}
}

pub struct ENetServer {
    server_host: *mut ENetHost,
    remote_host: u32,
    remote_port: u16,
    delay: u64,
    forward_ips: bool,
    clients: HashMap<SocketAddr, enet_client::ENetClient>,
}

fn addr_from_enet_address(address: &ENetAddress) -> SocketAddr {
    SocketAddr::new(IpAddr::from(Ipv4Addr::from(address.host)), address.port)
}

impl ENetServer {
    pub fn create(port: u16, remote_host: String, remote_port: u16, delay: u64, forward_ips: bool) -> thread::JoinHandle<()> {
        thread::spawn(move || {
            let address = ENetAddress {
                host: 0,
                port: port
            };
            let server_host: *mut ENetHost;
            unsafe {
                server_host = enet_host_create (&address, 128, 3, 0, 0);
                if server_host.is_null() {
                    panic!("Could not create server host")
                }
                println!("Server host listening on port {}", port);
                (*server_host).duplicatePeers = 128;
            }
            let ip_addr: Ipv4Addr = remote_host.parse().unwrap();
            let ip_octets = ip_addr.octets();
            let ip_u32: u32 = ((ip_octets[3] as u32)<<24) + ((ip_octets[2] as u32)<<16) + ((ip_octets[1] as u32)<<8) + (ip_octets[0] as u32);
            let mut server = ENetServer {
                server_host,
                remote_host: ip_u32,
                remote_port,
                delay,
                forward_ips,
                clients: HashMap::new(),
            };
            loop {
                server.slice();
                thread::sleep(Duration::from_millis(1));
            };
        })
    }
    
    pub fn slice(&mut self) {
        let mut event: ENetEvent = ENetEvent {
            channelID: 0,
            data: 0,
            type_: _ENetEventType_ENET_EVENT_TYPE_NONE,
            peer: 0 as *mut ENetPeer,
            packet: 0 as *mut ENetPacket,
        };
        unsafe {
            loop {
                if enet_host_check_events(self.server_host, &mut event) <= 0 {
                    if enet_host_service(self.server_host, &mut event, 5) <= 0 {
                    }
                }
                match event.type_ {
                    _ENetEventType_ENET_EVENT_TYPE_CONNECT => {
                        let addr = addr_from_enet_address(&(*event.peer).address);
                        match self.clients.entry(addr) {
                            Entry::Occupied(o) => o.into_mut(),
                            Entry::Vacant(v) => v.insert(enet_client::ENetClient::new(event.peer, self.remote_host, self.remote_port, self.forward_ips))
                        };
                        println!("Client connected ({})", addr.ip());
                    },
                    _ENetEventType_ENET_EVENT_TYPE_RECEIVE => {
                        let addr = addr_from_enet_address(&(*event.peer).address);
                        match self.clients.get_mut(&addr) {
                            Some(client) => {
                                client.handle_incoming(event.channelID, event.packet);
                            },
                            None => {}
                        };
                        if (*event.packet).referenceCount==0 {
                            enet_packet_destroy(event.packet);
                        }
                    },
                    _ENetEventType_ENET_EVENT_TYPE_DISCONNECT => {
                        let addr = addr_from_enet_address(&(*event.peer).address);
                        match self.clients.get_mut(&addr) {
                            Some(client) => {
                                client.disconnect();
                            },
                            None => {}
                        };
                        self.clients.remove(&addr);
                        println!("Client disconnected ({})", addr.ip());
                    },
                    _ => {}
                }
                let time = SystemTime::now();
                for (_, client) in &mut self.clients {
                    client.slice(time, self.delay);
                }
            }
            // enet_host_destroy(self.server_host);
        }
    }
}

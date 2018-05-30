#![allow(non_upper_case_globals)]

use enet_sys::{ENetHost, ENetAddress, ENetEvent, ENetPeer, ENetPacket};
use enet_sys::{_ENetEventType_ENET_EVENT_TYPE_CONNECT, _ENetEventType_ENET_EVENT_TYPE_RECEIVE, _ENetEventType_ENET_EVENT_TYPE_DISCONNECT, _ENetEventType_ENET_EVENT_TYPE_NONE};
use enet_sys::{enet_host_create, enet_host_service, enet_packet_destroy, enet_host_connect, enet_host_flush, enet_peer_disconnect, enet_peer_throttle_configure, enet_host_bandwidth_limit, enet_peer_send, enet_packet_create};

use std::collections::{VecDeque};
use std::time::{Duration, SystemTime};
use std::os::raw::c_void;

struct ENetMessage {
    time: SystemTime,
    chan: u8,
    packet: *mut ENetPacket,
    pass_through: bool,
}

pub struct ENetClient {
    client_peer: *mut ENetPeer,
    client_host: *mut ENetHost,
    conn_peer: *mut ENetPeer,
    pub connected: bool,
    forward_ip: bool,
    queue: VecDeque<ENetMessage>,
}

impl ENetClient {
    pub fn new(client_peer: *mut ENetPeer, remote_host: u32, remote_port: u16, forward_ip: bool) -> ENetClient {
        let addr = ENetAddress {
            host: remote_host,
            port: remote_port,
        };
        unsafe {
            let client_host = enet_host_create(0 as *const ENetAddress, 2, 3, 0, 0);
            if client_host == 0 as *mut ENetHost {
                panic!("Could not create client host");
            }
            let conn_peer = enet_host_connect(client_host, &addr, 3, 0); 
            enet_host_flush(client_host);
            ENetClient {
                client_peer,
                client_host,
                conn_peer,
                connected: false,
                forward_ip,
                queue: VecDeque::new(),
            }
        }
    }
    
    pub fn handle_incoming(&mut self, chan: u8, packet: *mut ENetPacket) {
        unsafe {
            if self.connected && packet != 0 as *mut ENetPacket && (*packet).data != 0 as *mut u8 {
                match *(*packet).data {
                    30 => { // N_PING
                        *(*packet).data = 31;
                        enet_peer_send(self.client_peer, chan, packet);
                    },
                    110 => {}, // N_SERVCMD
                    _ => { enet_peer_send(self.conn_peer, chan, packet); }
                }
            }
        }
    }
    
    pub fn slice(&mut self, time: SystemTime, delay: u64) {
        let mut event: ENetEvent = ENetEvent {
            channelID: 0,
            data: 0,
            type_: _ENetEventType_ENET_EVENT_TYPE_NONE,
            peer: 0 as *mut ENetPeer,
            packet: 0 as *mut ENetPacket,
        };
        unsafe {
            while enet_host_service(self.client_host, &mut event, 0) > 0 {
                match event.type_ {
                    _ENetEventType_ENET_EVENT_TYPE_CONNECT => {
                        self.connected = true;
                        enet_peer_throttle_configure(self.conn_peer, 5000, 2, 2);
                        enet_host_bandwidth_limit(self.client_host, 0*1024, 0*1024);
                        
                        if self.forward_ip {
                            let ip = (*self.client_peer).address.host;
                            // N_SERVMSG setip xxxx
                            let buf: [u8; 12] = [110, 115, 101, 116, 105, 112, 0, 0x81, ip as u8, (ip>>8) as u8, (ip>>16) as u8, (ip>>24) as u8];
                            let mut packet = enet_packet_create(buf.as_ptr() as *const c_void, 12, 1);
                            (*packet).flags = 1;
                            enet_peer_send(self.conn_peer, 1, packet);
                            if (*packet).referenceCount==0 {
                                enet_packet_destroy(packet);
                            }
                        }
                    },
                    _ENetEventType_ENET_EVENT_TYPE_RECEIVE => {
                        if event.packet != 0 as *mut ENetPacket && (*event.packet).data != 0 as *mut u8 {
                            let msg_type = *(*event.packet).data;
                            let pass_through = msg_type == 1 ||
                                               msg_type == 2 ||
                                               msg_type == 35 ||
                                               msg_type == 36 ||
                                               msg_type == 37 ||
                                               msg_type == 25;
                            let msg = ENetMessage {
                                time: time.clone(),
                                chan: event.channelID,
                                packet: event.packet,
                                pass_through,
                            };
                            self.queue.push_back(msg);
                        }
                    },
                    _ENetEventType_ENET_EVENT_TYPE_DISCONNECT => {
                        enet_peer_disconnect(self.client_peer, event.data);
                        self.connected = false;
                    },
                    _ => {},
                }
            }
            let delay: Duration = Duration::from_millis(delay);
            if self.connected {
                while {
                    let front = self.queue.front();
                    !front.is_none() &&
                    (
                        front.unwrap().pass_through ||
                        time.duration_since(front.unwrap().time).unwrap() >= delay
                    )
                } {
                    let msg = self.queue.pop_front().unwrap();
                    enet_peer_send(self.client_peer, msg.chan, msg.packet);
                    if (*msg.packet).referenceCount==0 {
                        enet_packet_destroy(msg.packet);
                    }
                }
            }
        }
    }
    
    pub fn disconnect(&mut self) {
        unsafe {
            if self.connected {
                enet_peer_disconnect(self.conn_peer, 0);
                enet_host_flush(self.client_host);
            }
            self.connected = false;
        }
    }
}

use std::io;
use std::thread;
use std::collections::{HashMap};
use std::collections::hash_map::Entry;
use std::net::{UdpSocket, SocketAddr};
use std::time::{Duration, SystemTime};

use client::Client;

pub struct Server {
    clients: HashMap<SocketAddr, Client>,
    socket: UdpSocket,
    remote_addr: String,
    delay: u64,
    grace: u64
}

impl Server {
    pub fn new(addr: &str, remote_addr: &str, delay: u64, grace: u64) -> thread::JoinHandle<()> {
        let mut server = Server {
            clients: HashMap::new(),
            socket: UdpSocket::bind(addr).unwrap(),
            remote_addr: remote_addr.into(),
            delay: delay,
            grace: grace
        };
        server.socket.set_nonblocking(true).unwrap();
        println!("Server listening on {}", addr);
        thread::spawn(move || {
            let mut buf = [0; 2048];
            loop {
                server.slice(&mut buf);
                thread::sleep(Duration::from_millis(1));
            };
        })
    }
    
    pub fn slice(&mut self, buf: &mut [u8]) {
        match self.socket.recv_from(&mut *buf) {
            Ok((len, src)) => {
                let client: &Client = match self.clients.entry(src) {
                    Entry::Occupied(o) => o.into_mut(),
                    Entry::Vacant(v) => v.insert(Client::new(src, &self.remote_addr))
                };
                client.handle_incoming(&mut *buf, len);
            },
            Err(ref e) if e.kind() == io::ErrorKind::WouldBlock => {}
            Err(e) => println!("Encountered IO error: {}", e),
        }
        
        let time = SystemTime::now();
        for (_, client) in &mut self.clients {
            match client.slice(buf, time, self.delay, self.grace) {
                Some(len) => {
                    self.socket.send_to(&buf[0..len], client.addr).is_ok();
                },
                None => {}
            };
        }
        self.clean_up(time);
    }
    
    pub fn clean_up(&mut self, time: SystemTime) {
        self.clients.retain(|_, client| {
            !client.check_timeout(time)
        });
    }
}

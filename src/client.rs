use std::collections::{VecDeque};
use std::net::{UdpSocket, SocketAddr};
use std::time::{Duration, SystemTime};

struct Message {
    time: SystemTime,
    buf: Vec<u8>
}

pub struct Client {
    socket: UdpSocket,
    pub addr: SocketAddr,
    connect_time: SystemTime,
    last_reply: SystemTime,
    queue: VecDeque<Message>
}

impl Client {
    pub fn new(addr: SocketAddr, remote_addr: &str) -> Client {
        let time = SystemTime::now();
        let client = Client {
            socket: UdpSocket::bind("0.0.0.0:0").unwrap(),
            addr: addr,
            connect_time: time,
            last_reply: time,
            queue: VecDeque::new()
        };
        client.socket.connect(remote_addr).unwrap();
        client.socket.set_nonblocking(true).unwrap();
        client
    }
    
    pub fn handle_incoming(&self, buf: &mut [u8], len: usize) {
        self.socket.send(&buf[0..len]).unwrap();
    }
    
    pub fn slice(&mut self, buf: &mut [u8], time: SystemTime, delay: u64, grace: u64) -> Option<usize> {
        let delay: Duration = Duration::from_millis(delay);
        let grace: Duration = Duration::from_millis(grace);
        match self.socket.recv_from(&mut *buf) {
            Ok((amt, _)) => {
                self.last_reply = time.clone();
                let msg = Message {
                    time: time.clone(),
                    buf: buf[0..amt].to_vec()
                };
                self.queue.push_back(msg);
            },
            Err(_) => {}
        };
        let mut len: usize = 0;
        match self.queue.front() {
            Some(msg) if time.duration_since(msg.time).unwrap() >= delay ||
                         time.duration_since(self.connect_time).unwrap() < grace => {
                buf[0..msg.buf.len()].clone_from_slice(msg.buf.as_slice());
                len = msg.buf.len();
            },
            Some(_) => {}
            None => {}
        }
        if len > 0 {
            self.queue.pop_front();
            Some(len)
        } else {
            None
        }
    }
    
    pub fn check_timeout(&self, time: SystemTime) -> bool {
        let timeout: Duration = Duration::from_secs(30);
        match time.duration_since(self.last_reply) {
            Ok(duration) => { duration >= timeout },
            Err(_) => true
        }
    }
}

use std::io::prelude::*;
use std::net::TcpStream;
use std::str::from_utf8;
use std::thread;
use std::time::Duration;

pub fn register_master(port: u16) {
    thread::spawn(move || {
        loop {
            match TcpStream::connect("37.59.116.203:28787") {
                Ok(mut stream) => {
                    let _ = stream.write(format!("regserv {}\n", port).as_bytes());
                    let mut res = [0; 128];
                    let len = stream.read(&mut res).unwrap();
                    let response = from_utf8(&res[..len]).unwrap();
                    if response.starts_with("failreg") {
                        println!("Master server regitration failed: {}", &response[8..]);
                    } else if response.starts_with("succreg") {
                        println!("Master server registration succeeded");
                    }
                    thread::sleep(Duration::from_secs(60*60));
                },
                Err(e) => {
                    println!("Error connecting to master server: {}", e);
                }
            }
        };
    });
}

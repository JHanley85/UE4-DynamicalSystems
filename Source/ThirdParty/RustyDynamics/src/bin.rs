extern crate RustyDynamics;

use RustyDynamics::*;

fn main() {

    println!("RustyBin says hi!");

    println!("uuid: {}", rb_uuid());

    let local_addr = "192.168.0.102:0".to_string();
    let server_addr = "127.0.0.1:8080".to_string();
    let mumble_addr = "127.0.0.1:64738".to_string();

    netclient_open(local_addr, server_addr, mumble_addr, "".to_string());

    std::thread::park();
}

pub fn main2() {
    mumblebot::cmd();
}

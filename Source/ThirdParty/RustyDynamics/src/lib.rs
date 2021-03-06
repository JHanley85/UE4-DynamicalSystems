extern crate tokio_core;
//extern crate tokio_timer;
extern crate futures;
extern crate uuid;
#[macro_use]
extern crate serde_derive;
extern crate bincode;
extern crate mumblebot;

use bincode::{serialize, deserialize, Infinite};

use std::io;
use std::net::{IpAddr, Ipv4Addr, SocketAddr};
use std::thread;
use std::sync::{Arc, Mutex};
use std::collections::VecDeque;

use futures::{Future, Stream, Sink};
use futures::future::{ok, err};
use tokio_core::net::{UdpSocket, UdpCodec};
use tokio_core::reactor::Core;

use std::ffi::CStr;
use std::os::raw::c_char;

use uuid::Uuid;

mod rnd;

pub struct LineCodec;

extern fn null_log(_log: *const std::os::raw::c_char) {
    ()
}

static mut C_LOG: extern fn(log: *const c_char) = null_log;

#[no_mangle]
pub extern fn rb_log_fn(log_fn: extern fn(log: *const c_char)) {
    unsafe { C_LOG = log_fn; }
}

fn log(log: std::string::String) {
    let c_string = std::ffi::CString::new(log).unwrap();
    unsafe { C_LOG(c_string.as_ptr()) };
}

impl UdpCodec for LineCodec {
    type In = (SocketAddr, Vec<u8>);
    type Out = (SocketAddr, Vec<u8>);

    fn decode(&mut self, addr: &SocketAddr, buf: &[u8]) -> io::Result<Self::In> {
        Ok((*addr, buf.to_vec()))
    }

    fn encode(&mut self, (addr, buf): Self::Out, into: &mut Vec<u8>) -> SocketAddr {
        into.extend(buf);
        addr
    }
}

type SharedQueue<T> = std::sync::Arc<std::sync::Mutex<std::collections::VecDeque<T>>>;

pub struct Client {
    uuid: Uuid,
    sender_pubsub: futures::sink::Wait<futures::sync::mpsc::UnboundedSender<Vec<u8>>>,
    vox: futures::sink::Wait<futures::sync::mpsc::Sender<Vec<u8>>>,
    task: Option<std::thread::JoinHandle<()>>,
    msg_queue: SharedQueue<Vec<u8>>,
    vox_queue: SharedQueue<Vec<u8>>,
    kill: futures::sink::Wait<futures::sync::mpsc::Sender<()>>,
}

#[no_mangle]
pub fn rd_get_pow_2_of_int32(num: u32) -> u32 {
    num * num
}

#[no_mangle]
pub fn rd_netclient_msg_push(client: *mut Client, bytes: *const u8, count: u32) {
    unsafe {
        let count : usize = count as usize;
        let mut msg = Vec::with_capacity(count);
        for i in 0..count {
            msg.push(*bytes.offset(i as isize));
        }
        (*client).sender_pubsub.send(msg);
    }
}

#[no_mangle]
pub fn rd_netclient_msg_pop(client: *mut Client) -> *mut Vec<u8> {
    unsafe {
        let mut data : Vec<u8> = Vec::new();
        {
            if let Ok(mut locked_queue) = (*client).msg_queue.try_lock() {
                if let Some(m) = locked_queue.pop_front() {
                    data = m;
                }
            }
        }
        let data = Box::new(data);
        Box::into_raw(data)
    }
}

#[no_mangle]
pub fn rd_netclient_msg_drop(msg: *mut Vec<u8>) {
    unsafe { Box::from_raw(msg) };
}

#[no_mangle]
pub fn rd_netclient_uuid(client: *mut Client, uuid: *mut u8) {
    unsafe {
        let u = format!("{}\npedro", (*client).uuid);
        for (i, c) in u.chars().enumerate().take(36) {
            *uuid.offset(i as isize) = c as u8;
        }
        *uuid.offset(36) = 0;
    }
}

#[no_mangle]
pub fn rd_netclient_drop(client: *mut Client) {
    unsafe {
        let mut client = Box::from_raw(client);
        client.kill.send(());
        log(format!("rd_netclient_drop"));
    };
}

#[no_mangle]
pub fn rd_netclient_vox_push(client: *mut Client, bytes: *const u8, count: u32) {
    unsafe {
        let vox = std::slice::from_raw_parts(bytes, count as usize);
        let vox = Vec::from(vox);
        (*client).vox.send(vox);
    }
}

#[no_mangle]
pub fn rd_netclient_vox_pop(client: *mut Client) -> *mut Vec<u8> {
    unsafe {
        let mut data : Vec<u8> = Vec::new();
        {
            if let Ok(mut locked_queue) = (*client).vox_queue.try_lock() {
                if let Some(m) = locked_queue.pop_front() {
                    data = m;
                }
            }
        }
        let data = Box::new(data);
        Box::into_raw(data)
    }
}

#[no_mangle]
pub fn rd_netclient_vox_drop(vox: *mut Vec<u8>) {
    unsafe { Box::from_raw(vox) };
}

#[no_mangle]
pub fn rd_netclient_open(local_addr: *const c_char, server_addr: *const c_char) -> *mut Client {

    let local_addr = unsafe { std::ffi::CStr::from_ptr(local_addr).to_owned().into_string().unwrap() };
    let server_addr = unsafe { std::ffi::CStr::from_ptr(server_addr).to_owned().into_string().unwrap() };

    let (kill_tx, kill_rx) = futures::sync::mpsc::channel::<()>(0);

    let (ffi_tx, ffi_rx) = futures::sync::mpsc::unbounded::<Vec<u8>>();
    let (vox_out_tx, vox_out_rx) = futures::sync::mpsc::channel::<Vec<u8>>(0);

    let msg_queue: VecDeque<Vec<u8>> = VecDeque::new();
    let msg_queue = Arc::new(Mutex::new(msg_queue));

    let vox_queue: VecDeque<Vec<u8>> = VecDeque::new();
    let vox_queue = Arc::new(Mutex::new(vox_queue));

    let mut client = Box::new(Client{
        uuid: Uuid::new_v4(),
        sender_pubsub: ffi_tx.wait(),
        vox: vox_out_tx.wait(),
        task: None,
        msg_queue: Arc::clone(&msg_queue),
        vox_queue: Arc::clone(&vox_queue),
        kill: kill_tx.wait(),
    });

    let task = thread::spawn(move || {

        let mut core = Core::new().unwrap();
        let handle = core.handle();

        let mumble_server = String::from("165.227.15.250:64738");
        let (app_logic, mum_tx, vox_in_rx) = mumblebot::run(&mumble_server, &handle);
        let app_logic = app_logic.map_err(|_| ());
        let mumble_say = mumblebot::say(vox_out_rx, mum_tx).map_err(|_| ());

        let mumble_listen = vox_in_rx.fold(vox_queue, |queue, pcm| {
            {
                let mut locked_queue = queue.lock().unwrap();
                locked_queue.push_back(pcm);
            }
            ok::<SharedQueue<Vec<u8>>, std::io::Error>(queue)
            .map_err(|_| ())
        });
        
        let server_addr: SocketAddr = server_addr.parse().unwrap();
        let local_addr: SocketAddr = local_addr.parse().unwrap_or(SocketAddr::new(IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1)), 0));
        let udp_socket = UdpSocket::bind(&local_addr, &handle).unwrap();
        let (tx, rx) = udp_socket.framed(LineCodec).split();

        let msg_tx = ffi_rx.fold(tx, |tx, msg| {
            tx.send((server_addr, msg))
            .map_err(|_| ())
        });

        let msg_rx = rx.fold(msg_queue, |queue, (_, msg)| {
            {
                let mut locked_queue = queue.lock().unwrap();
                locked_queue.push_back(msg);
            }
            ok::<SharedQueue<Vec<u8>>, std::io::Error>(queue)
        })
        .map_err(|_| ());

        let kill_switch = kill_rx
            .fold((),|a,b| {
                log(format!("kill_switch"));
                err::<(),()>(())
            });

        let msg_tasks = Future::join(msg_tx, msg_rx);
        let mum_tasks = Future::join(mumble_say,mumble_listen);

        core.run(Future::join4( mum_tasks,msg_tasks, app_logic, kill_switch));


        log(format!("core end"));

    });

    client.task = Some(task);

    Box::into_raw(client)
}

#[repr(C)]
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
pub struct RigidBody {
    id: u8,
    px: f32,
    py: f32,
    pz: f32,
    pw: f32,
    lx: f32,
    ly: f32,
    lz: f32,
    lw: f32,
}

#[repr(C)]
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
pub struct Avatar {
    id: u8,
    px: f32,
    py: f32,
    pz: f32,
    pw: f32,
    rx: f32,
    ry: f32,
    rz: f32,
    rw: f32,
}

#[repr(C)]
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
pub struct World {
    avatar_parts: Vec<Avatar>,
    rigid_bodies: Vec<RigidBody>,
}

#[no_mangle]
pub fn rd_netclient_push_world(client: *mut Client, world: *const World) {
    
    unsafe {
        let mut msg = vec![1u8];
        let mut encoded: Vec<u8> = serialize(&(*world), Infinite).unwrap();
        msg.append(&mut encoded);
        (*client).sender_pubsub.send(msg);
    }
}

#[no_mangle]
pub fn rd_netclient_dec_world(bytes: *const u8, count: u32) -> *const World {
    unsafe {
        let msg = std::slice::from_raw_parts(bytes, count as usize);
        let world: World = deserialize(msg).unwrap();
        let world = Box::new(world);
        Box::into_raw(world)
    }
}

#[no_mangle]
pub fn rd_netclient_drop_world(world: *mut World) {
    unsafe { Box::from_raw(world) };
}

#[derive(Serialize, Deserialize, PartialEq, Debug)]
pub struct TestEntity {
    x: f32,
    y: f32,
}

#[derive(Serialize, Deserialize, PartialEq, Debug)]
pub struct TestWorld(Vec<TestEntity>);

#[no_mangle]
pub fn rd_netclient_test_world(world: *const TestWorld) {
    unsafe {
        let world_cmp = TestWorld(vec![TestEntity { x: 0.0, y: 4.0 }, TestEntity { x: 10.0, y: 20.5 }]);

        assert_eq!(world_cmp, *world, "raw ffi");
        println!("raw world: {:?}", *world);

        let encoded: Vec<u8> = serialize(&(*world), Infinite).unwrap();
        assert_eq!(encoded.len(), 8 + 4 * 4, "compact length");

        let decoded: TestWorld = deserialize(&encoded[..]).unwrap();
        assert_eq!(world_cmp, decoded, "decoding world");
        println!("decoded world: {:?}", decoded);
    }
}

#[no_mangle]
pub fn rd_netclient_real_world(world: *const World) {
    unsafe {
        let world_cmp = World {
            avatar_parts: vec![
                Avatar{id: 20, px: 1.0, py: 1.1, pz: 1.2, pw: 1.3, rx: 2.0, ry: 2.1, rz: 2.2, rw: 2.3},
                Avatar{id: 21, px: 1.0, py: 1.1, pz: 1.2, pw: 1.3, rx: 2.0, ry: 2.1, rz: 2.2, rw: 2.3},
                Avatar{id: 22, px: 1.0, py: 1.1, pz: 1.2, pw: 1.3, rx: 2.0, ry: 2.1, rz: 2.2, rw: 2.3},
                Avatar{id: 23, px: 1.0, py: 1.1, pz: 1.2, pw: 1.3, rx: 2.0, ry: 2.1, rz: 2.2, rw: 2.3},
                Avatar{id: 24, px: 1.0, py: 1.1, pz: 1.2, pw: 1.3, rx: 2.0, ry: 2.1, rz: 2.2, rw: 2.3},
                ],
            rigid_bodies: vec![
                RigidBody{id: 10, px: 1.0, py: 1.1, pz: 1.2, pw: 1.3, lx: 2.0, ly: 2.1, lz: 2.2, lw: 2.3},
                RigidBody{id: 11, px: 1.0, py: 1.1, pz: 1.2, pw: 1.3, lx: 2.0, ly: 2.1, lz: 2.2, lw: 2.3},
                RigidBody{id: 12, px: 1.0, py: 1.1, pz: 1.2, pw: 1.3, lx: 2.0, ly: 2.1, lz: 2.2, lw: 2.3},
                ],
        };

        println!("mem::size_of::<Avatar> {}", std::mem::size_of::<Avatar>());
        println!("mem::size_of::<RigidBody> {}", std::mem::size_of::<RigidBody>());
        println!("mem::size_of::<Vec<Avatar>> {}", std::mem::size_of::<Vec<Avatar>>());
        println!("mem::size_of::<Vec<RigidBody>> {}", std::mem::size_of::<Vec<RigidBody>>());
        println!("mem::size_of::<World> {}", std::mem::size_of::<World>());
        println!("mem::size_of::<usize> {}", std::mem::size_of::<usize>());

        assert_eq!(world_cmp, *world, "struct layout match");

        let encoded: Vec<u8> = serialize(&(*world), Infinite).unwrap();
        let decoded: World = deserialize(&encoded[..]).unwrap();
        assert_eq!(world_cmp, decoded, "encoding decoding match");
    }
}

#[cfg(test)]
mod tests {

    use super::*;

    #[test]
    fn it_works() {
    }
}

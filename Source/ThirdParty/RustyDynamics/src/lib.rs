#![allow(non_snake_case)]

extern crate bincode;
extern crate cgmath;
extern crate futures;
pub extern crate mumblebot;
extern crate time;
#[macro_use]
extern crate serde_derive;
extern crate tokio_core;
extern crate uuid;
extern crate byteorder;

use mumblebot::positional;
use cgmath::*;

use bincode::{deserialize, serialize, Infinite};

use std::io;
use std::net::{IpAddr, Ipv4Addr, SocketAddr};
use std::thread;
use std::sync::{Arc, Mutex};
use std::collections::VecDeque;
use std::hash::{Hash, Hasher};

use futures::{Future, Sink, Stream};
use futures::future::{err, ok};
use tokio_core::net::{UdpCodec, UdpSocket};
use tokio_core::reactor::Core;

use std::os::raw::c_char;
use time::{Duration, PreciseTime};
mod rnd;

use byteorder::{LittleEndian, ReadBytesExt,WriteBytesExt};
pub struct LineCodec;

extern "C" fn null_log(_log: *const std::os::raw::c_char) {
    ()
}

static mut C_LOG: extern "C" fn(log: *const c_char) = null_log;

#[no_mangle]
pub extern "C" fn rb_log_fn(log_fn: extern "C" fn(log: *const c_char)) {
    unsafe {
        C_LOG = log_fn;
    }
}

fn log(log: std::string::String) {
    let c_string = std::ffi::CString::new(log).unwrap();
    unsafe { C_LOG(c_string.as_ptr()) };
}

#[no_mangle]
pub fn rb_uuid() -> u32 {
    let mut s = std::collections::hash_map::DefaultHasher::new();
    uuid::Uuid::new_v4().hash(&mut s);
    ((s.finish() + 10000) as u16) as u32 | 0b00000000000000100000000000000000
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
    sender_pubsub: futures::sink::Wait<futures::sync::mpsc::Sender<Vec<u8>>>,
    positional: futures::sink::Wait<futures::sync::mpsc::Sender<positional::PositionalAudio>>,
    listener: futures::sink::Wait<futures::sync::mpsc::Sender<positional::PositionalAudio>>,
    msg_queue: SharedQueue<Vec<u8>>,
    kill: futures::sink::Wait<futures::sync::mpsc::Sender<()>>,
}


#[no_mangle]
pub fn rd_register(client: *mut Client,  id:u32) -> bool{
	unsafe{
		let mut msg = vec![1u8,1u8];
		let mut wtr = vec![];
		wtr.write_u32::<LittleEndian>(id).unwrap();
		msg.append(&mut wtr);
		if let Err(err) = (*client).sender_pubsub.send(msg) {
		  	  return false;
			  log(format!("rd_request_server_time: {} ",err));
		  }else{
		  	  return true;
		  }
	}

}
#[no_mangle]
pub fn rd_request_server_time(client: *mut Client, req: u8)->u64{
	unsafe{
		let mut msg = vec![1u8];
		msg[1]=req;
		let mut encoded: Vec<u8> =  Vec::new();
		msg.append(&mut encoded);
		  let requestedTime = rd_system_time();
		  if let Err(err) = (*client).sender_pubsub.send(msg) {
		  	  return 0;
			  log(format!("rd_request_server_time: {} ",err));
		  }else{
		  	  return requestedTime;
		  }
	}
}

#[no_mangle]
pub fn rd_system_time()->u64{
	return time::precise_time_ns();
}
#[no_mangle]
pub fn rd_time_delta_ns(time1:u64,time2:u64)->u64{
let a : Duration = Duration::nanoseconds(time1 as i64);
let b : Duration = Duration::nanoseconds(time2 as i64);
match b.checked_sub(&a) {
	Some(x)=>{match x.num_nanoseconds(){
				Some(y)=>return y as u64,
				None=>return 0
				}
			},
			None=>return 0
	}
}
#[no_mangle]
pub fn rd_netclient_msg_push(client: *mut Client, bytes: *const u8, count: u32) -> bool {
    unsafe {
        let msg = std::slice::from_raw_parts(bytes, count as usize);
        let msg = Vec::from(msg);
        if let Err(err) = (*client).sender_pubsub.send(msg) {
			return false;
            log(format!("rd_netclient_msg_push: {}", err));
        }else{
			return true;
		}
    }
}

#[no_mangle]
pub fn rd_netclient_msg_pop(client: *mut Client) -> *mut Vec<u8> {
    unsafe {
        let mut data: Vec<u8> = Vec::new();
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
pub fn rd_netclient_drop(client: *mut Client) {
    unsafe {
        let mut client = Box::from_raw(client);
        let res = client.kill.send(());
        log(format!("rd_netclient_drop: {:?}", res));
    };
}

#[no_mangle]
pub fn rd_netclient_open(
    local_addr: *const c_char,
    server_addr: *const c_char,
    mumble_addr: *const c_char,
    audio_src: *const c_char,
) -> *mut Client {
    let local_addr = unsafe {
        std::ffi::CStr::from_ptr(local_addr)
            .to_owned()
            .into_string()
            .unwrap()
    };
    let server_addr = unsafe {
        std::ffi::CStr::from_ptr(server_addr)
            .to_owned()
            .into_string()
            .unwrap()
    };
    let mumble_addr = unsafe {
        std::ffi::CStr::from_ptr(mumble_addr)
            .to_owned()
            .into_string()
            .unwrap()
    };
    let audio_src = unsafe {
        std::ffi::CStr::from_ptr(audio_src)
            .to_owned()
            .into_string()
            .unwrap()
    };
    netclient_open(local_addr, server_addr, mumble_addr, audio_src)
}

pub fn netclient_open(
    local_addr: String,
    server_addr: String,
    mumble_addr: String,
    audio_device: String,
) -> *mut Client {
    let local_addr: SocketAddr = local_addr
        .parse()
        .unwrap_or(SocketAddr::new(IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1)), 0));
    let server_addr: SocketAddr = server_addr
        .parse()
        .unwrap_or(SocketAddr::new(IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1)), 0));
    let mumble_addr: SocketAddr = mumble_addr
        .parse()
        .unwrap_or(SocketAddr::new(IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1)), 0));

    let (kill_tx, kill_rx) = futures::sync::mpsc::channel::<()>(0);

    let (ffi_tx, ffi_rx) = futures::sync::mpsc::channel::<Vec<u8>>(1000);

    let (positional_tx, positional_rx) =
        futures::sync::mpsc::channel::<positional::PositionalAudio>(10);
    let (listener_tx, listener_rx) =
        futures::sync::mpsc::channel::<positional::PositionalAudio>(10);

    let (vox_out_tx, vox_out_rx) = futures::sync::mpsc::channel::<Vec<u8>>(1000);
    let mut voxs = Vec::new();
    let mut vox_inp_rxs = Vec::new();
    for _ in 0..16 {
        let (vox_inp_tx, vox_inp_rx) =
            futures::sync::mpsc::channel::<(Vec<u8>, positional::PositionalAudio)>(1000);
        voxs.push(positional::VoxIn {
            session_id: 999999999,
            last_io: std::time::SystemTime::now(),
            tx: vox_inp_tx,
        });
        vox_inp_rxs.push(vox_inp_rx);
    }

    let msg_queue: VecDeque<Vec<u8>> = VecDeque::new();
    let msg_queue = Arc::new(Mutex::new(msg_queue));

    let client = Box::new(Client {
        sender_pubsub: ffi_tx.wait(),
        positional: positional_tx.wait(),
        listener: listener_tx.wait(),
        msg_queue: Arc::clone(&msg_queue),
        kill: kill_tx.wait(),
    });

    thread::spawn(move || {
        let mut core = Core::new().unwrap();
        let handle = core.handle();

        let (mumble_loop, _tcp_tx, udp_tx) = mumblebot::run(local_addr, mumble_addr, voxs, &handle);

        let mumble_say = mumblebot::say(vox_out_rx, positional_rx, udp_tx.clone());
        let device = match audio_device.is_empty() {
            true => None,
            false => Some(audio_device),
        };
        let kill_sink = mumblebot::gst::sink_main(device, vox_out_tx.clone());
        let (kill_src, mumble_listen, listener_task) =
            mumblebot::gst::src_main(listener_rx, vox_inp_rxs);

        let udp_socket = UdpSocket::bind(&local_addr, &handle).unwrap();
        let (tx, rx) = udp_socket.framed(LineCodec).split();

        let msg_out_task = ffi_rx
            .fold(tx, |tx, msg| tx.send((server_addr, msg)).map_err(|_| ()))
            .map_err(|_| std::io::Error::new(std::io::ErrorKind::Other, "msg_out_task"));

        let msg_inp_task = rx.fold(msg_queue, |queue, (_, msg)| {
            {
                let mut locked_queue = queue.lock().unwrap();
                locked_queue.push_back(msg);
            }
            ok::<SharedQueue<Vec<u8>>, std::io::Error>(queue)
        }).map_err(|_| std::io::Error::new(std::io::ErrorKind::Other, "msg_inp_task"));

        let kill_switch = kill_rx
            .fold((), |_a, _b| {
                log(format!("kill_switch"));
                kill_sink();
                kill_src();
                err::<(), ()>(())
            })
            .map_err(|_| std::io::Error::new(std::io::ErrorKind::Other, "kill_switch"));

        let msg_tasks = Future::join(msg_inp_task, msg_out_task);
        let mum_tasks = Future::join(mumble_say, mumble_listen);

        if let Err(err) = core.run(Future::join5(
            mum_tasks,
            msg_tasks,
            mumble_loop,
            listener_task,
            kill_switch,
        )) {
            // if let Err(err) = core.run(Future::join(mum_tasks, mumble_loop)) {
            log(format!("rd_netclient_open: {}", err));
        }

        log(format!("core end"));
    });

    Box::into_raw(client)
}

#[repr(C)]
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
pub struct Rigidbody {
    id: u32,
    px: f32,
    py: f32,
    pz: f32,
    pw: f32,
    lx: f32,
    ly: f32,
    lz: f32,
    lw: f32,
    rx: f32,
    ry: f32,
    rz: f32,
    rw: f32,
    ax: f32,
    ay: f32,
    az: f32,
    aw: f32,
}

#[repr(C)]
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
pub struct NetGuid{
	_session: u8,
	_class: u8,
	_object:u8,
	_property:u8
}
#[repr(C)]
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
pub struct Avatar {
    id: u32,
    root_px: f32,
    root_py: f32,
    root_pz: f32,
    root_pw: f32,
    root_rx: f32,
    root_ry: f32,
    root_rz: f32,
    root_rw: f32,
    head_px: f32,
    head_py: f32,
    head_pz: f32,
    head_pw: f32,
    head_rx: f32,
    head_ry: f32,
    head_rz: f32,
    head_rw: f32,
    handL_px: f32,
    handL_py: f32,
    handL_pz: f32,
    handL_pw: f32,
    handL_rx: f32,
    handL_ry: f32,
    handL_rz: f32,
    handL_rw: f32,
    handR_px: f32,
    handR_py: f32,
    handR_pz: f32,
    handR_pw: f32,
    handR_rx: f32,
    handR_ry: f32,
    handR_rz: f32,
    handR_rw: f32,
    height: f32,
    floor: f32,
}

#[repr(C)]
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
pub struct Archive {
	guid: u32,		
	data: Vec<u8>,
}

#[no_mangle]
pub fn rd_netclient_push_archive(client: *mut Client,farchive: *const Archive){
	unsafe {	
		let mut msg = vec![4u8];
		let mut encoded: Vec<u8> = serialize(&(*farchive), Infinite).unwrap();
		msg.append(&mut encoded);

		if let Err(err) = (*client).sender_pubsub.send(msg) {
            log(format!("rd_netclient_push_avatar: {}", err));
		}
	}
}

#[no_mangle]
pub fn rd_netclient_dec_archive(bytes: *const u8, count: u32) -> *const Archive{
	unsafe {
        let msg = std::slice::from_raw_parts(bytes, count as usize);
		  let archive: Archive = deserialize(msg).unwrap();
        let archive = Box::new(archive);
        Box::into_raw(archive)
    }
}
#[no_mangle]
pub fn rd_netclient_drop_archive(archive: *mut Archive){
	 unsafe { Box::from_raw(archive) };
}

#[no_mangle]
pub fn rd_netclient_push_avatar(client: *mut Client, avatar: *const Avatar) {
    unsafe {
        let mut msg = vec![2u8];
        let mut encoded: Vec<u8> = serialize(&(*avatar), Infinite).unwrap();
        msg.append(&mut encoded);
        if let Err(err) = (*client).sender_pubsub.send(msg) {
            log(format!("rd_netclient_push_avatar: {}", err));
        }
        let pos = positional::PositionalAudio {
            loc: vec3((*avatar).head_px, (*avatar).head_py, (*avatar).head_pz),
            rot: Quaternion::new(
                (*avatar).head_rw,
                (*avatar).head_rx,
                (*avatar).head_ry,
                (*avatar).head_rz,
            ),
        };
        if let Err(err) = (*client).positional.send(pos) {
            log(format!("rd_netclient_push_avatar positional: {}", err));
        }
        if let Err(err) = (*client).listener.send(pos) {
            log(format!("rd_netclient_push_avatar listener: {}", err));
        }
    }
}

#[no_mangle]
pub fn rd_netclient_dec_avatar(bytes: *const u8, count: u32) -> *const Avatar {
    unsafe {
        let msg = std::slice::from_raw_parts(bytes, count as usize);
        let avatar: Avatar = deserialize(msg).unwrap();
        let avatar = Box::new(avatar);
        Box::into_raw(avatar)
    }
}

#[no_mangle]
pub fn rd_netclient_drop_avatar(avatar: *mut Avatar) {
    unsafe { Box::from_raw(avatar) };
}

#[no_mangle]
pub fn rd_netclient_push_rigidbody(client: *mut Client, rigidbody: *const Rigidbody) {
    unsafe {
        let mut msg = vec![3u8];
        let mut encoded: Vec<u8> = serialize(&(*rigidbody), Infinite).unwrap();
        msg.append(&mut encoded);
        (*client).sender_pubsub.send(msg);
    }
}

#[no_mangle]
pub fn rd_netclient_dec_rigidbody(bytes: *const u8, count: u32) -> *const Rigidbody {
    unsafe {
        let msg = std::slice::from_raw_parts(bytes, count as usize);
        let rigidbody: Rigidbody = deserialize(msg).unwrap();
        let rigidbody = Box::new(rigidbody);
        Box::into_raw(rigidbody)
    }
}

#[no_mangle]
pub fn rd_netclient_drop_rigidbody(rigidbody: *mut Rigidbody) {
    unsafe { Box::from_raw(rigidbody) };
}

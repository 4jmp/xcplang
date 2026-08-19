use std::ffi::CStr;
use std::os::raw::c_char;
#[no_mangle]
pub extern "C" fn xcp_rust_version() -> *const c_char {
    b"rust-vm-bridge-1.0\0".as_ptr() as *const c_char
}
#[no_mangle]
pub extern "C" fn xcp_rust_count_bytes(input: *const c_char) -> usize {
    if input.is_null() {
        return 0;
    }
    unsafe { CStr::from_ptr(input).to_bytes().len() }
}

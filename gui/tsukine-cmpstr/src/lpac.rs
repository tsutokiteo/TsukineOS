#[derive(Debug, PartialEq)]
pub enum LpacLevel { Light, Heavy, GuestDenied }

pub fn request(level: LpacLevel, reason: &str) {
    println!("[LPAC] {:?} — {}", level, reason);
}
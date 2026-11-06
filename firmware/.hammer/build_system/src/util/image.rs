/// Image where the contents of memory are editable, and can be turned into a compact image
pub struct EditableImage {
    data: BTreeMap<u32, u8>,
}
/// Image with multiple data chunks and their respective addresses. Coalesced where possible
pub struct Image {
    data: Vec<ImageEntry>,
}
/// Data chunk at a given address
pub struct ImageEntry {
    pub address: u32,
    pub data: Vec<u8>,
}
impl EditableImage {
    /// Create a new, empty image for editing
    pub fn new() -> Self {
        Self {
            data: BTreeMap::new(),
        }
    }
    /// Create editable image from Intel HEX data
    pub fn from_intel_hex(hex: &str) -> Result<Self, IntelHexError> {
        let mut this = Self::new();
        this.insert_intel_hex(hex)?;
        Ok(this)
    }
    /// Insert data at address, overwriting as needed
    pub fn insert(&mut self, address: u32, data: &[u8]) {
        for (n, b) in data.iter().enumerate() {
            self.data.insert(n as u32 + address, *b);
        }
    }
    /// Insert the data represented by the provided image
    pub fn insert_image(&mut self, other: &Image) {
        for entry in other.iter() {
            self.insert(entry.address, &entry.data);
        }
    }
    /// Insert the data represented by the provided image
    pub fn insert_editable_image(&mut self, other: &EditableImage) {
        self.data.extend(other.data.iter());
    }
    /// Insert/merge the provided Intel HEX data into this current image.
    pub fn insert_intel_hex(&mut self, hex: &str) -> Result<(), IntelHexError> {
        use ihex::Record;

        let hex = ihex::Reader::new(hex);

        enum ExtendedAddress {
            Segment(u16),
            Linear(u16),
        }

        let mut extended_address = ExtendedAddress::Linear(0);
        let mut eof_found = false;

        for record in hex.into_iter() {
            if eof_found {
                Err(IntelHexError::DataAfterEof)?;
            }
            match record? {
                Record::Data { offset, value } => {
                    let address = match extended_address {
                        ExtendedAddress::Segment(segment) => (segment as u32) * 16,
                        ExtendedAddress::Linear(linear) => (linear as u32) << 16,
                    } + offset as u32;
                    self.insert(address, &value);
                }
                Record::ExtendedSegmentAddress(segment) => {
                    extended_address = ExtendedAddress::Segment(segment)
                }
                Record::ExtendedLinearAddress(linear) => {
                    extended_address = ExtendedAddress::Linear(linear)
                }
                Record::StartSegmentAddress { .. } => {}
                Record::StartLinearAddress(_) => {}
                Record::EndOfFile => {
                    eof_found = true;
                }
            }
        }
        if !eof_found {
            Err(IntelHexError::MissingEof)?;
        }
        Ok(())
    }
    /// Convert to a read-only coalesced image
    pub fn as_image(&self) -> Image {
        let mut blocks = Vec::new();
        let mut block: Option<ImageEntry> = None;

        for (&address, &data) in &self.data {
            match block {
                Some(ref mut block) => {
                    if block.address + block.data.len() as u32 == address {
                        block.data.push(data);
                    } else {
                        blocks.push(std::mem::replace(
                            block,
                            ImageEntry {
                                address,
                                data: vec![data],
                            },
                        ));
                    }
                }
                None => {
                    block = Some(ImageEntry {
                        address,
                        data: vec![data],
                    })
                }
            }
        }

        if let Some(block) = block {
            blocks.push(block)
        }

        Image { data: blocks }
    }
    /// Return the bounds of the firmware data internally
    pub fn bounds(&self) -> Option<std::ops::RangeInclusive<u32>> {
        self.data
            .first_key_value()
            .map(|(&first, _)| self.data.last_key_value().map(|(&last, _)| first..=last))
            .flatten()
    }
    /// Pad the image with the provided value, anywhere in range where there isn't already data.
    pub fn pad(&mut self, value: u8, range: impl Iterator<Item = u32>) {
        for address in range {
            self.data.entry(address).or_insert(value);
        }
    }
}
impl Image {
    /// Convert image to Intel HEX representation
    pub fn as_intel_hex(&self) -> String {
        use ihex::Record;
        let mut records = Vec::new();
        let mut linear_address_base = None;
        for block in self.iter() {
            const CHUNK_SIZE: usize = 16;
            let mut address = block.address;
            for data in block.data.chunks(CHUNK_SIZE) {
                let new_linear_base = (address >> 16) as u16;
                if Some(new_linear_base) != linear_address_base {
                    records.push(Record::ExtendedLinearAddress(new_linear_base));
                    linear_address_base = Some(new_linear_base);
                }
                records.push(Record::Data {
                    offset: (address & 0xFFFF) as u16,
                    value: data.into(),
                });
                address += data.len() as u32;
            }
        }
        records.push(Record::EndOfFile);
        ihex::create_object_file_representation(&records).unwrap()
    }
}
impl IntoIterator for Image {
    type Item = ImageEntry;

    type IntoIter = <Vec<ImageEntry> as IntoIterator>::IntoIter;

    fn into_iter(self) -> Self::IntoIter {
        self.data.into_iter()
    }
}
impl std::ops::Deref for Image {
    type Target = Vec<ImageEntry>;

    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

#[derive(PartialEq, Eq, Clone, Copy, Debug, Hash)]
pub enum IntelHexError {
    /// ParserError
    Parser(ihex::ReaderError),
    /// Missing EOF Marker
    MissingEof,
    /// Data After EOF
    DataAfterEof,
}

impl Error for IntelHexError {}

impl fmt::Display for IntelHexError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            IntelHexError::Parser(e) => write!(f, "{}", e),
            IntelHexError::MissingEof => write!(f, "No EOF Marker found in input"),
            IntelHexError::DataAfterEof => write!(f, "Data found after EOF Marker found in input"),
        }
    }
}
impl From<ihex::ReaderError> for IntelHexError {
    fn from(value: ihex::ReaderError) -> Self {
        Self::Parser(value)
    }
}

use std::collections::BTreeMap;
use std::error::Error;
use std::fmt;

# geds-rs: Rust bindings for GEDS

This project contains a Rust crate that implements a Rust API for GEDS' C++ implementation. 
It utilizes [CXX](https://cxx.rs) to generate a safe C ABI for Rust/C++ interoperability. 

To use in a Rust project, add the ```geds-rs``` crate as a dependency in your Cargo.toml file, specifying the path to the install
directory of ```geds-rs``` (under ```${GEDS_INSTALL_DIR}/rust```):

```toml
[dependencies]
geds-rs = { path = "path/to/geds-rs" }
```

This crate requires that ```libgeds.so``` is installed and available in one of the system's library paths. ```geds-rs``` is currently supported in Ubuntu 22.04 and 20.04.

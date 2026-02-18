//
// Copyright 2022- IBM Inc. All rights reserved
// SPDX-License-Identifier: Apache-2.0
//

fn main() {
    let manifest_dir = env!("CARGO_MANIFEST_DIR");

    cxx_build::bridge("src/lib.rs")
        .file(format!("{manifest_dir}/src/GEDSFileWrapper.cpp"))
        .file(format!("{manifest_dir}/src/GEDSWrapper.cpp"))
        .include(format!("{manifest_dir}/src"))
        .include(format!("{manifest_dir}/include"))
        .std("c++20")
        .compile("geds_rs");

    println!("cargo:rerun-if-changed={manifest_dir}/src/lib.rs");
    println!("cargo:rerun-if-changed={manifest_dir}/src/GEDSWrapper.cpp");    
    println!("cargo:rerun-if-changed={manifest_dir}/src/GEDSFileWrapper.cpp");
    println!("cargo:rustc-link-lib=dylib=geds");
}

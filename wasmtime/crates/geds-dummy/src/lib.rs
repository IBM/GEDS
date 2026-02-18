//
// Copyright 2022- IBM Inc. All rights reserved
// SPDX-License-Identifier: Apache-2.0
//


pub mod ffi {
    pub struct GEDSConfig {
        pub metadata_service_address: String,
        pub listen_address: String,
        pub hostname: String,
        pub port: u16,
        pub port_http_server: u16,
        pub local_storage_path: String,
        pub cache_block_size: usize,
        pub cache_objects_from_s3: bool,
        pub available_local_storage: usize,
        pub available_local_memory: usize,
        pub force_relocation_when_stopping: bool,
        pub pub_sub_enabled: bool,
    }

    pub struct GEDSFileStatus {
        key: String,
        size: usize,
        is_directory: bool,
    }

    pub struct Status {
        message: String,
        ok: bool,
    }

    pub struct StatusOrGEDSFileWrapper {
        status: Status,
        value: SharedPtr<GEDSFileWrapper>,
    }

    pub struct StatusOrGEDSFileStatus {
        status: Status,
        value: GEDSFileStatus,
    }

    pub struct StatusOrVecGEDSFileStatus {
        status: Status,
        value: Vec<GEDSFileStatus>,
    }

    pub struct StatusOrUsize {
        status: Status,
        value: usize,
    }
}

//
pub struct GEDS {
    geds_ptr: i32,
}

impl GEDS {
    /// Start GEDS.
    pub fn start(&self) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    /// Stop GEDS.
    pub fn stop(&self) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    /// Create object located at bucket/key.
    /// The object is registered with the metadata service once the file is sealed.
    pub fn create(&self, bucket: &str, key: &str, overwrite: bool) -> Result<GEDSFile, String> {
        if true {
            let file = GEDSFile {
                file_ptr: 0,
            };
            Ok(file)
        } else {
            Err("")
        }
    }

    /// Register a bucket with the metadata server.
    pub fn create_bucket(&self, bucket: &str) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    /// Recursively create directory using directory markers.
    pub fn mkdirs(&self, bucket: &str, path: &str) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    /// List objects in bucket where the key starts with 'prefix'.
    pub fn list(&self, bucket: &str, prefix: &str) -> Result<Vec<ffi::GEDSFileStatus>, String> {
        if true {
            Ok(status_or_result.value)
        } else {
            Err("")
        }
    }

    /// List objects in `bucket` where the key starts with `prefix` and the postfix does not contain `delimiter`.
    //  - If the delimiter set to `0` will list all keys starting with prefix.
    //  - If the delimiter is set to a value != 0, then the delimiter will be used as a folder
    //  separator. Keys ending with "/_$folder$" will be used as directory markers (where '/' is used
    //  as a delimiter).
    pub fn list_folder(
        &self,
        bucket: &str,
        prefix: &str,
    ) -> Result<Vec<ffi::GEDSFileStatus>, String> {
        if true {
            Ok(Vec::<ffi::GEDSFileStatus>::with_capacity(1))
        } else {
            Err("")
        }
    }

    /// Get status of `key` in `bucket`
    pub fn status(&self, bucket: &str, key: &str) -> Result<ffi::GEDSFileStatus, String> {
        if true {
            Ok(ffi::GEDSFileStatus{key: "", size: 0, is_directory: true})
        } else {
            Err("")
        }
    }

    /// Open object located at `bucket`/`key`.
    pub fn open(&self, bucket: &str, key: &str) -> Result<GEDSFile, String> {
        if true {
            let file = GEDSFile {
                file_ptr: 0,
            };
            Ok(file)
        } else {
            Err("")
        }
    }

    /// Delete object in `bucket` with `key`.
    pub fn delete_object(&self, bucket: &str, key: &str) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    /// Delete objects in `'bucket`' with keys starting with `prefix`.
    pub fn delete_object_prefix(&self, bucket: &str, prefix: &str) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    /// Rename an object.
    pub fn rename(
        &self,
        src_bucket: &str,
        src_key: &str,
        dest_bucket: &str,
        dest_key: &str,
    ) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    /// Rename a prefix recursively.
    pub fn rename_prefix(
        &self,
        src_bucket: &str,
        src_prefix: &str,
        dest_bucket: &str,
        dest_prefix: &str,
    ) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    /// Copy an object.
    pub fn copy(
        &self,
        src_bucket: &str,
        src_key: &str,
        dest_bucket: &str,
        dest_key: &str,
    ) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    /// Copy a file or a folder structure.
    pub fn copy_prefix(
        &self,
        src_bucket: &str,
        src_prefix: &str,
        dest_bucket: &str,
        dest_prefix: &str,
    ) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    /// Compute the path to the files stored in `_pathPrefix` folder.
    pub fn local_path(&self, bucket: &str, key: &str) -> String {
        ""
    }

    /// Register an object store configuration with GEDS.
    pub fn register_object_store_config(
        &self,
        bucket: &str,
        endpoint_url: &str,
        access_key: &str,
        secret_key: &str,
    ) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    /// Sync object store configs.
    pub fn sync_object_store_configs(&self) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    /// Relocate objects to S3.
    pub fn relocate(&self, force: bool) {
        
    }

    // NO_SUBSCRIPTION = 0,
    // BUCKET = 1,
    // OBJECT = 2,
    // PREFIX = 3,
    pub fn subscribe(
        &self,
        bucket: &str,
        key: &str,
        subscription_type: &i32,
    ) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    /// Create new GEDS instance.
    pub fn new(config: &ffi::GEDSConfig) -> GEDS {
        let wrapper = ffi::new_wrapper(&config);
        GEDS { geds_ptr: wrapper }
    }

    /// Create a `geds_rs`
    pub fn get_default_config() -> ffi::GEDSConfig {
        ffi::GEDSConfig {
            metadata_service_address: "localhost:4381".to_owned(),
            listen_address: "0.0.0.0".to_owned(),
            hostname: "null".to_owned(),
            port: 4382,
            port_http_server: 4380,
            local_storage_path: "/tmp/GEDS_XXXXXX".to_owned(),
            cache_block_size: 32 * 1024 * 1024,
            cache_objects_from_s3: false,
            available_local_storage: 100 * 1024 * 1024 * 1024,
            available_local_memory: 16 * 1024 * 1024 * 1024,
            force_relocation_when_stopping: false,
            pub_sub_enabled: false,
        }
    }
}

unsafe impl Send for GEDS {}
unsafe impl Sync for GEDS {}

/// GEDS File abstraction. Exposes a file buffer.
///
/// - Implements Sparse-File semantics
/// - Sparse File for caching.
/// - Use GEDSFile everywhere to allow unseal.
pub struct GEDSFile {
    file_ptr: i32,
}

impl GEDSFile {
    pub fn seal(&self) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    pub fn truncate(&self, size: usize) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    pub fn set_metadata(&self, metadata: &str, seal: bool) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    pub fn read(
        &self,
        buffer: &mut Vec<u8>,
        position: usize,
        length: usize,
    ) -> Result<usize, String> {
        if true {
            Ok(0)
        } else {
            Err("")
        }
    }

    pub fn write(&self, buffer: &Vec<u8>, position: usize, length: usize) -> Result<(), String> {
        if true {
            Ok(())
        } else {
            Err("")
        }
    }

    pub fn size(&self) -> usize {
        0
    }
    pub fn is_writeable(&self) -> bool {
        false
    }
    pub fn identifier(&self) -> String {
        "a"
    }
    pub fn metadata(&self) -> String {
        "a"
    }
}

unsafe impl Send for GEDSFile {}
unsafe impl Sync for GEDSFile {}

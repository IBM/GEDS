//
// Copyright 2022- IBM Inc. All rights reserved
// SPDX-License-Identifier: Apache-2.0
//

/// Rust API for [GEDS](https://github.com/IBM/GEDS)
use cxx::{SharedPtr, UniquePtr};

#[cxx::bridge(namespace = geds_rs)]
pub mod ffi {
    unsafe extern "C++" {
        include!("GEDSFileWrapper.h");
        type GEDSFileWrapper;
        fn seal(&self) -> Status;
        fn truncate(&self, size: usize) -> Status;
        fn set_metadata(&self, metadata: &str, seal: bool) -> Status;
        fn read(&self, buffer: &mut Vec<u8>, position: usize, length: usize) -> StatusOrUsize;
        fn write(&self, buffer: &Vec<u8>, position: usize, length: usize) -> Status;

        // GEDSFile properties
        fn size(&self) -> usize;
        fn is_writeable(&self) -> bool;
        fn identifier(&self) -> String;
        fn metadata(&self) -> String;
    }

    unsafe extern "C++" {
        include!("GEDSWrapper.h");
        type GEDSWrapper;
        fn start(&self) -> Status;
        fn stop(&self) -> Status;
        fn create(&self, bucket: &str, key: &str, overwrite: bool) -> StatusOrGEDSFileWrapper;
        fn create_bucket(&self, bucket: &str) -> Status;
        fn mkdirs(&self, bucket: &str, path: &str) -> Status;
        fn list(&self, bucket: &str, key: &str) -> StatusOrVecGEDSFileStatus;
        fn list_folder(&self, bucket: &str, prefix: &str) -> StatusOrVecGEDSFileStatus;
        fn status(&self, bucket: &str, key: &str) -> StatusOrGEDSFileStatus;
        fn open(&self, bucket: &str, key: &str) -> StatusOrGEDSFileWrapper;
        fn delete_object(&self, bucket: &str, key: &str) -> Status;
        fn delete_object_prefix(&self, bucket: &str, prefix: &str) -> Status;
        fn subscribe(&self, bucket: &str, key: &str, subscription_type: &i32) -> Status;
        fn rename(
            &self,
            src_bucket: &str,
            src_key: &str,
            dest_bucket: &str,
            dest_key: &str,
        ) -> Status;
        fn rename_prefix(
            &self,
            src_bucket: &str,
            src_prefix: &str,
            dest_bucket: &str,
            dest_prefix: &str,
        ) -> Status;
        fn copy(
            &self,
            src_bucket: &str,
            src_key: &str,
            dest_bucket: &str,
            dest_key: &str,
        ) -> Status;
        fn copy_prefix(
            &self,
            src_bucket: &str,
            src_prefix: &str,
            dest_bucket: &str,
            dest_prefix: &str,
        ) -> Status;
        fn local_path(&self, bucket: &str, key: &str) -> String;
        fn register_object_store_config(
            &self,
            bucket: &str,
            endpoint_url: &str,
            access_key: &str,
            secret_key: &str,
        ) -> Status;
        fn sync_object_store_configs(&self) -> Status;
        fn relocate(&self, force: bool);

        pub fn new_wrapper(config: &GEDSConfig) -> UniquePtr<GEDSWrapper>;
    }

    #[namespace = "shared"]
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

    #[namespace = "shared"]
    pub struct GEDSFileStatus {
        key: String,
        size: usize,
        is_directory: bool,
    }

    #[namespace = "shared"]
    pub struct Status {
        message: String,
        ok: bool,
    }

    #[namespace = "shared"]
    pub struct StatusOrGEDSFileWrapper {
        status: Status,
        value: SharedPtr<GEDSFileWrapper>,
    }

    #[namespace = "shared"]
    pub struct StatusOrGEDSFileStatus {
        status: Status,
        value: GEDSFileStatus,
    }

    #[namespace = "shared"]
    pub struct StatusOrVecGEDSFileStatus {
        status: Status,
        value: Vec<GEDSFileStatus>,
    }

    #[namespace = "shared"]
    pub struct StatusOrUsize {
        status: Status,
        value: usize,
    }
}

//
pub struct GEDS {
    geds_ptr: UniquePtr<ffi::GEDSWrapper>,
}

impl GEDS {
    /// Start GEDS.
    pub fn start(&self) -> Result<(), String> {
        let status = self.geds_ptr.start();
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
        }
    }

    /// Stop GEDS.
    pub fn stop(&self) -> Result<(), String> {
        let status = self.geds_ptr.stop();
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
        }
    }

    /// Create object located at bucket/key.
    /// The object is registered with the metadata service once the file is sealed.
    pub fn create(&self, bucket: &str, key: &str, overwrite: bool) -> Result<GEDSFile, String> {
        let status_or_result = self.geds_ptr.create(bucket, key, overwrite);
        if status_or_result.status.ok {
            let file = GEDSFile {
                file_ptr: status_or_result.value,
            };
            Ok(file)
        } else {
            Err(status_or_result.status.message)
        }
    }

    /// Register a bucket with the metadata server.
    pub fn create_bucket(&self, bucket: &str) -> Result<(), String> {
        let status = self.geds_ptr.create_bucket(bucket);
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
        }
    }

    /// Recursively create directory using directory markers.
    pub fn mkdirs(&self, bucket: &str, path: &str) -> Result<(), String> {
        let status = self.geds_ptr.mkdirs(bucket, path);
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
        }
    }

    /// List objects in bucket where the key starts with 'prefix'.
    pub fn list(&self, bucket: &str, prefix: &str) -> Result<Vec<ffi::GEDSFileStatus>, String> {
        let status_or_result = self.geds_ptr.list(bucket, prefix);
        if status_or_result.status.ok {
            Ok(status_or_result.value)
        } else {
            Err(status_or_result.status.message)
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
        let status_or_result = self.geds_ptr.list_folder(bucket, prefix);
        if status_or_result.status.ok {
            Ok(status_or_result.value)
        } else {
            Err(status_or_result.status.message)
        }
    }

    /// Get status of `key` in `bucket`
    pub fn status(&self, bucket: &str, key: &str) -> Result<ffi::GEDSFileStatus, String> {
        let status_or_result = self.geds_ptr.status(bucket, key);
        if status_or_result.status.ok {
            Ok(status_or_result.value)
        } else {
            Err(status_or_result.status.message)
        }
    }

    /// Open object located at `bucket`/`key`.
    pub fn open(&self, bucket: &str, key: &str) -> Result<GEDSFile, String> {
        let status_or_result = self.geds_ptr.open(bucket, key);
        if status_or_result.status.ok {
            let file = GEDSFile {
                file_ptr: status_or_result.value,
            };
            Ok(file)
        } else {
            Err(status_or_result.status.message)
        }
    }

    /// Delete object in `bucket` with `key`.
    pub fn delete_object(&self, bucket: &str, key: &str) -> Result<(), String> {
        let status = self.geds_ptr.delete_object(bucket, key);
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
        }
    }

    /// Delete objects in `'bucket`' with keys starting with `prefix`.
    pub fn delete_object_prefix(&self, bucket: &str, prefix: &str) -> Result<(), String> {
        let status = self.geds_ptr.delete_object_prefix(bucket, prefix);
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
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
        let status = self
            .geds_ptr
            .rename(src_bucket, src_key, dest_bucket, dest_key);
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
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
        let status = self
            .geds_ptr
            .rename_prefix(src_bucket, src_prefix, dest_bucket, dest_prefix);
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
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
        let status = self
            .geds_ptr
            .copy(src_bucket, src_key, dest_bucket, dest_key);
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
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
        let status = self
            .geds_ptr
            .copy_prefix(src_bucket, src_prefix, dest_bucket, dest_prefix);
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
        }
    }

    /// Compute the path to the files stored in `_pathPrefix` folder.
    pub fn local_path(&self, bucket: &str, key: &str) -> String {
        self.geds_ptr.local_path(bucket, key)
    }

    /// Register an object store configuration with GEDS.
    pub fn register_object_store_config(
        &self,
        bucket: &str,
        endpoint_url: &str,
        access_key: &str,
        secret_key: &str,
    ) -> Result<(), String> {
        let status = self.geds_ptr.register_object_store_config(
            bucket,
            endpoint_url,
            access_key,
            secret_key,
        );
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
        }
    }

    /// Sync object store configs.
    pub fn sync_object_store_configs(&self) -> Result<(), String> {
        let status = self.geds_ptr.sync_object_store_configs();
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
        }
    }

    /// Relocate objects to S3.
    pub fn relocate(&self, force: bool) {
        self.geds_ptr.relocate(force);
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
        let status = self.geds_ptr.subscribe(bucket, key, subscription_type);
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
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
    file_ptr: SharedPtr<ffi::GEDSFileWrapper>,
}

impl GEDSFile {
    pub fn seal(&self) -> Result<(), String> {
        let status = self.file_ptr.seal();
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
        }
    }

    pub fn truncate(&self, size: usize) -> Result<(), String> {
        let status = self.file_ptr.truncate(size);
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
        }
    }

    pub fn set_metadata(&self, metadata: &str, seal: bool) -> Result<(), String> {
        let status = self.file_ptr.set_metadata(metadata, seal);
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
        }
    }

    pub fn read(
        &self,
        buffer: &mut Vec<u8>,
        position: usize,
        length: usize,
    ) -> Result<usize, String> {
        let status_or_result = self.file_ptr.read(buffer, position, length);
        if status_or_result.status.ok {
            Ok(status_or_result.value)
        } else {
            Err(status_or_result.status.message)
        }
    }

    pub fn write(&self, buffer: &Vec<u8>, position: usize, length: usize) -> Result<(), String> {
        let status = self.file_ptr.write(buffer, position, length);
        if status.ok {
            Ok(())
        } else {
            Err(status.message)
        }
    }

    pub fn size(&self) -> usize {
        self.file_ptr.size()
    }
    pub fn is_writeable(&self) -> bool {
        self.file_ptr.is_writeable()
    }
    pub fn identifier(&self) -> String {
        self.file_ptr.identifier()
    }
    pub fn metadata(&self) -> String {
        self.file_ptr.metadata()
    }
}

unsafe impl Send for GEDSFile {}
unsafe impl Sync for GEDSFile {}

#[cfg(test)]
mod tests {
    use crate::GEDS;

    #[test]
    fn test_file() {
        let geds = create_geds();

        // GEDS::start
        let start_result = geds.start();
        assert!(start_result.is_ok(), "{}", start_result.err().unwrap());

        // GEDS::create_bucket
        let create_bucket_result = geds.create_bucket("test-bucket");
        assert!(
            create_bucket_result.is_ok(),
            "{}",
            create_bucket_result.as_ref().unwrap_err()
        );

        // GEDS::create
        let create_result = geds.create("test-bucket", "test-file", true);
        assert!(create_result.is_ok(), "{}", create_result.err().unwrap());

        // GEDSFile::write
        let file = create_result.unwrap();
        let write_vec: Vec<u8> = "Hello world!".as_bytes().to_vec();
        let write_result = file.write(&write_vec, 0, 12);
        assert!(write_result.is_ok(), "{}", write_result.err().unwrap());

        // GEDSFile::read
        let read_vec: &mut Vec<u8> = &mut Vec::with_capacity(12);
        let read_result = file.read(read_vec, 0, 12);
        assert!(
            read_result.is_ok() && *read_result.as_ref().unwrap() == 12,
            "{}",
            read_result.as_ref().unwrap_err()
        );

        assert_eq!(*read_vec, write_vec);

        // GEDSFile::set_metadata
        let set_metadata_result = file.set_metadata("test_metadata", false);
        assert!(
            set_metadata_result.is_ok(),
            "{}",
            set_metadata_result.err().unwrap()
        );

        // GEDSFile::truncate
        assert!(file.truncate(5).is_ok());

        // GEDSFile::seal
        assert!(file.seal().is_ok());

        // GEDSFile properties
        let size = file.size();
        let is_writeable = file.is_writeable();
        let identifier = file.identifier();
        let metadata = file.metadata();
        assert_eq!(size, 5);
        assert_eq!(is_writeable, true); // why?
        println!("File identifier is {identifier}");
        assert_eq!(metadata, "test_metadata");

        // GEDS::delete_object
        assert!(geds.delete_object("test-bucket", "test-file").is_ok());

        // GEDS::stop
        let stop_result = geds.stop();
        assert!(stop_result.is_ok());
    }

    fn create_geds() -> GEDS {
        let config = GEDS::get_default_config();
        GEDS::new(&config)
    }
}

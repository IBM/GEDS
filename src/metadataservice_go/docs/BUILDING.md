# Building

## Used Technologies

- Go v1.20
- protoc libprotoc v3.15

## Building

Do a ``make help`` to see all the available commands.

- Build commands:
  ```bash
  cp env.secret env
  make build
  ```
	- Test command:
  ```bash
    cp tests/configs/app.env.secret tests/configs/app.env
    make test
  ```
  (Optional) for testing populating key-value store with S3 objects, set the S3 config in``env``.


	- Run command:
  ```bash
    cp configs/app.env.secret configs/app.env
    make run-mds
  ```

  ``app.env`` contains the configurations of the MDS, e.g., ports, enabling/disabling publish/subscribe, etc. 

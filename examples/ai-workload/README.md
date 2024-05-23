# GEDS AI Workload Examples

Setup:

- Create the python environment: `python3 -m venv venv`
- Source the python environment: `source venv/bin/activate`
- Install GEDS smart open: Download the latest GEDS smart\_open whl and install it:
  ```
    VERSION=1.0.5
    wget https://github.com/IBM/GEDS/releases/download/v${VERSION}/geds_smart_open-${VERSION}-py3-none-any.whl.ubuntu22.04-release
    mv geds_smart_open-${VERSION}-py3-none-any.whl.ubuntu22.04-release geds_smart_open-${VERSION}-py3-none-any.whl
  ```

Run the examples:

- `ai_training_example.py`: Simulates snapshotting from multiple GPUS and storing the data on S3 in the background.
- `write_data.py`: Write data to GEDS
- `read_data.py`: Read written data from the network (created by `write_data`)
- `write_data_spilling.py`: Write data + explicit spilling to S3 to determine spilling throughput.


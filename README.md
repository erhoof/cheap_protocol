# cheap_protocol
Implementation of memory cheap &amp; safe protocol

## Execution
1. Place **data_in.txt** nearby executable (you can grab it from **test** folder)
2. Execute it
3. Debug info is present in **stdout**, general output is present in **data_out.txt** file

## Build
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```
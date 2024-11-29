## Overview

This project is a semaphore-based resource management system implemented in C. It includes multiple test scenarios to simulate different resource request and liberation patterns. The project is organized into several directories, each serving a specific purpose.

## Directory Structure

```
.gitignore
build/
    sem/
        main.c
    semaphore/
        main.c
main
Makefile
README.md
src/
    lib/
        consts.h
        display.h
        ipc.h
        mq.h
        types.h
        utils.h
    main.c
tests/
    multi_dem_multi_lib/
        0.txt
        1.txt
        2.txt
        3.txt
        4.txt
    multi_dem_one_lib/
        0.txt
        1.txt
        2.txt
        3.txt
        4.txt
    no_dem/
        0.txt
        1.txt
        2.txt
        3.txt
        4.txt
    one_dem_multi_lib/
        0.txt
        1.txt
        2.txt
        3.txt
        4.txt
    one_dem_one_lib/
        0.txt
        1.txt
        2.txt
        3.txt
        4.txt
```

## Files and Directories

- **build/**: Contains the compiled binaries and related files.
  - **sem/**: Contains the main source file for the semaphore-based implementation.
  - **semaphore/**: Contains another main source file for semaphore-based implementation.
- **src/**: Contains the source code and header files.
  - **lib/**: Contains header files with constants, utility functions, and type definitions.
    - consts.h: Defines constants and test paths.
    - display.h, ipc.h, mq.h, types.h, utils.h: Other utility headers.
  - main.c: Main source file for the project.
- **tests/**: Contains test scenarios with different resource request and liberation patterns.
  - **multi_dem_multi_lib/**: Multiple resource requests with multiple liberations.
  - **multi_dem_one_lib/**: Multiple resource requests with one liberation.
  - **no_dem/**: No resource request.
  - **one_dem_multi_lib/**: One resource request with multiple liberations.
  - **one_dem_one_lib/**: One resource request with one liberation.
- **Makefile**: Build script for the project.
- **README.md**: This file.

## **Constants and Definitions**

The constants and test paths are defined in **`consts.h`** and are used throughout the project. Key constants include:

- **`PROCESS_NUM`**: The number of processes.
- **`BUFFER_SIZE`**: The size of the buffer.
- **`WAIT_TIME`**: The wait time for processes.
- **`MSG_SIZE`**: The size of the message.

## Building the Project

To build the project, run the following command in the root directory:

```sh
make
```

This will compile the source files and generate the binaries in the build directory.

## License

This project is licensed under the MIT License. See the LICENSE file for more details.

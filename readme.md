# Operating System 2019/2020 Projects
Here are listed all proposed project ideas for this academic year.
If one would like to do a custom project that she/he designed,
it must be approved by the prof. and the tutor.

### General projects guidelines:
In this section you will find generic guidelines for **all projects**.

###### General guidelines:
- All projects should be hosted on github/gitlab or similars.
- Each student performs commits using his own account.
- We want to see the individual commits. Projects with a single commit are considered invalid.
- The contribution of each member in the group to the development
  should be crystal clear. Comment your sections in the source with some
  initials or nickname

  Example:
  ```cpp
  //gg: here we clear the list of connections and do some other stuff
  List_clear(&connections);
  List_pushBack(&connections, stuff_added);
  //ia: I am erasing stuff because this and that
  List_clear(&nodes)
  ```

###### Arduino guidelines:
- Resources: you should use at least one interrupt, a timer and an I/O port

- All projects involving Arduino should include some sort of serial
  communication with the host.
  The serial communication should be performed using **interrupts**
  (polling based communication is not valid).
  In this sense, you should
    - enable UART interrupts (both tx and rx)
    - whenever a UART interrupt is generated the ISR should put/get the character in two common buffers (one for rx and one for tx)
    - main loop reads/writes from/in this two common buffers in a deferred fashion

  ```cpp
  // rx interrupt service routine
  ISR(USART0_RX_vect) {
    // read the UART register UDR0
    uint8_t c = UDR0;
    // copies values from UART register to common rx_buffer
    // ...
  }
  //tx interrupt service routine
  ISR(USART0_UDRE_vect) {
    // copies values from common tx_buffer to UART register
    // UDR0 = ...
  }
  // deferred handler (called in the main loop)
  uint8_t UART_getChar( /*common rx_buffer something else*/ ) {
    // waiting that something is there
    while(/*rx_buffer is empty*/);
    // reads value from common buffer
    // returns the read value
  }
  // deferred handler (called in the main loop)
  void UART_putChar( /*common tx_buffer something else*/ ) {
    // waiting that something is there
    while(/*tx_buffer is not empty*/);
    // pops a value from tx_buffer until everthing is sent
  }
  ```

- The comm protocol should be **binary**, and data integrity should be ensured by
  a checksum. Hence, if the communication between MCU and PC is performed sending strings,
  the project is considered not finished.

- If the project requires to setup some parameters of the server,
  those should be stored **permanently** on the board's EEPROM.

  Example: a project of Arduino includes a *server*
  (represented by the MCU board), and a *client* (running on the PC).
  These are two different programs. In the final version of the project,
  Cutecom is not allowed for receiving data from the server
  (although it can be used during the development stage).


### Projects
Here is a list of pre-approved projects.
Remember, if you have an idea for a project you can propose it to us!
Each project you will find contains several tags, regarding:
- group size for that project: `[x1]` , `[x2]` , ...
- programming language: `[C98]`, `[C++]`, `[C98 + C++]`, ...
- if it can be extended as final project (aka thesis) or to cover other courses' projects: `[thesis]`, `[labiagi]` ...
- if one wants takes a project with the `[thesis]` tag and **does not want to present it as thesis project**, the size of the group rises to 2. Still, none of the group members should present the project as bachelor thesis.

1. **Message queues in disastrOS** ---> `[x1]` `[C98]`

  implement an IPC system based on message queues in `disastrOS`
  to allow asynchronous communication between processes;
  the starting point is the version of disastrOS presented in class -
  available [here](https://gitlab.com/grisetti/sistemi_operativi_2019_20/-/tree/master/source/08_disastrOS/disastrOS_04_resources).


2. **Buddy allocator using bitmap** ---> `[x1]` `[C98]`

  the project is based on the conepts of the buddy allocator presented in class;
  you implement such a system relying only on a bitbmap to store the tree, instead of the dynamic structures (trees and lists) proposed in class -
  available [here](https://gitlab.com/grisetti/sistemi_operativi_2019_20/-/tree/master/source/06_memory_allocation/buddy_allocator).


3. **Arduino SmartHouse with web-server** ---> `[x1]` `[C98 + various]`

  The project is composed by a server and a client, organized as follows

  _Server (Arduino)_: Build a smart switch/dimmer/controller. Don't use Bluetooth dongles (it requires additional configurations) but rely on the good old UART.
  The arduino should support:
  - 8 switches/dimmers (simulate them with LED).
  - 8 ADC channels
  - 8 Digital inputs

  You should define a protocol to:
  - Configure the device (give it a `name`), set which channels are used,
    and what is the name of each channel
    (e.g. `pin_0` -> `entrance_dimmer_light`, `pin_1` -> `kitchen_light`);
    the server configuration should be stored in the board's EEPROM (permanent).
  - Send commands to the device (based on the `name`);
    upon correct execution of a command,
    the device should send back an `ack` message.
  - Receive readings from the device (based on the `name`);

  _Client (PC)_: Implement a "controller" program that can initialize the device,
  and interact with it, either from command line or through a web-server.
  Even if you choose to use a web-server, _the serial communication with the server
  should be done in `C`_. As for the pure web-server interface,
  you can choose the programming language that you prefer
  (and/or you can use external libraries like
  [libwebsockets](https://libwebsockets.org/)).
  An example of how the client shell should work is the following:
  ```sh
  #ia start the client shell
  smart_house_host <args>
  #ia now the client shell is running and we can setup/query the server
  $> set_name "kitchen_controller"
  $> set_channel_name "kitchen_controller" switch_0 "top_fridge"
  $> set_channel_value "top_fridge" 1
  $> query_channels "kitchen_controller"
  ```


4. **Arduino SmartHouse with web-server and Bluetooth** ---> `[x1]` `[C98 + various]` `[thesis]`

The specifications are the same of the base Arduino SmartHouse, however,
the communication layer will rely on Bluetooth instead of the UART.
A Bluetooth module for the Arduino is required (cost is around 5 EUR).


5. **Filesystem** ---> `[x1]` `[C98]`

  You should implement a very simple filesystem interface using binary files.
  A stub of the expected APIs is given and no external library should be used.
  Allocation should be done using linked list (LLA-FS).
  Functioning is reported below:
  - The file system reserves the first part of the file to store:
    - a linked list of free blocks
    - linked lists of file blocks
    - a single global directory
  - Blocks are of two types
    - data blocks: contain "random" information (i.e. actual data of a file)
    - directory blocks: contain a sequence of structs of type `directory_entry`,
    containing the blocks where the files in that folder start and
    whether or not those are directory themselves.

  The starting point for this project is the stub reported in the directory
  [`simple_file_system`](simple_file_system). Note that, you should provide an
  executable that tests the filesystem (and shows that it's working properly).
  A sort of naive interactive shell is very appreciated - even if it is not mandatory.


6. **Filesystem with inode** ---> `[x1]` `[C98]` `[thesis]`

  Same specification of the simple _Filesystem_ project, however,
  instead of performing block allocation and bookkeeping through linked lists,
  it should be done through `inodes` structures (IA-FS).


7. **Signals in disastrOS** ---> `[x1]` `[C98]` `[thesis]`

  You should implement an IPC mechanism.
  Each process has a `signal` variable in the PCB and each specific signal
  should be represented as a bitmask to be set in the `signal` variable.
  Each signal might have an handler. Signals of the same process share a common stack.
  Each installed signal should be managed by an appropriate context that, when terminated, returns to the context of the process.
  A signal is checked when a process enters the running state, and if so the context switch should occur
  not on the process context but on the signal context.
  While serving a signal, other signals of the same type are ignored.
  The starting point for this project is the version of disastrOS presented in class -
  available [here](https://gitlab.com/grisetti/sistemi_operativi_2019_20/-/tree/master/source/08_disastrOS/disastrOS_04_resources).


8. **Syscall in Arduino scheduler**  ---> `[x1]` `[C98]` `[thesis]`

  Extend the given Arduino scheduler to support a syscall mechanism similar to the one presented in [disastrOS](https://gitlab.com/grisetti/sistemi_operativi_2019_20/-/tree/master/source/08_disastrOS/disastrOS_04_resources).
  Implement syscalls to write/read from the serial in a kernel buffer. The transmission/reception of data will be managed by interrupts. No `ASM` code is required!
  The starting point for this project is the basic Arduino scheduler available
  [here](https://gitlab.com/grisetti/sistemi_operativi_2019_20/-/tree/master/source/05_stack_and_context_switch/avr_context_switch).


9. **Arduino MIDI keyboard** ---> `[x1]` `[C98]` `[thesis]`

  Extend the simple keyboard exercise to send MIDI packets
  when a key is pressed. A MIDI packet is composed by
  - pitch: the note to generate - represented by the key ID
  - duration: if the key remains pressed, the note sound continues.

  Note that, no physical keyboard is required: as we proposed in the class exercise,
  keys can be simulated with 2 wires.
  Still, feel free to use whatever physical button you want.
  The starting point or this project is the keyboard exercise presented in class
  and available [here](https://gitlab.com/grisetti/sistemi_operativi_2019_20/-/tree/master/source/04_hardware_avr_intro/07_keyboard_exercise).


10. **Private chat** ---> `[x2]` `[C98/C++11]`

  You should develop a private chat environment to exchange
  *text messages* between hosts. Message encryption is optional but not required.
  The project should be composed by 2 main modules:
  - Server: receives and stores each message in a sort of chat *database*.
    A very naive database would consist in a `User` struct,
    that contains everything (login credentials, chats, ...).
    Each `Chat` structure contains the actual messages.
  - Client: provides a very simple interface to select a
    receiver for our message and then write the content of the message.
    Login is required. If a receiver is not *subscribed* returns an error.

  The project should be tested with at least 3 users :)


11. **Private chat with voice messages** ---> `[x1]` `[C98/C++11]` `[thesis]`

  The specs are the same of the previous projects, however, the environment
  should support also *voice messages* - that could be send via UDP
  (not encrypted).


12. **Multi-CPU scheduler simulator**  ---> `[x1]` `[C98]` `[thesis]`

  Extend the scheduler simulator proposed in class to support multi-core CPU.
  Note that each core is allowed to run only one process at the time.
  The starting point for this project is the CPU scheduler simulator
  presented in class and available
  [here](https://gitlab.com/grisetti/sistemi_operativi_2019_20/-/tree/master/source/09_cpu_scheduling/cpu_scheduler_simulator).


13. **ROS node to communicate with Arduino board** ---> `[x1]` `[C98 + C++11]` `[thesis]` `[labiagi]`

  The student should develop a ROS node that streams the Orazio packets.
  The project is - in some sense - similar to the Arduino SmartHouse.
  In this context, the server is represented by the Orazio robot which already
  sends binary packets over the UART and allows to receive commands from a client
  (e.g. `set_param` ... ).
  You should develop a C/C++ based client that open the serials and creates an interactive shell to communicate with the robot.
  Finally, instead of sending the
  information to a webserver (as it happens now in the
  [Orazio package](https://gitlab.com/srrg-software/srrg2_orazio)
  or as you would do in the Arduino SmartHouse project),
  you should broadcast them on specific ROS topics.
  The complete specs of this project should be agreed
  both with the professor and the tutor.


### Final Comments
Rememeber that **BEFORE** you start working on a project, even if it is one of the proposed one, you must communicate your choice to the prof and the tutor (via email).
Example:

```
To: grisetti@diag.uniroma1.it, ialoise@uniroma1.it
Subject: [SO] Project Assignment
[...]
Group composition: Mario Rossi (ID: xxxxxxx), Franco Bianchi (ID: yyyyyyy), ...
Reference mail of each group component: ....
Selected project: ...
Repository link: ...
```

When you finish the project, you should send an email to both the professor
**and** the tutor. Note that you should provide also a *very brief* report
on the project (max 2 pages), reporting
- what
- how
- how to run.

You should include this document in your repository.

This project contains 3 kind of skiplists and the server and client to use skiplist in shared memory.

The skiplists are implemented by three methods:
    I. Memory. (skiplist)
    II. Disk. (safesl)
    III. Shared memory. (smsl)
    
I can be very quick, but data would be lost if the process was killed.
II can save the data even if power is off by write logs to disk file.
III will storage the data in shared memory, you can save the data after the process is killed.

Smsl has server and client code. You can run a server to storage data, other clients can manpulate the server.
By type "make", you can got the execuable file of running server.
If you want to call this server, you can include "include/smslclient.h" at your project.
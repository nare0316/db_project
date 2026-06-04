
**Server-Client Database Program**

This is a multi-threaded TCP server-client database system written in C that allows multiple clients to perform basic CRUD operations on a flat-file database.

* How It Works
> Server (server.c)

Listens on a TCP socket and spawns a new pthread for each connecting client
Stores records in a binary flat file (db.txt), where each record contains an id, name, age, and salary
Uses a mutex lock to safely handle concurrent access from multiple clients
Supports three operations: INSERT, SELECT, and DELETE

> Client (client.c)

Connects to the server via TCP and presents an interactive command-line prompt
Validates query syntax locally before sending anything to the server
Sends tokenized commands and receives/displays responses

* Supported Commands
    Command	Example:
    * INSERT:	INSERT name="Mary" age=25 salary=3000.0
    * SELECT:	SELECT name age or SELECT *
    * DELETE:	DELETE id=3

* Key Design Choices
Records are stored at fixed offsets in the file using pread/pwrite, enabling O(1) positional access
Deleted records are tombstoned (id set to -1) rather than physically removed
Syntax checking is done client-side to reduce unnecessary network traffic
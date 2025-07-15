# DocEditor
A Concurrent Document Editor in C.  Clients connect to the server process, whereby they can access, read and edit a document simultaneously.
Written in C.

Server maintains a document in a linked list data structure.  At a specified time, the document will collect all edits made by a server and edit the server.

Client connects to the server by initially sending a signal handshake.  From then, mkfifos are opened up and edits and requests are sent to and from the server.

First run:
make server
make client

1. Start the server by running it with the following command.
<TIME INTERVAL> is the time interval in milliseconds at which the versions of the document updates.
./server <TIME INTERVAL>
3. The server starts and immediately prints its Process ID (PID) to stdout:
Server PID: <pid>
4. A client joins by invoking:
./client <server_pid> <username>

From then on

the following edits, calls, requests can be made by the client.
•  INSERT <pos> <content>; Inserts textual content at the specified cursor position
and document version. Newlines are NOT permitted in this command.

• DEL <pos> <no_char>; Deletes <no_char> characters starting from the specified
cursor position. If the deletion flows beyond the end of the document, truncate at the end
of the document.

NEWLINE <pos>; Inserts newline formatting at the position.

• HEADING <level> <pos>; Inserts a heading of the given level (1 to 3) at the specified position.

• BOLD <pos_start> <pos_end>; Applies bold formatting from the start to end
cursor position.

• ITALIC <pos_start> <pos_end>; Applies italic formatting from the start to end
cursor position.

• BLOCKQUOTE <pos>; Adds blockquote formatting at the specified position.

• ORDERED_LIST <pos>; Adds ordered list formatting at the position. Any surrounding list items will be renumbered appropriately (e.g., 1, 2, 3...). For example, if an ordered
list is added to the end, it’ll find the previous number and set the new item to previous

number +1. If an ordered list is added in between existing ordered list blocks, it’ll be the
previous item + 1 and all list items will also be + 1. You must handle newlines which can
break ordered lists into two. Delete that breaks formatting does NOT need to be handled.

• UNORDERED_LIST <pos>; Applies unordered list formatting at the specified location.

• CODE <pos_start> <pos_end>; Applies inline code formatting to the given range.

• HORIZONTAL_RULE <pos>; Inserts a horizontal rule and newline at the cursor position.

• LINK <pos_start> <pos_end> <link>; Wraps the text between positions in
[] and appends the link in () to create a Markdown-style hyperlink.

DISCONNECT; Disconnects the client from the server. When the user types DISCONNECT to the client stdin, it’ll send DISCONNECT to the server. Then the server will
close and clean up the pipes. The client will detect this and exit gracefully, freeing all
resources.

Client Debugging Commands
• DOC?; Prints the entire document to the client’s terminal.

• PERM?; Requests and displays the client’s current role/permissions from the server.

• LOG?; Outputs a full log of all executed commands in order.


Server Responses
• SUCCESS; Indicates that the client’s command was successfully executed.

• Reject <reason>; Indicates the command was rejected, with a message detailing
the reason.

These responses are NOT sent immediately, but only sent as part of the broadcast message.
Server Debugging Commands

• DOC?; Prints the current document state to the server terminal.

• LOG?; Outputs a full log of all executed commands in order.

The server may reject a client’s command for several reasons. In such cases, it will respond
with a rejection message in the format:
Reject <reason>
Systems Programming Page 18 of 21
COMP2017 9017
Below is a list of possible rejection reasons:

UNAUTHORISED The client does not have sufficient permissions (i.e., write) to execute the
specified command.

INVALID_POSITION One or more provided cursor positions are outside the bounds of the
current document. For cursor ranges, the end position is equal to or lower than the start
position.

DELETED_POSITION The specified cursor position points to text that has already been deleted.

OUTDATED_VERSION The version targeted by the command is no longer accepted by the
server. Commands must reference either the current (or previous for COMP9017) document version


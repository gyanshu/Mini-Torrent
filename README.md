## Torrent like peer-to-peer file sharing system. 
For the uninitiated, bittorrent is a P2P file sharing protocol which accounts for over 25% of traffic
on the internet.
In peer-to-peer file sharing, a software client on an end-user PC requests a file, and portions of
the requested file residing on peer machines are sent to the client, and then reassembled into a
full copy of the requested file.
# Creation of torrent file at client
Normally the bittorrent systems work via sharing a “.torrent” file which contains the metadata
related to the file being shared in plaintext format. For this assignment you will develop your
own format of .torrent file called the “.mtorrent”.
It should contain the following fields:
- Tracker UR L1: The url of the tracker server1, i.e. tracker1 IP & port
- Tracker URL2: The url of the tracker server2, i.e. tracker2 IP & port
- Filename : the name with which the file would be saved on disk.
- Filesize : Complete file size
- Hash String: String whose length would be a multiple of 20. This string is a
concatenation of SHA1 hashes of each piece of file.


# Taking SHA1 hash of files:
You have to divide the file into logical “pieces” , wherein the size of each piece should be
512KB.
Example: Suppose the file size is 1024KB , then divide it into two pieces of 512KB each and
take SHA1 hash of each part, assume that the hashes are H1 & H2 then the corresponding
hash string would be H1H2 , where H1 & H2 are 20 characters each and H1H2 is 40
characters.

# Torrent tracker 

A tracker is a special type of server, one that assists in the communication between peers
using the BitTorrent protocol.
The "tracker" server for your assignment will keep track of all the available peer machines on
which the files or its parts reside.
After the initial peer-to-peer file download is started, peer-to-peer communication can continue
without the connection to a tracker.

# Torrent Clients


client has the following functionalities:
- Share files to the tracker and generate a corresponding “.mtorrent” file
- You have already done this in Part 1
- Retrieve peer information from tracker upon providing a “.mtorrent” file.
- Download files from multiple peers simultaneously, same goes for upload.
- Maintain mapping of paths of files and related .mtorrent files- this should be persistent
across runs.

## Mini Torrent Project

### HOW TO RUN?
1. Go to Tracker Folder and run the following commands: 

```sh
make
./tracker tracker_info.txt 1
```

2. Go to client folder and run :

```sh
make
./client 127.0.0.1:5000 tracker_info.txt
```

*Now you can check the following functionalities!*

- [x] Login 
- [x] Create User
- [x] Create Group
- [x] Join Group
- [x] List Groups
- [x] Accept Join Requests
- [x] List Group Requests

Yet to implement : 

- [ ] Leave Group
- [ ] Stop Sharing
- [ ] Destination Path Download
- [ ] show downloads
- [ ] Print Message Cleanup
- [ ] Make downloader as seeder when it starts downloading 
# chanchive
Modify a local archive of a 4chan thread.

## Installation
1. Compile the program
```shell
gcc -o chanchive chanchive.c
```
2. Move the binary to a directory in your $PATH
```shell
mv chanchive /usr/bin
```

## Setup
This program only modifies an already existing archive. To get a local archive of a thread you'll need to get the HTML of the thread, preferably with wget. Here's how to do it with wget:
```shell
wget -P <path/to/directory> -m -l 1 -nd -np -p -k -H -D is2.4chan.org <url>
```
This will also download all the images of the thread, so I strongly recommend you put all of this in its own directory.

## How to use it
Well, it's really simple. First, copy all the post numbers you want. Then, you have two options, either paste them all inside a file, with a newline separating each post number, or make a list and separate the post numbers by a comma. Lastly, run the damn command:
 - From a file
```shell
chanchive --file <path/to/file> <archive file>
```
 - From a list
```shell
chanchive --list <post>,<post>,<post> <archive file>
```
If you only want to add one post then you don't need a comma, or a newline if you're doing it from a file.

Once you've run the command, you'll notice a new file has appeared in your directory ending with ".new.html". You're going to want to move, delete, or rename the original archive, and then rename the new one by removing the extension. And that's it, you've successfully modified your archive to only show the posts you want.

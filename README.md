# Tra, a file system synchronizer

### [Russ Cox](http://swtch.com/~rsc)
### [William Josephson](http://www.cs.princeton.edu/~wkj/)

## What is Tra?

Tra is a file system synchronizer. Suppose you set up tra to keep your home directory on many different machines (many could be as few as two: for example, your network file server and your laptop). Tra propagates changes you make in one place to the other, but it is careful not to lose changes.

For example, suppose you are using tra to manage your home directories on machines _a_ and _b_. If you change file _f_ on machine _a_ and then synchronize, tra will copy the new _f_ to machine _b_. Similarly, if you change _f_ on _b_ and then synchronize, tra will copy the new _f_ to machine _a_. Of course, if this was all you wanted, you could almost use something like `rsync -ut`, assuming you have the first clue what `-ut` makes `rsync` do.

The neat part about tra is that it won’t ever drop a change. Continuing the example, if you change file f on both machines and then run tra will report an update/update conflict, meaning you’ve made simultaneous updates to the file, and you are left to sort out the mess. The updates are simultaneous in the sense that each change was made to a version of the file that didn’t incorporate the other change.

Tra is like Unison except that it handles arbitrary synchronization patterns. See the technical report below for more details.

## How can I obtain tra?

Tra is written using the libraries from [Plan 9 from User Space](http://swtch.com/plan9port/). Your first step is thus to install Plan 9 from User Space.

Once you’ve done that, download [tra.tgz](http://swtch.com/tra/tra.tgz) and run mk install in the src directory after unpacking it.

The current tra sources should be considered experimental. Only use if you have regular backups in case tra makes a mistake.

## Tell me more!

Tra is based on vector time pairs, which are described in MIT LCS Technical Memo MIT-LCS-TM-650 (aka MIT CSAIL Technical Report MIT-CSAIL-TR-2005-014), [File Synchronization with Vector Time Pairs](http://publications.csail.mit.edu/tmp/MIT-CSAIL-TR-2005-014.pdf).

## What if I have more questions?

Feel free to mail rsc@swtch.com.

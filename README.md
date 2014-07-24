Install
=======

KyotoPantry is a userspace file-deduplication program. You give it a list of
files and it'll index and cache 4k blocks, removing duplicates.

How it Works
------------

*Forward:* I wanted to try over-engineering a C++ project. Since I love the
actor model, I make heavy use of IPC via ZeroMQ. Serialization is handled via msgpack.

There are four main components to kyotopantry: The Gatekeeper, the Pikemen and
the Vault.

* Gatekeeper: Responsible for handing out jobs to pikemen
* Pikement: Responsible for processing files and reporting blocks to the Vault
* The Vault: Responsible for storing blocks and hashes of blocks, whether that
  block has been seen or not, etc.

When started, you have the option of telling KyotoPantry how many workers to
spawn. Workers are Pikement and they will continue to process jobs from the
gatekeeper until there are no jobs left.

Currently there are two different types of jobs that a gatekeeper can give: An
indexing job, and the actual deduplication job.

When you run kyotopantry, it will queue up every file passed in as a job. The
gatekeeper makes sure these are stored as 'indexing' jobs. When a Pikeman comes
by and asks for a job, the gatekeeper will dole it out and mark it as being
worked on, but not finished.

At this point the Pikeman is indexing the file: It loops through the entire
thing, hashing the (configurable) 4k blocks and sending them to the vault. The
vault marks down that block and it's position.

Once a Pikeman finishes indexing the file, it tells the gatekeeper and asks for
the next job. The gatekeeper will refuse to give out deduplication jobs until
all current files have been indexed, the ensures that all blocks have been seen
and hashed and exist in the Vault's block database.

Once all files have been marked as indexed (or errored out and removed from the
queue), the Gatekeeper will being to hand out deduplication jobs to Pikemen.

Deduplication jobs are basically a little more than indexing jobs, they hash and
communicate in the same way. For now I'm just going to rehash blocks (It's
fairly quick) and ask the vault to see if they exist in any file other than the
one the Pikeman is currently working on.

If they do, send the block to the vault and delete it. Hand-wave hand-wave and
the new file is copied in-place of the old file.

Installation
============

liboleg
-------

liboleg is used to handle jobs, data persistence, etc. You'll need to build that
first:

```bash
git submodule init
git submodule update
cd OlegDB
make liboleg
````

kyotopantry
-----------

kyotopantry requires a C++11-enabled compiler. You'll also need ZeroMQ and
Msgpack. Make sure you've already built liboleg.

```bash
sudo apt-get install libzmq-dev libmsgpack-dev
make
````

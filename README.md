Install
=======

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

You'll need ZeroMQ and Msgpack. Make sure you've already built liboleg.

```bash
sudo apt-get install libzmq-dev libmsgpack-dev
make
````

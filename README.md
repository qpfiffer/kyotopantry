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

kyotopantry requires a C++11-enabled compiler. You'll also need ZeroMQ and
Msgpack. Make sure you've already built liboleg.

```bash
sudo apt-get install libzmq-dev libmsgpack-dev
make
````

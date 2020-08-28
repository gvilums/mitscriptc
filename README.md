### 6.818 Skeleton Code

This repository hosts the skeleton code for 6.818, a class for building a virtual machine for a Python-like language called MITScript.

#### Dependencies

Here are the basic dependencies on Ubuntu 20.04.1 (LTS) to get the skeleton up and running:

```
sudo apt install git 
sudo apt install make
sudo apt install default-jre
sudo apt install g++
```

Here is an additional tool to aid debugging:

```
sudo apt install valgrind
```

#### Virtual Machine

Here is an Ubuntu 20.04.1 (LTS) virtual machine with these dependencies already installed:
[link](https://drive.google.com/open?id=1JSB0ueE2sXz2fG8O42Kw0JCKHKwgc2Vv).

Password: IwillgetanA

#### Updating

To pull updates from this repository, go to your own repository, and run:

```sh
git remote add upstream https://github.com/6S081/6818-fa20
```

Then, to update, you can run:

```
git fetch upstream
git merge upstream/master
```

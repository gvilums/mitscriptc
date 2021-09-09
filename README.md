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
[link](https://drive.google.com/file/d/1czHe5Ihbjm018RRX74o-Fq0PL_gxyPD6/view?usp=sharing).

Password: IwillgetanA

#### Updating

To pull updates from this repository, go to your own repository, and run:

```sh
git remote add upstream git@github.com:mit-6818/6818-fa21.git
```

Then, to update, you can run:

```
git fetch upstream
git merge upstream/main
```

# QLLM
An user interface for LLMs to chat with AIs

## How to build

### Dependencies
#### Qt Framework
To build QLLM you need Qt5 or Qt6 installed on your system. To install Qt5 on your debian or ubuntu run below command:

```bash
sudo apt-get install qtbase5-dev
```

#### Ollama
QLLM use ollama project to run models. So you need to install ollama to make QLLM works.
You can download and install ollama [here](https://ollama.com/).

#### CMake and GCC
```bash
sudo apt-get install cmake gcc g++
```

### Clone and Build
To build QLLM run below commands:

```bash
git clone 'https://github.com/Aseman-Land/qllm.git' --depth 1
cd qllm
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=release -DCMAKE_INSTALL_PREFIX=/usr ..
make install
```

## How to Run
Just run below command or create a shortcut for it:
```bash
qllm
```
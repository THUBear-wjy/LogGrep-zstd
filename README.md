This is the open-source code of paper "LogGrep: Fast and Cheap Cloud Log Storage by Exploiting both Static and Runtime Patterns" (Eurosys'2023), we use zstd as the packing method in this version.

# Paper Reference
Junyu Wei, Guangyan Zhang, Junchao Chen, Yang Wang, Weimin Zheng, Tingtao Sun, Jiesheng Wu, Jiangwei Jiang. LogGrep: Fast and Cheap Cloud Log Storage by Exploiting both Static and Runtime Patterns. in Proceedings of the 18th European Conference on Computer Systems (EuroSys’23), Roma, Italy, May 2023.

# Folder Description
## query
Query code to search on compressed logs. The entry is in CmdLineTool. Main logics are in LogStore_API.cpp and SearchAlgorithm.cpp
## compression
Compression code to compress original log files into zip files. Core logic are in main.cpp.
## example
A minimal working example for quick test. Each folder corresponds to a kind of log, inside which log files are cut into block (64MB for each).
## example_zip
Empty by default. Compressed results during quick test can be found here.
## LogHub_Seg_zip
Empty by default. Compressed results during large test can be found here.
## output
Empty by default.
## zstd-dev
zstd used by facebook to serve as the basic compression tool for LogGrep

# Supported environments
## Hardware
CPU: 2× Intel Xeon E5-2682 2.50GHz CPUs (with 16 cores)
RAM: 188GB

## OS Version
Red Hat 4.8.5 with Linux kernel 3.10.0

Ubuntu 11.3.0 with Linux kernel 5.15.0

Above are platforms we have tested suceffully, if you have trouble with other kernal version, please contact thuweijy@vip.163.com

## Compiler Version
gcc version 4.8.5 20150623

## Other requirements
Python version >= 3.6.8, use ``pip3 install --upgrade requests`` to install non-listed requirement

use ``yum groupinstall 'Development Tools`` to install other requirements.

# Compilation and quick test
## Compilation
``mkdir ./output``

``mkdir ./example_zip``

``cd ./zstd-dev/lib``

``make``

``cd ../../compression``

``make``

``cd ../``

``cd ./query``

``make``
## Quick test
``cd ./compression``

``python3 quickTest.py``

Then you can find compressed files in ./example_zip/.

``cd ./cmdline_loggrep``

``./thulr_cmdline [Compressed Folder] [QUERY]``

[QUERY] is the query statement

[Compressed Folder] is one of the folder under ./example_zip/

All testing query for quick test can be found at ./query4quicktest.txt. For example, to run query on Apache logs, you can use command as follow:

``./thulr_cmdline ../example_zip/Apache "error and Invalid URI in request"``
## Large test
Download large dataset from https://zenodo.org/record/7056802#.Yxm1RexBwq1 to ../LogHub_Seg/

First compress these log blocks.

``cd ./compression``

``python3 largeTest.py ../LogHub_Seg/``

Then you can find compressed files in ./LogHub_Seg_zip/

You can also find a compression summary under ./compression such as ./compression/Log_2022-09-21, which records whether there is an error and the compression speed.

Then query compressed logs.

``cd ./cmdline_loggrep``

``./thulr_cmdline [Compressed Folder] [QUERY]``

[QUERY] is the query statement

[Compressed Folder] is one of the folder under ./LogHub_Seg_zip/

All testing query for large test can be found at ./query4largetest.txt. For example, to run query on Hadoop logs, you can use command as follow:

``./thulr_cmdline ../LogHub_Seg_zip/Hadoop "ERROR and RECEIVED SIGNAL 15: SIGTERM and 2015-09-23"``
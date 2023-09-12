from subprocess import call
import time
import datetime

Hadoop_Target = [
    "41447",
    "44433",
    "4144672",
    "389",
    "Receiving",
    "terminating",
    "duration: 3251285",
    "Retrying connect to server",
    "BP-108841162-10.10.34.11-1440074360971", 
    "blk_1075557039_1816215",
    "HAS_DOWNSTREAM_IN_PIPELINE",
    "c40d0a4fbe6b"
]

fw = open("./Log_{}".format(datetime.date.today()), "w")
for cnt, tar in enumerate(Hadoop_Target):
    call("echo 3 > /proc/sys/vm/drop_caches", shell=True)
    command = "./thulr_cmdline ../LogData/LogGrepOut/Hadoop/ \"" + tar + "\" >> Result/Hadoop_" + str(cnt) + ".res"
    print("Testing: " + command)
    time1 = time.time()
    call(command, shell=True)
    time2 = time.time()
    tr = "Hadoop_" + str(cnt) + ".res" + " Cost: " + str(time2 - time1)
    print(tr)
    fw.write(tr + "\n")

# Spark_Target = [
#     "273097",
#     "141701",
#     "129041",
#     "138326",
#     "Added",
#     "PythonRunner",
#     "size: 4.0 MB",
#     "2076 bytes result sent to driver",
#     "sparkExecutorActorSystem@mesos-slave-26", 
#     "attempt_201706091754_1961_m_000031_78507",
#     "broadcast_6_piece180",
#     "b15ce3035de0"
# ]

# for cnt, tar in enumerate(Spark_Target):
#     call("echo 3 > /proc/sys/vm/drop_caches", shell=True)
#     command = "./thulr_cmdline ../LogData/LogGrepOut/Spark/ \"" + tar + "\" >> Result/Spark_" + str(cnt) + ".res"
#     print("Testing: " + command)
#     time1 = time.time()
#     call(command, shell=True)
#     time2 = time.time()
#     print("Spark_" + str(cnt) + ".res" + " Cost: " + str(time2 - time1))
#     fw.write("Spark_" + str(cnt) + ".res" + " Cost: " + str(time2 - time1) + "\n")

# Thunderbird_Target = [
#     "16646",
#     "65894",
#     "1667085",
#     "1671928",
#     "SCSI",
#     "synchronized",
#     "ib_sm_sweep.c:1516",
#     "Configuration caused by some ports in INIT state",
#     "6:05:16", 
#     "Thunderbird_B3",
#     "5ad00000",
#     "0x00000000000fd6b0"
# ]

# for cnt, tar in enumerate(Thunderbird_Target):
#     call("echo 3 > /proc/sys/vm/drop_caches", shell=True)
#     command = "./thulr_cmdline ../LogData/LogGrepOut/Thunderbird/ \"" + tar + "\" >> Result/Thunderbird_" + str(cnt) + ".res"
#     print("Testing: " + command)
#     time1 = time.time()
#     call(command, shell=True)
#     time2 = time.time()
#     print("Thunderbird_" + str(cnt) + ".res" + " Cost: " + str(time2 - time1))
#     fw.write("Thunderbird_" + str(cnt) + ".res" + " Cost: " + str(time2 - time1) + "\n")

fw.close()

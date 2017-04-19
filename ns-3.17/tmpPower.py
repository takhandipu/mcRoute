import os
import sys
'''
orig_stdout = sys.stdout
f = file('1.txt', 'w')
sys.stdout = f
# os.system('NS_GLOBAL_VALUE="RngRun=3" ./waf --run "mine --numNodes=2 --radioPerNode=1"')
for numNodes in range(2,22,2):
    for radioPerNode in range(1,11):
        for run in range(1,101):
            os.system('./waf --run "mine --numNodes='+`numNodes`+' --radioPerNode='+`radioPerNode`+' --RngRun='+`run`+'" >> RandomWalk2dMobilityModel.txt')
            # print ('./waf --run "mine --numNodes='+`numNodes`+' --radioPerNode='+`radioPerNode`+' --RngRun='+`run`+'" >> RandomWalk2dMobilityModel.txt')
        pass
sys.stdout = orig_stdout
f.close()
'''
orig_stdout = sys.stdout
f = file('1.txt', 'w')
sys.stdout = f
# os.system('NS_GLOBAL_VALUE="RngRun=3" ./waf --run "mine --numNodes=2 --radioPerNode=1"')
powerRanges = [20, 23, 25, 26, 27]
for dBm in powerRanges:
    for run in range(1, 51):
        os.system('./waf --run "scratch/mcRoute --seed='+`run`+' --dBm='+`dBm`+'" >> 2016_12_03_power.txt')
pass
sys.stdout = orig_stdout
f.close()

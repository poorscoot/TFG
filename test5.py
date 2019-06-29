from subprocess import call
import os
import time

my_path = os.path.abspath(os.path.dirname(__file__))


os.chdir(os.path.join(my_path, 'TestCasesInter'))

cmd="./test5"
call(cmd, shell=True)

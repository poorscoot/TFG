from subprocess import call
import os
import time

my_path = os.path.abspath(os.path.dirname(__file__))


os.chdir(os.path.join(my_path, 'TestCasesIntra'))

cmd="./test18"
call(cmd, shell=True)

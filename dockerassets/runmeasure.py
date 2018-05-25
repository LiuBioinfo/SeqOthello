import time
import threading
import datetime as DT
import logging
import argparse
import subprocess
logger = logging.getLogger(__name__)

logging.basicConfig(level=logging.DEBUG,
                    format='[%(asctime)s %(threadName)s] %(message)s',
                    datefmt='%H:%M:%S')


def do_work(id, stop):
    logger.info("starting to check subprocess {}".format(id))
    logger.info(subprocess.check_output(['ps', '-p', '{}'.format(id), '-o' ,'rss,pid,cmd,pcpu']))
    while True:
        try:
            logger.info(subprocess.check_output(['ps', '-p', '{}'.format(id), '-o' ,'rss,pid,pcpu','h']))
        except subprocess.CalledProcessError:
            pass
        if stop():
            logger.info("I am thread watching on pid {}. stopping".format(id))
            break
        time.sleep(1)



if __name__ == '__main__':
    parser = argparse.ArgumentParser('run and measure memory')
    parser.add_argument('--cmd', nargs = 1, required = True)
    parser.add_argument('--log', nargs = 1, required = True)
    arg= parser.parse_args()
    print(arg)
    stop_threads = False
    workers = []
    f = open(arg.log[0], "w")
    p = subprocess.Popen(arg.cmd[0].split(' '), stdout=f)
    tmp = threading.Thread(target=do_work, args=(p.pid, lambda: stop_threads))
    print(p.pid)
    workers.append(tmp)
    tmp.start()
    p.wait()
    stop_threads = True
    for worker in workers:
        worker.join()

    print('Finis.')



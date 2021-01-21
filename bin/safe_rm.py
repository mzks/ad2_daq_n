#!/usr/bin/python3

import os, sys
import glob
import subprocess
import argparse


def safe_rm(target_dir='20200705', verbose=1, dry_run=True):

    data_dir = '/home/pi/ad2_daq/data/'
    copied_host = 'data_server'
    copied_dir = 'ad2/data/'
    files = glob.glob(data_dir + target_dir + '/*')

    txts = glob.glob(data_dir + target_dir + '/*.txt')
    if len(txts) == 0:
        print('No files')
        return

    last_txts = sorted(txts, reverse=True)[0]

    ls_out = subprocess.check_output(['ssh', copied_host, 
        'ls', copied_dir+target_dir]).decode('utf-8')
    rm_cmd = ['rm', '-rf']


    for file in files:

        local_filename = os.path.basename(file)
        if file == last_txts:
            print('keep the latest config file', file)
        elif ls_out.find(local_filename) >= 0: # find the file
            rm_cmd.append(file)
        else:
            print('Not copied yet:', local_filename)


    if verbose > 1: print_cmd(rm_cmd)
    if dry_run == False: subprocess.run(rm_cmd)


def print_cmd(cmd):
    for c in cmd:
        print(c, end=' ')
    print()



if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Safe remover', add_help=True)
    parser.add_argument('-v', '--verbose', help='verbose level', default=1, type=int)
    parser.add_argument('-t', '--target_dir', help='Target directory', default='test1', type=str)
    parser.add_argument('-d', '--dryrun', help='Dry run', action='store_true')

    args = parser.parse_args()
    safe_rm(args.target_dir, args.verbose, args.dryrun)


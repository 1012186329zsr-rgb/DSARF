import numpy as np
import os


def parse_path_log(file_name):
    with open(file_name, 'r') as f:
        chip_num, degree = list(map(int, f.readline().split(',')))
        res = []
        for i in range(chip_num):
            res.extend(list(map(int, f.readline().split(','))))
        assert len(res) == chip_num * degree
    return res


if __name__ == '__main__':
    file_name = 'temp/gpt_xl_t1000/path_log/0.txt'
    print(parse_path_log(file_name))
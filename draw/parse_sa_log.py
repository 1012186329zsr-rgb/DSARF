import re


def parse_sa_log(file_name: str):
    init_avg, init_var, init_balance = 0, 0, 0
    temperature = []
    delta_cost = []
    cost = []
    taken = []
    avg = []
    var = []
    balance = []
    with open(file_name, 'r') as f:
        line = f.readline()
        while line:
            if '-------- iter' in line:
                res = re.findall(r'-?\d+\.?\d*', line)
                if len(res) < 8 or len(res) > 9:
                    print('Error' + line)
                    print(file_name)
                    # continue
                temperature.append(float(res[1]))
                delta_cost.append(float(res[2]))
                cost.append(float(res[3]))
                taken.append(int(res[4]))
                avg.append(float(res[5]))
                var.append(float(res[6]))
                balance.append(float(res[7]))
            elif '--- initial' in line:
                res = re.findall(r'-?\d+\.?\d*', line)
                assert len(res) == 3
                init_avg, init_var, init_balance = float(
                    res[0]), float(res[1]), float(res[2])
            line = f.readline()
    return init_avg, init_var, init_balance, temperature, delta_cost, cost, taken, avg, var, balance


if __name__ == '__main__':
    parse_sa_log('topology/temp/log.txt')

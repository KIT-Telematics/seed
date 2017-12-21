import csv
import matplotlib.pyplot as plt
import sys

def main():
    with open(sys.argv[1]) as f:
        x = []
        y = []

        for row in csv.reader(f):
            assert len(row) == 2
            x.append(float(row[0]))
            y.append(float(row[1]))

        plt.plot(x, y, marker='o')
        plt.show()

if __name__ == '__main__':
    main()

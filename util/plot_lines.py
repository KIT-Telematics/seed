import matplotlib.pyplot as plt
import sys

def main():
    with open(sys.argv[1]) as f:
        y = [float(l) for l in f]
        plt.plot(y, marker='o')
        plt.show()

if __name__ == '__main__':
    main()

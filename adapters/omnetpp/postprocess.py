import csv
import itertools
import json
import os
import sys

sca_types = {
    "fct": "FCT"
}

vec_types = {
    "txPk:vector(binavg(packetBits))": "TxBits",
    "rxPk:vector(binavg(packetBits))": "RxBits"
}

def main():
    with open(os.path.join(sys.argv[1], "mapping.json")) as f:
        mapping = json.load(f)

    with open(os.path.join(sys.argv[1], "General-0.sca")) as f:
        files = {}
        for sca_type in sca_types.keys():
            out = os.path.join(sys.argv[2], sca_type + ".csv")
            _f = open(out, "w")
            files[sca_type] = (csv.writer(_f), _f)

        for line in f:
            split = line.split()
            if len(split) == 0:
                pass
            elif split[0] == "scalar":
                if split[2] in sca_types:
                    files[split[2]][0].writerow([split[1], split[3]])

        for _, _f in files.values():
            _f.close()

    with open(os.path.join(sys.argv[1], "General-0.vec")) as f:
        assert next(f).startswith("version")
        assert next(f).startswith("run")

        watch = {}

        for line in f:
            split = line.split()
            if len(split) == 0:
                pass
            elif split[0] == "attr":
                pass
            elif split[0] == "vector":
                if split[3] in vec_types:
                    assert split[4] == "ETV"
                    out = os.path.join(sys.argv[2], split[2] + "." + vec_types[split[3]] + ".csv")
                    _f = open(out, "w")
                    watch[split[1]] = (csv.writer(_f), _f)
            else:
                rem = line
                break

        for line in itertools.chain([rem], f):
            split = line.split()
            if split[0] in watch:
                watch[split[0]][0].writerow([split[2], split[3]])

        for _, _f in watch.values():
            _f.close()

if __name__ == '__main__':
    main()

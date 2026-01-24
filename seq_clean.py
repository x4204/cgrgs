import sys

_, path = sys.argv
with open(path) as file:
    for line in file:
        line = line.strip()
        if len(line) == 0 or line[0] == '>':
            continue
        sys.stdout.write(line.lower())

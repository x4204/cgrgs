encoding = ['A', 'C', 'G', 'T', 'a', 'c', 'g', 't']
rest = set(chr(n) for n in range(0, 256)) - set(encoding)
encoding += sorted(rest)

for i, n in enumerate(encoding):
    print(f'[{ord(n):3d}] = {i%4},')

import matplotlib.pyplot as plt
import numpy as np

values = []


def const_move(start, stop, up, m):
    ch = 0 if up else 1
    step = 0.02
    steps = int((stop - start) / step / 2)
    min = 3
    _max = 9
    max = _max * m
    max = _max if max > _max else max
    max = min if max < min else max
    print(max, min)
    curr = 0

    for i in range(steps):
        curr = min + (max - min) * (i / steps)
        values.append(curr if up else -curr)
        # values.append(curr)
    for i in range(steps):
        curr = max - (max - min) * (i / steps)
        values.append(curr if up else -curr)
        # values.append(curr)


def shaking():
    # time.sleep(5 / 60)
    direction = False  # down

    timestamps = [
        [5, 15, 1],  # down
        [15, 26, 1],  # up
        [26, 34, 0.5],
        [34, 41, 0.5],
        [41, 48, 0.4],
        [48, 53, 0.4],
        [53, 62, 0.35],
        [62, 67, 0.35],
        [67, 90, 0.35],
    ]

    i = 0

    while True:
        k = 2
        const_move(timestamps[i][0] / 60 * k, timestamps[i][1] / 60 * k, direction, timestamps[i][2])
        direction = not direction
        i += 1
        if i >= len(timestamps): break


# shaking()
# window =3
# values = np.convolve(values, np.ones(window)/window, mode='valid')


# Plot

# Read line by line
values = []
with open("data/hs_trajectory_409_8.txt", "r") as f:
    for line in f:
        t = []
        for v in line.strip().split(" "):
            t.append(float(v))
        values.append(t)

plt.figure(figsize=(12, 6))
plt.plot(values, marker='o', markersize=2, linewidth=1)
plt.grid(True)

plt.show()

import matplotlib.pyplot as plt
import numpy as np
import os


ems = []


def const_move(start, stop, up, m):
    ch = 0 if up else 1
    step = 0.02
    steps = int((stop - start) / step / 2)
    min = 3
    _max = 9
    max = _max * m
    max = _max if max > _max else max
    max = min if max < min else max
    # print(max, min)
    curr = 0

    for i in range(steps):
        curr = min + (max - min) * (i / steps)
        ems.append(curr - min if up else -curr + min)
    for i in range(steps):
        curr = max - (max - min) * (i / steps)
        ems.append(curr - min if up else -curr + min)


def shaking():
    # time.sleep(5 / 60)
    direction = False  # down

    timestamps = [
        [0, 14, 1],  # down
        [14, 26, 1],  # up
        [26, 33, 0.5],
        [33, 40, 0.5],
        [40, 47, 0.4],
        [47, 52, 0.4],
        [52, 62, 0.35],
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


shaking()
window = 5
# ems = np.convolve(ems, np.ones(window)/window, mode='valid')


# Plot

# Read line by line
bone = []
# with open("data/bone_rotation.txt", "r") as f:
with open("data/handpos.txt", "r") as f:
    for line in f:
        t = []
        for v in line.strip().split(" "):
            t.append(float(v))
        bone.append(
                    + t[1]
                    # + t[2]
                    # + t[3]
                    )
bone = bone[3:]
bone = np.array(bone)
bone -= np.mean(bone)
bone /= np.max(np.abs(bone))

arr = bone[:100]
maxima = []
minima = []

for i in range(1, len(arr) - 1):
    if arr[i] > arr[i - 1] and arr[i] > arr[i + 1]:
        maxima.append((i, arr[i]))
    if arr[i] < arr[i - 1] and arr[i] < arr[i + 1]:
        minima.append((i, arr[i]))

# print("Maxima:", maxima)
# print("Minima:", minima)


ems = np.diff(bone)


def scale(arr):
    a, b = -9, 9
    return a + (arr - arr.min()) * (b - a) / (arr.max() - arr.min())

# print(ems[:80])

plt.figure(figsize=(12, 6))


entries = os.listdir("data")
min_lines = 140
movements = [0.] * 140
shakes = 0
for entry in entries:
    if entry.startswith("vibro_hs_trajectory") and entry.endswith(".txt"):
        movement = []
        print("Processing:", entry)

        with open(os.path.join("data", entry), "r") as f:
            for line in f:
                values = line.strip().split(" ")
                movement.append(
                    # float(values[0])
                    + float(values[1])
                    # + float(values[2])
                )
        if len(movement) < min_lines:
            continue
        movements = np.add(movements, movement[:min_lines])
        shakes += 1

movements -= np.mean(movements)
movements = movements / shakes
movements /= np.max(np.abs(movements))

anim_steps = 80
x1 = np.linspace(0, 100, anim_steps)
x2 = np.linspace(0, 100, len(movements))
# ems += 0.0001
tmp = []
a_max = np.max(ems)
a_min = 0
b_max = 9
b_min = 3

for value in ems:
    scaled_value = 0
    if value >= 0:
        scaled_value = b_min + (value - a_min) * (b_max - b_min) / (a_max - a_min)
    else:
        scaled_value = -b_min + (value - a_min) * (b_max - b_min) / (a_max - a_min)

    tmp.append(scaled_value)

ems = np.array(tmp)

plt.plot(x1, bone[:anim_steps], label='Animation', marker='.', markersize=2, linewidth=1)
# plt.plot(bone[:80], label='Animation', marker='.', markersize=2, linewidth=1)
# plt.plot(ems[:80], label='EMS', marker='.', markersize=2, linewidth=1)
plt.plot(x2, movements, label="Average shakes", marker='.', markersize=2, linewidth=1)

# plt.plot(x1[:-1], np.diff(bone[:anim_steps]), label='Diff', marker='.', markersize=2, linewidth=1, color='green')
# plt.plot(x2[:-1], np.diff(movements), label="Daverage_shakes", marker='.', markersize=2, linewidth=1)

# special_x = [14, 25, 33, 40, 47, 52]
# for sx in special_x:
#     plt.axvline(x=sx, color="black", linewidth=0.5, linestyle="--")

ems = []
with open("ems.txt", "r") as f:
    for line in f:
        ems.append(float(line.strip()))

ems = np.array(ems)
# ems /= np.max(np.abs(ems))

ems1 = [x if x > 0 else 0 for x in ems]

ems2 = [-x if x < 0 else 0 for x in ems]

ems_d = np.diff(ems)
ems3 = [x if x > 0 else -x for x in ems_d]
ems3 = [4 if x > 3 else x for x in ems3]

# plt.plot(ems1, label='channel 1 (Byceps)', marker='.', markersize=2, linewidth=1)
# plt.plot(ems2, label='channel 2 (Triceps)', marker='.', markersize=2, linewidth=1)
# plt.plot(ems3, label='channel 3 (Flexor)', marker='.', markersize=2, linewidth=1)

plt.grid(True)
plt.legend()



plt.show()



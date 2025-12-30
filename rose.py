import numpy as np
import matplotlib.pyplot as plt
from matplotlib.projections.polar import PolarAxes
from matplotlib.projections import register_projection
from matplotlib.spines import Spine
from matplotlib.path import Path
from matplotlib.transforms import Affine2D


class Plotter():

    def __init__(self):
        pass

    def load_data(self, filepath):
        self.categories = ["Involvement", "Comfort", "Experienced Realism", "Usability", "Acceptance"]

        self.hs_categories = ["Naturalness", "Immersion", "Realism", "Agency", "Acceptance"]

        # file = "2.333333333	4.666666667	2.333333333	2.333333333	2.333333333	3.333333333	4.333333333	3	3.333333333	3.666666667	3.666666667	3.666666667	3.666666667	3.666666667	4.333333333	2.666666667	3	2.666666667	3.333333333	2.666666667	4.666666667	4.333333333	4.333333333	2.333333333	4	3.666666667	4	4.666666667	4.666666667	2.666666667"
        # file = "2.444444444	3.888888889	2.444444444	2.555555556	3.555555556	3.111111111	4	3.555555556	3.555555556	4	4	4	4.444444444	4	4.444444444	2.777777778	3.333333333	3	3.777777778	3.222222222	4.333333333	4.555555556	4.555555556	2.888888889	4	4.333333333	4	4.666666667	4.666666667	3.625	3"
        file = "2.416666667	3.833333333	2.416666667	2.5	3.75	3	3.833333333	3.416666667	3.416666667	4.083333333	3.833333333	3.666666667	4.333333333	3.916666667	4.416666667	2.75	3.166666667	2.916666667	3.75	3.333333333	4.166666667	4.333333333	4.333333333	2.666666667	3.916666667	3.916666667	3.5	4.333333333	4.5	3.555555556	3.166666667"
        file = file.split("\t")

        self.no_feedback = [float(file[0]), float(file[1]), float(file[2]), float(file[3]), float(file[4])]
        self.vibration = [float(file[5]), float(file[6]), float(file[7]), float(file[8]), float(file[9])]
        self.ems = [float(file[10]), float(file[11]), float(file[12]), float(file[13]), float(file[14])]

        self.vibro_handshake = [float(file[15]), float(file[16]), float(file[17]), float(file[18]), float(file[19])]
        self.ems_handshake = [float(file[20]), float(file[21]), float(file[22]), float(file[23]),
                              float(file[24]), float(file[25])]

        self.ems_acceptance = [float(file[26]), float(file[27]), float(file[28]), float(file[29])]

    def plot(self):
        # Angles
        N = len(self.categories)
        angles = np.linspace(0, 2 * np.pi, N, endpoint=False).tolist()
        angles += angles[:1]  # close the loop

        # Values closed
        self.ems += self.ems[:1]
        self.vibration += self.vibration[:1]
        self.no_feedback += self.no_feedback[:1]

        # Plot
        fig, ax = plt.subplots(figsize=(5, 5), subplot_kw=dict(polar=True))

        ax.set_theta_offset(np.pi / 2)
        ax.set_theta_direction(-1)
        ax.set_thetagrids(np.degrees(angles[:-1]), self.categories)
        labels = ax.get_xticklabels()

        for lbl in labels:
            lbl.set_y(lbl.get_position()[1] - 0.15)

        # Plot series
        ax.plot(angles, self.no_feedback, color="tab:green", linewidth=2, label="No feedback")
        ax.plot(angles, self.vibration, color="tab:red", linewidth=2, label="Vibration")
        ax.plot(angles, self.ems, color="tab:blue", linewidth=2, label="EMS")

        # ax.yaxis.set_tick_params(labelsize=10)
        # ax.set_rgrids([1, 2, 3, 4, 5], angle=0)
        ax.set_ylim(0, 5)
        ax.set_yticklabels([])

        ax.grid(alpha=0.7)
        outer_circle = ax.spines['polar']

        outer_circle.set_linewidth(0.5)  # reduce thickness
        outer_circle.set_edgecolor("gray")  # change color
        outer_circle.set_alpha(0.7)

        # Title and legend
        ax.legend(loc="upper right", bbox_to_anchor=(1.2, 1.1))

        for r in [1, 2, 3, 4, 5]:
            ax.text(np.pi, r, str(r),
                    ha='center', va='bottom',
                    fontsize=9, color="black",
                    zorder=10)

        plt.tight_layout()
        plt.show()

    def plot_handshake(self):
        # Angles
        N = len(self.hs_categories)
        angles = np.linspace(0, 2 * np.pi, N, endpoint=False).tolist()
        angles += angles[:1]  # close the loop

        # Values closed
        self.vibro_handshake += self.vibro_handshake[:1]
        self.ems_handshake[4] = (self.ems_handshake[4] + self.ems_handshake[5]) / 2
        self.ems_handshake = self.ems_handshake[:-1]
        self.ems_handshake += self.ems_handshake[:1]

        # Plot
        fig, ax = plt.subplots(figsize=(5, 5), subplot_kw=dict(polar=True))

        ax.set_theta_offset(np.pi / 2)
        ax.set_theta_direction(-1)
        ax.set_thetagrids(np.degrees(angles[:-1]), self.hs_categories)
        labels = ax.get_xticklabels()

        for lbl in labels:
            lbl.set_y(lbl.get_position()[1] - 0.15)

        # Plot series
        ax.plot(angles, self.vibro_handshake, color="tab:green", linewidth=2, label="Vibro handshake")
        ax.plot(angles, self.ems_handshake, color="tab:red", linewidth=2, label="EMS handshake")

        # ax.yaxis.set_tick_params(labelsize=10)
        # ax.set_rgrids([1, 2, 3, 4, 5], angle=0)
        ax.set_ylim(0, 5)
        ax.set_yticklabels([])

        ax.grid(alpha=0.7)
        outer_circle = ax.spines['polar']

        outer_circle.set_linewidth(0.5)  # reduce thickness
        outer_circle.set_edgecolor("gray")  # change color
        outer_circle.set_alpha(0.7)

        # Title and legend
        ax.legend(loc="upper right", bbox_to_anchor=(1.2, 1.1))

        for r in [1, 2, 3, 4, 5]:
            ax.text(np.pi, r, str(r),
                    ha='center', va='bottom',
                    fontsize=9, color="black",
                    zorder=10)

        plt.tight_layout()
        plt.show()

if __name__ == "__main__":
    plotter = Plotter()
    plotter.load_data("data/response.csv")
    # plotter.plot()
    plotter.plot_handshake()

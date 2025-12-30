import numpy as np
from enum import Enum
import scipy.stats as stats
import pandas as pd
import pingouin as pg


class CUBE(Enum):
    VISUAL = 0
    HAPTIC = 1
    EMS = 2


class PARAM(Enum):
    INVOLVEMENT = 0
    COMFORT = 1
    REALISM = 2
    USABILITY = 3
    ACCEPTANCE = 4


class HANDSHAKE_PARAMS(Enum):
    NATURALNESS = 0
    IMMERSION = 1
    REALISM = 2
    AGENCY = 3
    ACCEPTANCE = 4


data = [
    [4, 4, 4, 4, 3, 5, 4, 4, 5, 5, 3, 4, 4, 4, 4, 4, 4, 3, 3, 3, 5, 5, 5, 4, 5, 4, 3, 4, 5, 3, 3],
    [1, 5, 1, 1, 1, 2, 5, 2, 2, 2, 3, 3, 2, 2, 4, 1, 1, 1, 3, 1, 4, 3, 3, 1, 2, 2, 4, 5, 4, 2, 1],
    [2, 5, 2, 2, 3, 3, 4, 3, 3, 4, 5, 4, 5, 5, 5, 3, 4, 4, 4, 4, 5, 5, 5, 2, 5, 5, 5, 5, 5, 3, 2],
    [3, 5, 1, 1, 5, 4, 4, 5, 5, 5, 4, 4, 5, 5, 5, 3, 3, 3, 5, 4, 4, 5, 5, 4, 5, 5, 5, 5, 5, 5, 4],
    [3, 5, 4, 4, 5, 5, 5, 5, 4, 5, 5, 5, 5, 4, 5, 4, 4, 4, 4, 5, 4, 5, 5, 2, 3, 5, 4, 5, 4, 3, 3],
    [4, 2, 4, 4, 5, 3, 4, 4, 4, 4, 4, 4, 4, 3, 5, 3, 5, 4, 4, 5, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5, 4],
    [2, 3, 2, 2, 2, 1, 3, 3, 2, 3, 4, 4, 5, 4, 4, 1, 2, 2, 3, 2, 4, 4, 4, 3, 4, 3, 2, 4, 5, 3, 3],
    [2, 3, 1, 1, 3, 3, 3, 2, 3, 3, 4, 4, 5, 4, 4, 4, 5, 4, 4, 3, 5, 5, 5, 4, 5, 5, 4, 5, 5, 5, 4],
    [1, 3, 3, 4, 5, 2, 4, 4, 4, 5, 4, 4, 5, 5, 4, 2, 2, 2, 4, 2, 5, 5, 5, 2, 3, 5, 4, 4, 4, 3, 3],
    [3, 2, 2, 2, 4, 2, 2, 2, 2, 4, 2, 2, 2, 2, 4, 3, 3, 3, 3, 4, 2, 2, 2, 2, 4, 2, 1, 2, 4, 4, 4],
    [2, 4, 3, 3, 4, 3, 4, 4, 3, 4, 4, 2, 5, 4, 4, 2, 2, 2, 3, 3, 5, 4, 4, 2, 3, 2, 2, 4, 4, 3, 3],
    [2, 5, 2, 2, 5, 3, 4, 3, 4, 5, 4, 4, 5, 5, 5, 3, 3, 3, 5, 4, 4, 5, 5, 2, 4, 4, 3, 4, 4, 4, 4]
]

data = np.array(data)


# print(a[:, 1])
# print(CUBE.EMS.value * 5 + PARAM.REALISM.value)
def test_touch():
    print("Touch Feedback Study Statistical Analysis")
    for param in PARAM:
        visual = data[:, CUBE.VISUAL.value * 5 + param.value]
        haptic = data[:, CUBE.HAPTIC.value * 5 + param.value]
        ems = data[:, CUBE.EMS.value * 5 + param.value]
        print(f"{param.name}")
        stat, p_value = stats.friedmanchisquare(visual, haptic, ems)
        print(f"  Friedman test: stat={stat:.4f}, p-value={p_value:.4f}")
        if p_value < 0.05:
            pairs = [
                ("EMS vs Vibration", ems, haptic),
                ("EMS vs No Feedback", ems, visual),
                ("Vibration vs No Feedback", haptic, visual)
            ]

            alpha = 0.05
            bonferroni_alpha = alpha / len(pairs)

            for name, a, b in pairs:
                stat, p = stats.wilcoxon(a, b, method="exact")
                # print(f"\t{name}: p = {p:.4f} (significant if < {bonferroni_alpha:.4f})")
                if p < bonferroni_alpha:
                    print(f"  Significant difference between {name}")
                else:
                    print(f"  No significant difference between {name}")


def test_touch_correction():
    N = 12
    print("Touch Feedback Study Statistical Analysis with Pingouin")
    for param in PARAM:
        print(f"{param.name}")
        visual = data[:, CUBE.VISUAL.value * 5 + param.value]
        haptic = data[:, CUBE.HAPTIC.value * 5 + param.value]
        ems = data[:, CUBE.EMS.value * 5 + param.value]

        df = pd.DataFrame({
            'subject': list(range(1, N + 1)) * 3,
            'condition': [*(['VISUAL'] * N), *(['HAPTIC'] * N), *(['EMS'] * N)],
            'score': [*visual, *haptic, *ems]

        })

        anova = pg.rm_anova(dv='score', within='condition', subject='subject',
                            data=df, detailed=True)

        print(anova)
        # print(anova.loc[0, 'p-unc'])
        if anova.loc[0, 'p-unc'] < 0.05:
            print("  Significant differences found, performing post-hoc tests...")
            posthocs = pg.pairwise_tests(dv='score', within='condition', subject='subject',
                                         data=df, padjust='bonferroni', parametric=False)
            print(posthocs[['A', 'B', 'p-unc', 'p-corr']])
        else:
            print("  No significant differences found.")


def test_handshake():
    print("Handshake Study Statistical Analysis")
    for param in HANDSHAKE_PARAMS:
        print(param.name)
        haptic = data[:, 15 + param.value]
        ems = data[:, 20 + param.value]
        stat, p_value = stats.wilcoxon(ems, haptic)
        print(f"  Wilcoxon test: stat={stat:.4f}, p-value={p_value:.4f}")
        if p_value < 0.05:
            print(f"  Significant difference between EMS and Vibration for {param.name}")


if __name__ == "__main__":
    # test_touch()
    test_handshake()
    # test_touch_correction()

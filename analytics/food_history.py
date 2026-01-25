import argparse
from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt

# ---- Targets (constant over time) ----
KCAL_MIN, KCAL_MAX, KCAL_TGT = 1750, 2150, 1950
PROT_MIN, PROT_MAX, PROT_TGT = 110, 140, 125
FIB_MIN,  FIB_MAX,  FIB_TGT  = 25,  40,  32


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--csv", required=True, help="Path to food_history.csv")
    ap.add_argument("--out", required=True, help="Path to output PNG")
    args = ap.parse_args()

    csv_path = Path(args.csv)
    out_path = Path(args.out)

    if not csv_path.exists():
        raise SystemExit(f"CSV not found: {csv_path}")

    df = pd.read_csv(csv_path)
    if df.empty:
        raise SystemExit("CSV is empty, nothing to plot.")

    df["date"] = pd.to_datetime(df["date"], format="%Y-%m-%d", errors="raise")
    df = df.sort_values("date")

    has_fiber = "fiber" in df.columns

    # Ensure numeric
    for col in ["kcal", "protein"] + (["fiber"] if has_fiber else []):
        df[col] = pd.to_numeric(df[col], errors="coerce").fillna(0.0)

    # Mask "empty" days
    if has_fiber:
        nonzero = ~((df["kcal"] == 0) & (df["protein"] == 0) & (df["fiber"] == 0))
    else:
        nonzero = ~((df["kcal"] == 0) & (df["protein"] == 0))

    dff = df.loc[nonzero].copy()

    fig, ax1 = plt.subplots(figsize=(12, 6))
    ax2 = ax1.twinx()

    # ----- Axis limits (FORCED) -----
    ax1.set_ylim(0, 2500)   # kcal
    ax2.set_ylim(0, 150)    # protein / fiber

    # ----- Target bands (constant) -----
    ax1.axhspan(KCAL_MIN, KCAL_MAX, color="tab:red", alpha=0.08)
    ax2.axhspan(PROT_MIN, PROT_MAX, color="tab:blue", alpha=0.08)
    ax2.axhspan(FIB_MIN,  FIB_MAX,  color="tab:green", alpha=0.08)

    # ----- Data curves -----
    # kcal
    l_kcal, = ax1.plot(
        dff["date"], dff["kcal"],
        color="tab:red", linewidth=2, label="Energy (kcal)"
    )
   

    # protein
    l_prot, = ax2.plot(
        dff["date"], dff["protein"],
        color="tab:blue", linewidth=2, linestyle="--", marker="o",
        label="Protein (g)"
    )

    # fiber
    l_fib, = ax2.plot(
        dff["date"], dff["fiber"],
        color="tab:green", linewidth=2, linestyle="--", marker="x",
        label="Fiber (g)"
    )


    # Combine legends from both axes
    lines = [
    l_kcal,
    l_prot,
    l_fib,
    ]

    labels = [l.get_label() for l in lines]

    fig.legend(
        lines, labels,
        loc="lower center",
        ncol=3,
        frameon=False,
        bbox_to_anchor=(0.5, -0.15)
    )

    plt.subplots_adjust(bottom=0.25)


    plt.title("Daily intake vs targets")

    plt.tight_layout(rect=[0, 0.12, 1, 1])  # laisse de la place en bas
    plt.savefig(out_path, dpi=150, bbox_inches="tight")

    plt.close()

    print(f"Wrote {out_path}")


if __name__ == "__main__":
    main()

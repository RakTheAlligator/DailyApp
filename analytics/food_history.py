import argparse
from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt

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
    
    # --- Support optional columns
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

    # kcal: left axis
    ax1.plot(
        dff["date"],
        dff["kcal"],
        color="tab:red",
        linewidth=2,
        label="Energy (kcal)"
    )

    # protein: right axis
    ax2.plot(
        dff["date"],
        dff["protein"],
        color="tab:blue",
        linewidth=2,
        marker="o",
        markersize=5,
        label="Protein (g)"
    )

    # fiber: right axis (if available)
    if has_fiber:
        ax2.plot(
            dff["date"],
            dff["fiber"],
            color="tab:green",
            linewidth=2,
            linestyle="--",
            marker="s",
            markersize=4,
            label="Fiber (g)"
        )

    # Labels / styling
    ax1.set_xlabel("Date")
    ax1.set_ylabel("Energy (kcal)", color="tab:red")
    ax2.set_ylabel("Protein (g) / Fiber (g)", color="tab:blue")

    ax1.tick_params(axis="y", labelcolor="tab:red")
    ax2.tick_params(axis="y", labelcolor="tab:blue")

    # Keep identical x-range (full history)
    ax1.set_xlim(df["date"].min(), df["date"].max())
    ax1.grid(True, which="both", axis="both", alpha=0.4)

    plt.title("Food history: daily intake (zeros skipped)")
    plt.tight_layout()
    plt.savefig(out_path, dpi=150)
    plt.close()

    print(f"Wrote {out_path}")

if __name__ == "__main__":
    main()

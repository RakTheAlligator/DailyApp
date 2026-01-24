import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
import numpy as np

CSV = Path("data/history_food.csv")
OUT = Path("data/history_food.png")

def main():
    if not CSV.exists():
        raise SystemExit(f"Missing {CSV}. Run ./food_tracker history first.")

    df = pd.read_csv(CSV, parse_dates=["date"]).sort_values("date")

    # Mask des jours "vides"
    nonzero = ~((df["kcal"] == 0) & (df["protein"] == 0))

    # Données à tracer (uniquement non-zéro)
    dff = df.loc[nonzero].copy()

    fig, ax1 = plt.subplots(figsize=(12, 6))
    ax2 = ax1.twinx()

    # kcal: ligne continue, couleur chaude
    ax1.plot(
        dff["date"],
        dff["kcal"],
        color="tab:red",
        linewidth=2,
        label="Energy (kcal)"
    )

    # protein: ligne + points réguliers, couleur froide
    ax2.plot(
        dff["date"],
        dff["protein"],
        color="tab:blue",
        linewidth=2,
        marker="o",
        markersize=5,
        label="Protein (g)"
    )

    # Axes
    ax1.set_xlabel("Date")
    ax1.set_ylabel("Energy (kcal)", color="tab:red")
    ax2.set_ylabel("Protein (g)", color="tab:blue")

    ax1.tick_params(axis="y", labelcolor="tab:red")
    ax2.tick_params(axis="y", labelcolor="tab:blue")

    # Garder exactement la même abscisse
    ax1.set_xlim(df["date"].min(), df["date"].max())

    ax1.grid(True, which="both", axis="both", alpha=0.4)

    plt.title("Food history: daily intake (zeros skipped)")
    plt.tight_layout()
    plt.savefig(OUT, dpi=150)
    plt.close()


    print(f"Wrote {OUT}")

if __name__ == "__main__":
    main()

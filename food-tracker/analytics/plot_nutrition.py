import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

CSV = Path("data/history_food.csv")
OUT = Path("data/history_food.png")

def main():
    if not CSV.exists():
        raise SystemExit(f"Missing {CSV}. Run ./food_tracker history first.")

    df = pd.read_csv(CSV, parse_dates=["date"]).sort_values("date")

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
    plt.savefig(OUT, dpi=150)
    plt.close()

    print(f"Wrote {OUT}")

if __name__ == "__main__":
    main()

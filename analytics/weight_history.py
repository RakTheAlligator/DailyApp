#!/usr/bin/env python3
import argparse
from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--csv", required=True, help="Path to weight_history.csv")
    ap.add_argument("--out", required=True, help="Path to output PNG")
    args = ap.parse_args()

    csv_path = Path(args.csv)
    out_path = Path(args.out)

    if not csv_path.exists():
        raise SystemExit(f"CSV not found: {csv_path}")

    df = pd.read_csv(csv_path)
    if df.empty:
        raise SystemExit("CSV is empty, nothing to plot.")

    # expected columns: date,weight_kg
    df["date"] = pd.to_datetime(df["date"], format="%Y-%m-%d", errors="raise")
    df = df.sort_values("date")

    out_path.parent.mkdir(parents=True, exist_ok=True)

    plt.figure()
    plt.plot(df["date"], df["weight_kg"])
    plt.xlabel("Date")
    plt.ylabel("Weight (kg)")
    plt.title("Weight tracker history")
    plt.tight_layout()
    plt.savefig(out_path, dpi=160)
    print(f"Wrote: {out_path}")

if __name__ == "__main__":
    main()

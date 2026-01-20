from pathlib import Path
import argparse
import pandas as pd
import matplotlib.pyplot as plt


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--csv", required=True, help="Path to weights.csv")
    parser.add_argument("--out", required=True, help="Path to output PNG")
    args = parser.parse_args()

    csv_path = Path(args.csv)
    out_png = Path(args.out)

    if not csv_path.exists():
        print(f"CSV introuvable: {csv_path}")
        return 1

    df = pd.read_csv(csv_path)
    if df.empty:
        print("CSV vide.")
        return 0

    if "date" not in df.columns or "weight_kg" not in df.columns:
        print("CSV invalide: colonnes attendues: date, weight_kg")
        return 1

    df["date"] = pd.to_datetime(df["date"], format="%Y-%m-%d", errors="coerce")
    df = df.dropna(subset=["date"]).sort_values("date")
    if df.empty:
        print("Aucune date valide.")
        return 0

    # Axe temps constant (journalier) + trous les jours sans mesure
    full_index = pd.date_range(df["date"].min(), df["date"].max(), freq="D")
    measured = df.set_index("date")["weight_kg"].sort_index()

    # série journalière avec trous
    daily = measured.reindex(full_index)

    # interpolation temporelle pour combler les jours manquants (ligne continue)
    daily_interp = daily.interpolate(method="time")

    plt.figure(figsize=(12, 6))

    # ligne continue (interpolée)
    plt.plot(daily_interp.index, daily_interp.values)

    # points mesurés (vrais points)
    plt.scatter(measured.index, measured.values)

    plt.title("Historique du poids (kg)")
    plt.xlabel("Date")
    plt.ylabel("Poids (kg)")
    plt.grid(True)
    plt.xticks(rotation=45)
    plt.tight_layout()

    out_png.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(out_png, dpi=150)
    plt.close()

    print(f"Graph genere: {out_png.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

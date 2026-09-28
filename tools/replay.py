import argparse
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from bee_monitor import Monitor, Observation  # noqa: E402


def optional_float(row: dict[str, str], key: str) -> float | None:
    value = row.get(key, "").strip()
    return float(value) if value else None


def main() -> int:
    parser = argparse.ArgumentParser(description="Replay hive observations through the edge alarm logic")
    parser.add_argument("csv_file", type=Path)
    args = parser.parse_args()
    monitor = Monitor()
    previous = None

    with args.csv_file.open(encoding="utf-8-sig", newline="") as handle:
        for row in csv.DictReader(handle):
            state = monitor.update(Observation(
                ai_anomaly=float(row["ai_anomaly"]),
                temperature_c=optional_float(row, "temperature_c"),
                humidity_rh=optional_float(row, "humidity_rh"),
                weight_kg=optional_float(row, "weight_kg"),
            ))
            if state != previous:
                print(f'{row.get("timestamp", monitor.samples_seen):>8}  {state.value:<8} '
                      f'fused={monitor.fused_score:.3f} env={monitor.environmental_score:.3f}')
                previous = state
    return 0


if __name__ == "__main__":
    raise SystemExit(main())


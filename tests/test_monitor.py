import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from bee_monitor import Config, Monitor, Observation, State  # noqa: E402


class MonitorTests(unittest.TestCase):
    def setUp(self):
        self.config = Config(baseline_samples=3, warning_consecutive=2, alert_consecutive=2, clear_consecutive=3)
        self.monitor = Monitor(self.config)
        for _ in range(3):
            self.monitor.update(Observation(0.1, 33.0, 55.0, 20.0))

    def test_learns_normal_baseline(self):
        self.assertEqual(self.monitor.state, State.NORMAL)
        self.assertAlmostEqual(self.monitor.baseline_temperature_c, 33.0)

    def test_single_noise_spike_does_not_alarm(self):
        self.assertEqual(self.monitor.update(Observation(1.0, 33.0, 55.0, 20.0)), State.NORMAL)
        self.assertEqual(self.monitor.update(Observation(0.1, 33.0, 55.0, 20.0)), State.NORMAL)

    def test_sustained_audio_and_environment_reaches_alert(self):
        abnormal = Observation(0.9, 35.2, 72.0, 18.5)
        self.monitor.update(abnormal)
        self.assertEqual(self.monitor.update(abnormal), State.ALERT)

    def test_warning_clears_but_alert_requires_acknowledgement(self):
        warning = Observation(0.7, 33.0, 55.0, 20.0)
        self.monitor.update(warning)
        self.assertEqual(self.monitor.update(warning), State.WARNING)
        normal = Observation(0.1, 33.0, 55.0, 20.0)
        for _ in range(3):
            self.monitor.update(normal)
        self.assertEqual(self.monitor.state, State.NORMAL)


if __name__ == "__main__":
    unittest.main()


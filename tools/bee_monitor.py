from dataclasses import dataclass
from enum import Enum


class State(str, Enum):
    LEARNING = "LEARNING"
    NORMAL = "NORMAL"
    WARNING = "WARNING"
    ALERT = "ALERT"


@dataclass
class Config:
    audio_weight: float = 0.75
    temperature_weight: float = 0.45
    humidity_weight: float = 0.15
    weight_weight: float = 0.40
    temperature_delta_c: float = 2.0
    humidity_delta_rh: float = 15.0
    weight_drop_ratio: float = 0.05
    warning_threshold: float = 0.45
    alert_threshold: float = 0.70
    clear_threshold: float = 0.30
    baseline_alpha: float = 0.001
    baseline_samples: int = 60
    warning_consecutive: int = 5
    alert_consecutive: int = 3
    clear_consecutive: int = 30


@dataclass
class Observation:
    ai_anomaly: float
    temperature_c: float | None = None
    humidity_rh: float | None = None
    weight_kg: float | None = None


class Monitor:
    def __init__(self, config: Config | None = None):
        self.c = config or Config()
        self.state = State.LEARNING
        self.samples_seen = 0
        self.baseline_temperature_c = 0.0
        self.baseline_humidity_rh = 0.0
        self.baseline_weight_kg = 0.0
        self.fused_score = 0.0
        self.environmental_score = 0.0
        self.warning_count = self.alert_count = self.clear_count = 0

    @staticmethod
    def _clamp(value: float) -> float:
        return max(0.0, min(1.0, value))

    def _mean_step(self, current: float, value: float) -> float:
        return (current * self.samples_seen + value) / (self.samples_seen + 1)

    def _environment(self, o: Observation) -> float:
        weighted = 0.0
        used = 0.0
        if o.temperature_c is not None:
            delta = max(0.0, o.temperature_c - self.baseline_temperature_c)
            weighted += self.c.temperature_weight * self._clamp(delta / self.c.temperature_delta_c)
            used += self.c.temperature_weight
        if o.humidity_rh is not None:
            delta = abs(o.humidity_rh - self.baseline_humidity_rh)
            weighted += self.c.humidity_weight * self._clamp(delta / self.c.humidity_delta_rh)
            used += self.c.humidity_weight
        if o.weight_kg is not None and self.baseline_weight_kg > 0.01:
            drop = max(0.0, (self.baseline_weight_kg - o.weight_kg) / self.baseline_weight_kg)
            weighted += self.c.weight_weight * self._clamp(drop / self.c.weight_drop_ratio)
            used += self.c.weight_weight
        return self._clamp(weighted / used) if used else 0.0

    def update(self, o: Observation) -> State:
        if self.samples_seen < self.c.baseline_samples:
            if o.temperature_c is not None:
                self.baseline_temperature_c = self._mean_step(self.baseline_temperature_c, o.temperature_c)
            if o.humidity_rh is not None:
                self.baseline_humidity_rh = self._mean_step(self.baseline_humidity_rh, o.humidity_rh)
            if o.weight_kg is not None:
                self.baseline_weight_kg = self._mean_step(self.baseline_weight_kg, o.weight_kg)
            self.samples_seen += 1
            if self.samples_seen >= self.c.baseline_samples:
                self.state = State.NORMAL
            return self.state

        self.environmental_score = self._environment(o)
        self.fused_score = self._clamp(
            self.c.audio_weight * self._clamp(o.ai_anomaly)
            + (1.0 - self.c.audio_weight) * self.environmental_score
        )
        self.warning_count = self.warning_count + 1 if self.fused_score >= self.c.warning_threshold else 0
        self.alert_count = self.alert_count + 1 if self.fused_score >= self.c.alert_threshold else 0
        self.clear_count = self.clear_count + 1 if self.fused_score <= self.c.clear_threshold else 0

        if self.alert_count >= self.c.alert_consecutive:
            self.state = State.ALERT
        elif self.state != State.ALERT and self.warning_count >= self.c.warning_consecutive:
            self.state = State.WARNING
        elif self.state == State.WARNING and self.clear_count >= self.c.clear_consecutive:
            self.state = State.NORMAL

        if self.state == State.NORMAL and self.fused_score < self.c.warning_threshold:
            a = self.c.baseline_alpha
            if o.temperature_c is not None:
                self.baseline_temperature_c += a * (o.temperature_c - self.baseline_temperature_c)
            if o.humidity_rh is not None:
                self.baseline_humidity_rh += a * (o.humidity_rh - self.baseline_humidity_rh)
            if o.weight_kg is not None:
                self.baseline_weight_kg += a * (o.weight_kg - self.baseline_weight_kg)
        self.samples_seen += 1
        return self.state

    def acknowledge(self) -> None:
        if self.state == State.ALERT and self.clear_count > 0:
            self.state = State.WARNING
            self.alert_count = 0


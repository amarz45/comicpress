from dataclasses import dataclass
from enum import Enum
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .displays import Display

class CompressionType(Enum):
    LOSSLESS = 0
    LOSSY = 1

class QualityType(Enum):
    DISTANCE = 0
    QUALITY = 1

@dataclass(frozen = True)
class Config:
    dpi: int
    display: "Display | None"
    resample: str
    bit_depth: int | None
    dither: float | None
    stretch_contrast: bool
    img_format: str
    compression_or_speed_level: int
    compression_type: CompressionType
    quality_type: QualityType
    img_quality: int

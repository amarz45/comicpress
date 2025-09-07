from dataclasses import dataclass
from enum import Enum
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .displays import Display
    from pyvips.enums import Kernel

class CompressionType(Enum):
    LOSSLESS = "Lossless"
    LOSSY = "Lossy"

class QualityType(Enum):
    DISTANCE = "Distance"
    QUALITY = "Quality"

@dataclass(frozen = True)
class Config:
    dpi: int
    display: "Display | None"
    resample: "Kernel"
    bit_depth: int | None
    dither: float | None
    stretch_contrast: bool
    img_format: str
    compression_or_speed_level: int
    compression_type: CompressionType
    quality_type: QualityType
    img_quality: int

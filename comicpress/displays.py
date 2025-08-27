from dataclasses import dataclass

@dataclass(frozen = True)
class Display:
    width: int
    height: int

DISPLAYS = {
    "Custom": None,

    # Kindle
    "Kindle (11th Gen, 2022)": Display(1072, 1448),
    "Kindle Paperwhite (11th Gen)": Display(1236, 1648),
    "Kindle Paperwhite Signature Edition": Display(1236, 1648),
    "Kindle Oasis (3rd Gen)": Display(1264, 1680),
    "Kindle Scribe": Display(1860, 2480),

    # Kobo
    "Kobo Clara BW": Display(1072, 1448),
    "Kobo Clara Colour": Display(1072, 1448),
    "Kobo Libra Colour": Display(1264, 1680),
    "Kobo Sage": Display(1440, 1920),
    "Kobo Elipsa 2E": Display(1404, 1872),

    # PocketBook
    "PocketBook Verse": Display(758, 1024),
    "PocketBook Verse Pro": Display(1072, 1448),
    "PocketBook InkPad 4": Display(1404, 1872),
    "PocketBook InkPad Color 3": Display(1404, 1872),
    "PocketBook Era": Display(1264, 1680),

    # Boox
    "Boox Palma": Display(824, 1648),
    "Boox Page": Display(1264, 1680),
    "Boox Note Air3 C": Display(1404, 1872),
    "Boox Tab Ultra C Pro": Display(1860, 2480),
    "Boox Tab X": Display(1600, 2560),

    # reMarkable
    "reMarkable 2": Display(1872, 1404)
}

from dataclasses import dataclass

@dataclass(frozen = True)
class Display:
    width: int
    height: int

DISPLAYS = {
    "Custom": None,

    "Boox": {
        "Note Air3 C": Display(1404, 1872),
        "Page": Display(1264, 1680),
        "Palma": Display(824, 1648),
        "Tab Ultra C Pro": Display(1860, 2480),
        "Tab X": Display(1600, 2560),
    },

    "Kindle": {
        "(11th Gen, 2022)": Display(1072, 1448),
        "Oasis (3rd Gen)": Display(1264, 1680),
        "Paperwhite (11th Gen)": Display(1236, 1648),
        "Paperwhite Signature Edition": Display(1236, 1648),
        "Scribe": Display(1860, 2480)
    },

    "Kobo": {
        "Clara BW": Display(1072, 1448),
        "Clara Colour": Display(1072, 1448),
        "Elipsa 2E": Display(1404, 1872),
        "Libra Colour": Display(1264, 1680),
        "Sage": Display(1440, 1920),
    },

    "Pocketbook": {
        "Era": Display(1264, 1680),
        "InkPad 4": Display(1404, 1872),
        "InkPad Color 3": Display(1404, 1872),
        "Verse": Display(758, 1024),
        "Verse Pro": Display(1072, 1448),
    },

    "reMarkable": {
        "2": Display(1872, 1404)
    }
}

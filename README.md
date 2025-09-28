# bk56_SearchActorFuzzy

A native SKSE plugin that adds a Papyrus-accessible function for fuzzy name-based Actor lookup. Useful for locating NPCs whose display names may be altered or obscured by other modifications.

---

## Function Overview

papyrus
Int bk56_SearchActorFuzzy(String actorName, Bool nearbyOnly, Bool handledOnly, Float WeightMin)

Returns the FormID of an Actor whose name closely matches the input string. Supports fuzzy matching and optional filters for proximity and engine-handled status.

# Parameters

actorName: Name to search for (e.g., "Sven", "Ysolda", "Elisif The Fair").

nearbyOnly: If true, restricts search to high-process actors near the player.

handledOnly: If true, only considers actors currently handled by the engine.

WeightMin: Minimum similarity weight for a match (higher = stricter).

# Return Value

Returns FormID of matched Actor if found.

Returns 0 if no match meets the threshold.

# Behavior Notes

Uses fuzzy string matching to compare input against actor names.

Does not modify game state.

Tune WeightMin based on mod interference or name overrides.

Use Game.GetForm(FormID) and cast to Actor to retrieve the NPC.

# Example Usage

Int resultFormID = bk56_SearchActorFuzzy("Balgruuf", false, true, 70.0)
If resultFormID != 0
    Actor matched = Game.GetForm(resultFormID) as Actor
    Debug.Trace("Found: " + matched.GetDisplayName())
EndIf

# Tips and Considerations

Always check for nonzero return before casting.

Lower WeightMin if modifications heavily alter NPC names.

Use nearbyOnly for performance-sensitive scenarios.

Use handledOnly if you wish to actually get a formID. If the game finds a base record it will try and return that.

# Version and License

Plugin: bk56_SearchActorFuzzy

Version: 1.0.0

License: MIT

# Credits

RapidFuzz: Used for fuzzy string matching. Licensed under MIT.

SKSE (Skyrim Script Extender): Required for plugin functionality. Developed by the SKSE Team.
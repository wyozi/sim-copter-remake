# 2026-09-23 - riding medic never unloads at the hospital (diagnosis only)

Report: medic aboard + medevac patient, landed on a hospital; patient only came out after the
player put the medic down.

- BHAV 263 rec[29] is **op17** (TryAlightHere / FUN_004cb190 -> FUN_004c9bc0), not an in-flight
  unload. Rider path: rec[0] op84 -> rec[1] op59 (carrier is player) -> rec[29] op17 GET OFF
  (F -> retT, retry next 801 loop) -> rec[31] op84 -> rec[3] op47.
- FUN_004c9bc0's height is measured against FUN_004c82c0 = max(object tops, terrain), so a
  landed helipad counts as ground: the original's medic steps out onto the roof.
- Riders tick: FUN_004c5fb0 -> FUN_004c6450 copies the carrier pos (FUN_004c9470) then runs
  FUN_004ce7b0 (state 5 has +0x17e = 1, never distance-culled).
- Remake: CanAlightHere (Ground/SimCopterGroundAgent.cpp ~3617) calls
  IsPassengerDeliveryLocationAllowed(GetMissionPassengerKind()) - state 5 maps to Rescue, and
  Rescue is refused on a roof (terrain-relative 37.5 cm). op17 fails forever on the helipad.
- FUN_004c25b0 only spawns a D1 medic when FUN_004c1fb0 finds no live person spawned from a D1
  tile within 5 tiles; the rider (tile follows the heli) suppresses a second one. The remake's
  40 s re-post may or may not have a replacement there.
Fix: skip the delivery-surface gate for IsEmergencyCrewMember() riders in CanAlightHere.

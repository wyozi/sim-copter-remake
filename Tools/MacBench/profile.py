#!/usr/bin/env python3
"""Write a pinned GameUserSettings.ini: 'epic' = shipped defaults (Low Power off, sg 3), 'low' = Low Power on."""
import re, sys
src, dst, prof = sys.argv[1:4]
text = open(src).read()
def setkey(t, key, val):
    return re.sub(rf"(?m)^{re.escape(key)}=.*$", f"{key}={val}", t)
low = prof == "low"
level = {"epic": "3", "high": "2", "medium": "1", "lowpreset": "0"}.get(prof, "3")
text = setkey(text, "bLowPowerMode", "True" if low else "False")
text = setkey(text, "LowPowerRestoreScalabilityLevel", "3" if low else "-1")
text = setkey(text, "LowPowerRestoreResolutionScale", "0.000000" if low else "-1.000000")
for g in ["ViewDistance","AntiAliasing","Shadow","GlobalIllumination","Reflection","PostProcess","Texture","Effects","Foliage","Shading","Landscape"]:
    text = setkey(text, f"sg.{g}Quality", "0" if low else level)
text = setkey(text, "sg.ResolutionQuality", "75" if low else "100")
text = setkey(text, "FullscreenMode", "2"); text = setkey(text, "LastConfirmedFullscreenMode", "2")
open(dst, "w").write(text)

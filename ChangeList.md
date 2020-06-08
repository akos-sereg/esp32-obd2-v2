# Changelog

# next release
- display trip statistics: distance traveled (km) and duration
- fuel level correction added

# release/3.0.0
 - adding intake air temperature page
 - adding ambient air temperature page
 - adding altitude page (calculated based on air temperature and barometric pressure)
 - lcd write interference fix
 - main tick rate changed from 500 ms to 50 ms when connected to real OBD2 device (eg. not phone when testing)

# release/2.0.1
 - off state handling fix

# release/2.0.0
 - led strip poll frequency from 300 to 200 ms
 - fuel calculation always take the min value of fuel level
 - prevent LCD display text overrun
 - mode button: toggle lcd backlight

# release/1.0.0

 - Displaying Fuel (liter, remaining km), Engine Coolant (C) and Battery (V)
 - Known issues: 
   - blue/white should be replaced with green/yellow for better visibility
   - crap data screws up LCD display
   - led strip not showing realtime data, should be set back to 200 ms
   - fuel volume readings are different, eg. depending on car's uphill vs horizontal position
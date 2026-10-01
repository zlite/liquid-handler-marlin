There have been [many attempts](https://www.hardware-x.com/article/S2468-0672(26)00087-8/fulltext) to modify a cheap Ender 3 printer into a liquid handler using standard syringes, but most suffer from similar problems: too expensive (they often use parts purchased from approved academic vendors, which tend to be way more expensive than their Amazon equivalents), too big, too heavy, too complicated or too imprecise (for example, having no limit switches to accurately home the syringe). 

This is a modern version that improves on all of those dimensions along with adding a useful and easy to use web interface.

Advantages:
- Cheap: less than $250 INCLUDING the computer to control it
- Very precise: it should be good for +/- 10 microliters, repeatably
- Fully automatable via a Web API
- Easy to teach it new tasks, no coding required


BOM:
- Ender 3
- M5Stack Basic
- M5Stack USB module
- Pancake Stepper Motor
- 100m 5m lead screw
- 5mm to 5mm coupler
- 100m MG9 linear rail
- Limit switch
- 100 nf ceramic capaciter


Instructions. When assembling the Ender 3, skip the extruder and filament drive. Although you can re-purpose the stepper motor for our syringe drive, it's a hassle to remove the drive gear and it's bigger and heavier than we need, so I prefer to replace it with the smaller pancake stepper linked above.


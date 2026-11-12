# Hardware test checklist

Fill this in by hand on a real Game Boy Advance after copying a release
package (`./dev release copy`). `./dev release package` embeds this file in the
package `README.txt`. Nothing here is run or recorded by a tool; a package has
had no hardware testing until a person ticks these boxes and writes the
results down.

Each box should take under a minute. Tick it only for the ROM you tested, and
write down what you saw for any box you cannot tick.

## Common (every ROM)

- [ ] Revision from the package README: ____________  Device model: ____________
- [ ] ROM file name tested: ____________________________
- [ ] Boots to the title screen on the device.
- [ ] Start a campaign mission and a custom game against one opponent.
- [ ] Build a structure and train a unit.
- [ ] A fight plays with sound.
- [ ] Save from the pause menu, power off, power on, load: the state matches.
- [ ] An existing older save for this ROM still loads.

## Terran T10
<!-- products: terran -->

- [ ] Terran custom game: SCVs mine and the supply count updates.
- [ ] A Command Center or Barracks command card shows the right icons and prices.

## Zerg Z10 and legacy Z9
<!-- products: zerg-z9, zerg-z10 -->

- [ ] Zerg custom game: Drones mine and Larva hatch into units.
- [ ] Build menu highlights line up with the icon rows and prices.

## Protoss P10
<!-- products: protoss-p10 -->

- [ ] Pylon power field and placement: structures off the field are refused, and a Cannon near a Pylon works.
- [ ] Construction audio plays when a structure is placed; B-menu highlights and prices line up.
- [ ] Press L on a producing building: the job cancels and the cost is refunded; Probes keep mining afterwards.

## Showcase
<!-- products: showcase -->

- [ ] The Showcase menu opens and a battle starts and runs to the end.

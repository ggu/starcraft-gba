# Wasteland: a winning route

This route was verified through the ordinary controller inputs in the host simulation and replayed on the GBA build. It keeps the original 23 Zerglings and starting resources. Enemy movement can vary, so use the map and objective display to make decisions rather than following a fixed frame count.

## Opening and escort

Start with five Marines and five SCVs in the northwest. The Command Center, Supply Depot and mineral field lie farther south; Raynor waits near the route between them. SCVs begin mining automatically.

Tap **R** to select the combat group. To select only part of the force, hold **L** and drag a box with the D-pad, then release **L**; only living friendly units inside the box are selected. Press **B**, move down to **Attack Move**, and press **A**. Use the D-pad to place the destination just ahead of the first Zerglings on the route from your starting force toward the mineral field. Attack Move lets the Marines engage enemies they encounter while advancing. Keep Raynor near the friendly force: he is a required rescue and must survive.

Once the first group is cleared, issue another Attack Move toward the nearest hostile group on the workers' route back to the Command Center. The SCVs will continue mining and returning loads on their own. Watch the minimap and the worker count; if the enemy reaches the workers, move the combat group to intercept it. Let the workers make repeated safe trips until the mineral count reaches about **400**.

Hold **R** while moving the cursor to travel faster. The cursor moves over the world map, so use the minimap and landmarks to orient yourself. Raynor is near the middle of the map; move the friendly combat group close enough for him to join, then protect him as the force continues toward the southern outpost.

## Build and train

With at least **400 minerals**, select an SCV by moving the cursor over a worker and pressing **A**. Press **B** and choose **Barracks** from the construction menu. Place it on open ground near the Command Center and press **A**. Building it costs **150 minerals**; the selected SCV will construct it. The remaining resources continue accumulating while the structure finishes.

When the Barracks is complete and you have at least **250 minerals**, move the cursor over it and press **A** to select it. Press **A** five times to queue five Marines. They cost **50 minerals each**. Keep Raynor alive and defend the Barracks while training finishes. The mission completes when you have rescued Raynor, own a Barracks and have at least ten living Marines.

## Save and controls

Press **Start** or **Select** to pause. From the pause screen, **B** saves, **L** loads, **R** restarts, and **A** resumes. Saving before construction is a useful checkpoint once mining is safe. A restart resets the live mission, so use it only after you have saved if you intend to reload.

- **D-pad:** move the cursor; hold **R** for faster movement, including during a box drag.
- **A:** select a unit/building or confirm an order. A single tap acts immediately. Tap A twice quickly (within about 0.4 seconds) on the same friendly unit to select all living friendly units of the same type currently on screen. Offscreen units are not included. Building selection and contextual commands keep their existing behavior.
- **L:** tap to cycle friendly units; hold with the D-pad to drag a selection box, then release to select living friendly units inside. A drag works from ground or a selected building. **B** cancels the box and preserves the previous selection; **A** is ignored while dragging. A committed drag cancels a pending move/build mode and returns to normal command mode. On a selected Barracks, tap **L** to cancel the oldest queued unit.
- **R:** tap to select the combat group; on a selected Barracks it cycles the training type. Holding **R** while dragging only speeds cursor movement; release the box and tap **R** separately to select the combat group.
- **B:** open the command menu; select Attack Move for combat or Barracks for construction.

These box-selection controls require a ROM rebuilt from the updated source; an older copy does not include them.

The mission has simplified enemy behavior and combat. If a force or worker route stalls, select the combat group and issue a new Attack Move to a nearby clear area along the route, then check the objective and mineral count again.

#ifndef SC64_Z6_HALLUCINATION_H
#define SC64_Z6_HALLUCINATION_H

/* Bounded Z6 support for the one source-created hallucinated Tassadar.
 * The GBA simulation runs at 30 ticks/second while the source lifetime is
 * 1350 frames at 24 frames/second. Ceil conversion prevents early removal;
 * the resulting 1688-tick lifetime is at most half a source frame longer. */
#if defined(SC64_Z6_RUNTIME)

enum {
    SC64_Z6_HALLUCINATION_SOURCE_RAW = 79,
    SC64_Z6_HALLUCINATION_SOURCE_OWNER = 2,
    SC64_Z6_HALLUCINATION_SOURCE_FRAMES = 1350,
    SC64_Z6_HALLUCINATION_SOURCE_FPS = 24,
    SC64_Z6_HALLUCINATION_RUNTIME_FPS = 30,
    SC64_Z6_HALLUCINATION_LIFETIME_TICKS =
        (SC64_Z6_HALLUCINATION_SOURCE_FRAMES *
         SC64_Z6_HALLUCINATION_RUNTIME_FPS +
         SC64_Z6_HALLUCINATION_SOURCE_FPS - 1) /
        SC64_Z6_HALLUCINATION_SOURCE_FPS,
    SC64_Z6_HALLUCINATION_FLAG = 8
};

_Static_assert(SC64_Z6_HALLUCINATION_LIFETIME_TICKS == 1688,
               "Z6 hallucination lifetime conversion changed");

static int race_z6_hallucination_actor(const Unit *q)
{
    return q && q->alive && (q->flags & SC64_Z6_HALLUCINATION_FLAG) &&
           q->raw_type == SC64_Z6_HALLUCINATION_SOURCE_RAW &&
           q->raw_owner == SC64_Z6_HALLUCINATION_SOURCE_OWNER &&
           q->type == U_TASSADAR &&
           q->owner == race_z6_source_owner(SC64_Z6_HALLUCINATION_SOURCE_OWNER) &&
           q->work_timer > 0 &&
           q->work_timer <= SC64_Z6_HALLUCINATION_LIFETIME_TICKS;
}

/* Candidate validators use this pure row check: it depends only on the
 * decoded unit and the map's resolved live owner for source slot 2. */
static int race_z6_hallucination_unit_state_valid(const Unit *q,
                                                   int expected_live_owner)
{
    int marked;
    if (!q || expected_live_owner < SIDE || expected_live_owner > 4 ||
        q->alive < 0 || q->alive > 1) return 0;
    marked = (q->flags & SC64_Z6_HALLUCINATION_FLAG) != 0;
    if (q->raw_type != SC64_Z6_HALLUCINATION_SOURCE_RAW ||
        q->raw_owner != SC64_Z6_HALLUCINATION_SOURCE_OWNER)
        return !marked;
    if (q->type != U_TASSADAR || q->owner != expected_live_owner) return 0;
    if (q->alive)
        return marked && q->work_timer > 0 &&
               q->work_timer <= SC64_Z6_HALLUCINATION_LIFETIME_TICKS;
    return !marked && q->work_timer == 0;
}

/* Called by the source Create Unit property callback after the actor has
 * received its immutable raw identity and owner. */
static int race_z6_hallucination_mark_slot(unsigned slot)
{
    Unit *q;
    if (!race_z6_active() || slot >= MAX_UNITS) return 0;
    q = &u[slot];
    if (!q->alive || q->type != U_TASSADAR ||
        q->raw_type != SC64_Z6_HALLUCINATION_SOURCE_RAW ||
        q->raw_owner != SC64_Z6_HALLUCINATION_SOURCE_OWNER ||
        q->owner != race_z6_source_owner(SC64_Z6_HALLUCINATION_SOURCE_OWNER))
        return 0;
    q->flags |= SC64_Z6_HALLUCINATION_FLAG;
    q->work_timer = SC64_Z6_HALLUCINATION_LIFETIME_TICKS;
    return 1;
}

/* Call after normal Z6 source-death accounting for a combat-killed copy.
 * Death ownership/counting remains with race_z6_note_unit_death(). */
static __attribute__((noinline)) int race_z6_hallucination_cleanup_dead(Unit *q)
{
    if (!q) return 0;
    if (!(q->flags & SC64_Z6_HALLUCINATION_FLAG)) return 1;
    if (q->alive ||
        q->raw_type != SC64_Z6_HALLUCINATION_SOURCE_RAW ||
        q->raw_owner != SC64_Z6_HALLUCINATION_SOURCE_OWNER ||
        q->type != U_TASSADAR) return 0;
    q->flags &= ~SC64_Z6_HALLUCINATION_FLAG;
    q->work_timer = 0;
    return 1;
}

/* This transform belongs before shield absorption and armor. Source damage
 * doubles the full incoming weapon amount for hallucinations, then applies
 * the usual target shield and armor stages. */
static __attribute__((noinline)) int race_z6_hallucination_incoming_damage(int target, int damage)
{
    if (!race_z6_active() || target < 0 || target >= MAX_UNITS || damage <= 0 ||
        !race_z6_hallucination_actor(&u[target])) return damage;
    if (damage > 0x3fffffff) return 0x7fffffff;
    return damage * 2;
}

/* Hallucinations retain normal targeting/weapon animation but cannot inflict
 * HP or shield damage. The caller must skip hit_target for these attackers;
 * setting damage to zero alone is insufficient because hit_target applies a
 * minimum-one damage rule after armor. */
static __attribute__((noinline)) int race_z6_hallucination_can_deal_damage(int attacker)
{
    return !race_z6_active() || attacker < 0 || attacker >= MAX_UNITS ||
           !(u[attacker].flags & SC64_Z6_HALLUCINATION_FLAG);
}

static int race_z6_hallucination_world_state_valid(void)
{
    if (!race_z6_active()) return 1;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        const Unit *q = &u[i];
        if (q->raw_type != SC64_Z6_HALLUCINATION_SOURCE_RAW ||
            q->raw_owner != SC64_Z6_HALLUCINATION_SOURCE_OWNER) {
            if (q->flags & SC64_Z6_HALLUCINATION_FLAG) return 0;
            continue;
        }
        if (!race_z6_hallucination_unit_state_valid(
                q, race_z6_source_owner(SC64_Z6_HALLUCINATION_SOURCE_OWNER)))
            return 0;
    }
    return 1;
}

/* Advance once per active gameplay tick. Expiration uses the Z6 source-death
 * accounting path, releases SIDE supply if the source owner policy ever maps
 * this actor to SIDE, and clears the marker so the slot can be reused safely. */
static int race_z6_hallucination_tick(int user_paused)
{
    if (!race_z6_active() || user_paused ||
        race_z6.runtime.model.outcome != SC64_Z6_RUNNING) return 1;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        if (!(q->flags & SC64_Z6_HALLUCINATION_FLAG)) continue;
        if (!q->alive) {
            if (!race_z6_hallucination_cleanup_dead(q)) return 0;
            continue;
        }
        if (!race_z6_hallucination_actor(q)) return 0;
        if (--q->work_timer > 0) continue;
        race_z6_note_unit_death(q);
        if (q->owner == SIDE) supply_used -= unit_supply(q->type);
        q->alive = 0;
        if (!race_z6_hallucination_cleanup_dead(q)) return 0;
    }
    return 1;
}

#endif /* SC64_Z6_RUNTIME */
#endif /* SC64_Z6_HALLUCINATION_H */

/* Default-off direct AttackUnit policy helpers.  The engine keeps using its
 * existing packed order and target fields; this header adds no state. */
#ifndef SC64_DIRECT_ATTACK_H
#define SC64_DIRECT_ATTACK_H

typedef struct {
    int alive;
    int hostile;
    int visible;
    int loaded;
    int invincible;
    int detectable;
} Sc64DirectTargetState;

static int __attribute__((noinline))
sc64_direct_target_valid(Sc64DirectTargetState s)
{
    return s.alive && s.hostile && !s.loaded && !s.invincible && s.detectable;
}

static int __attribute__((noinline))
sc64_direct_target_can_damage(Sc64DirectTargetState s)
{
    return sc64_direct_target_valid(s) && s.visible;
}

static int __attribute__((noinline))
sc64_direct_order_keep_last_seen(int visible, int distance2_to_last_seen)
{
    return !visible && distance2_to_last_seen > 36;
}

#endif

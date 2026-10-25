#ifndef SC64_DERIVED_SAVE_H
#define SC64_DERIVED_SAVE_H
/* Opt-in generation-38 rows. maxhp/armor are reconstructed
 * from the same profile facts used by constructors. Unit.weapon is omitted
 * only because no gameplay path reads it; this is semantic, not bitwise,
 * round-trip preservation. Decode sets it to canonical zero. Changing these
 * constructor facts or giving Unit.weapon a gameplay meaning requires a new
 * save generation or a compatible explicit migration. */
#define SC64_DERIVED_UNIT_BYTES 29
#define SC64_DERIVED_BUILDING_BYTES 25
_Static_assert(SC_RU_COUNT <= 64 && SC_RB_COUNT <= 64,
               "derived save enums must fit their six-bit fields");
typedef struct {
  u8 bytes[SC64_DERIVED_UNIT_BYTES];
} Sc64DerivedUnitSave;
typedef struct {
  u8 bytes[SC64_DERIVED_BUILDING_BYTES];
} Sc64DerivedBuildingSave;
_Static_assert(sizeof(Sc64DerivedUnitSave) == 29, "derived unit row");
_Static_assert(sizeof(Sc64DerivedBuildingSave) == 25, "derived building row");

static int sc64_derived_put(u8 *p, unsigned n, unsigned *bit, unsigned w,
                            u32 v) {
  if (!p || !bit || !w || w > 32 || *bit > n * 8u || w > n * 8u - *bit ||
      (w < 32 && v >= (1u << w)))
    return 0;
  for (unsigned i = 0; i < w; i++, (*bit)++)
    if ((v >> i) & 1u)
      p[*bit >> 3] |= (u8)(1u << (*bit & 7));
  return 1;
}
static int sc64_derived_get(const u8 *p, unsigned n, unsigned *bit, unsigned w,
                            u32 *v) {
  u32 x = 0;
  if (!p || !bit || !v || !w || w > 32 || *bit > n * 8u || w > n * 8u - *bit)
    return 0;
  for (unsigned i = 0; i < w; i++, (*bit)++)
    x |= (u32)((p[*bit >> 3] >> (*bit & 7)) & 1u) << i;
  *v = x;
  return 1;
}
static int sc64_derived_zero(const u8 *p, unsigned n) {
  for (unsigned i = 0; i < n; i++)
    if (p[i])
      return 0;
  return 1;
}
static int sc64_derived_unit_hp(int type) {
  int hp = type >= 0 && type < SC_RU_COUNT ? sc64_race_unit_rules[type].hp : 0;
  int z = sc64_zp_actor_default_hp(type);
  if (z)
    hp = z;
#ifdef SC64_Z7_EGG_PRODUCTION
  if (type == U_Z7_EGG)
    hp = 200;
#endif
  return hp;
}
static int sc64_derived_unit_hp_for_raw(int type, int raw_type) {
  int hp = sc64_derived_unit_hp(type);
#ifdef SC64_RACES
  /* Source identities may alias one gameplay enum (early Raynor is raw 19
   * but keeps U_RAYNOR). The compact row already stores raw_type, so rebuild
   * imported actors from that canonical source record when it has HP facts. */
  const Sc64StockUnitProfile *stock = sc64_stock_unit_profile(raw_type);
  if (stock && stock->hp) hp = (int)stock->hp;
#else
  (void)raw_type;
#endif
  return hp;
}
static int sc64_derived_building_hp(int type) {
  if (type < 0 || type >= SC_RB_COUNT)
    return 0;
  return sc64_race_building_max_hp(type);
}
static int sc64_derived_unit_encode(Sc64DerivedUnitSave *out, const Unit *q,
                                    unsigned ww, unsigned wh) {
  Sc64DerivedUnitSave z = {{0}};
  unsigned k = 0;
  static const Unit zero = {0};
  if (!out || !q || !ww || !wh || ww > 2048 || wh > 2048 || q->type < 0 ||
      q->type >= SC_RU_COUNT)
    return 0;
  if (!memcmp(q, &zero, sizeof(*q))) {
    *out = z;
    return 1;
  }
  if (q->x < 0 || (unsigned)q->x >= ww || q->y < 0 || (unsigned)q->y >= wh ||
      q->hp < -131072 || q->hp >= 131072 || (q->alive && q->hp < 0) ||
      q->maxhp != sc64_derived_unit_hp_for_raw(q->type,q->raw_type) || q->owner < 0 ||
      q->owner > 4 || q->type < 0 || q->type >= SC_RU_COUNT || q->alive > 1 ||
      q->selected > 1 || q->cd < 0 || q->cd > 65535 || q->order < 0 ||
      q->order > 15 || q->tx < 0 || q->tx > 2047 || q->ty < 0 || q->ty > 2047 ||
      q->carry < -65536 || q->carry >= 65536 || q->build < -512 ||
      q->build >= 512 || q->work_timer < 0 || q->work_timer > 65535 ||
      q->armor != sc64_race_unit_armor(q->type) || q->weapon < 0 ||
      q->weapon > 65535 || q->target < -1 || q->target > 511 ||
      q->path_timer < 0 || q->path_timer > 24 || q->path_retry < 0 ||
      q->path_retry > 12 || q->path_x < 0 || q->path_x > 2047 ||
      q->path_y < 0 || q->path_y > 2047 || q->shield < 0 ||
      q->shield > 131071 || q->raw_type < -1 || q->raw_type > 254 ||
      q->raw_owner < -1 || q->raw_owner > 14 || q->flags < 0 ||
      q->flags > 255 || q->shield_remainder < 0 || q->shield_remainder > 1279)
    return 0;
#define P(w, v)                                                                \
  do {                                                                         \
    if (!sc64_derived_put(z.bytes, sizeof(z.bytes), &k, w, (u32)(v)))          \
      return 0;                                                                \
  } while (0)
  P(11, q->x);
  P(11, q->y);
  P(18, q->hp + 131072);
  P(3, q->owner);
  P(6, q->type);
  P(1, q->alive);
  P(1, q->selected);
  P(16, q->cd);
  P(4, q->order);
  P(11, q->tx);
  P(11, q->ty);
  P(17, q->carry + 65536);
  P(10, q->build + 512);
  P(16, q->work_timer);
  P(10, q->target + 512);
  P(5, q->path_timer);
  P(4, q->path_retry);
  P(11, q->path_x);
  P(11, q->path_y);
  P(17, q->shield);
  P(8, q->raw_type + 1);
  P(4, q->raw_owner + 1);
  P(8, q->flags);
  P(11, q->shield_remainder);
  P(1, 1u); /* Nonzero row marker; all-zero slots have their own encoding. */
#undef P
  if (k != 226)
    return 0;
  *out = z;
  return 1;
}
static int sc64_derived_unit_decode(Unit *out, const Sc64DerivedUnitSave *row,
                                    unsigned ww, unsigned wh) {
  Unit q;
  u32 v;
  unsigned k = 0;
  if (!out || !row || !ww || !wh || ww > 2048 || wh > 2048)
    return 0;
  memset(&q, 0, sizeof(q));
  if (sc64_derived_zero(row->bytes, sizeof(row->bytes))) {
    *out = q;
    return 1;
  }
#define G(w, d)                                                                \
  do {                                                                         \
    if (!sc64_derived_get(row->bytes, sizeof(row->bytes), &k, w, &v))          \
      return 0;                                                                \
    (d) = (int)v;                                                              \
  } while (0)
  G(11, q.x);
  G(11, q.y);
  G(18, v);
  q.hp = (int)v - 131072;
  G(3, q.owner);
  G(6, q.type);
  G(1, q.alive);
  G(1, q.selected);
  G(16, q.cd);
  G(4, q.order);
  G(11, q.tx);
  G(11, q.ty);
  G(17, v);
  q.carry = (int)v - 65536;
  G(10, v);
  q.build = (int)v - 512;
  G(16, q.work_timer);
  G(10, v);
  q.target = (int)v - 512;
  G(5, q.path_timer);
  G(4, q.path_retry);
  G(11, q.path_x);
  G(11, q.path_y);
  G(17, q.shield);
  G(8, v);
  q.raw_type = (int)v - 1;
  G(4, v);
  q.raw_owner = (int)v - 1;
  G(8, q.flags);
  G(11, q.shield_remainder);
  G(1, v);
  if (v != 1u)
    return 0;
#undef G
  if (q.type < 0 || q.type >= SC_RU_COUNT)
    return 0;
  q.maxhp = sc64_derived_unit_hp_for_raw(q.type,q.raw_type);
  q.armor = sc64_race_unit_armor(q.type);
  q.weapon = 0;
  if (k != 226 || q.x < 0 ||
      (unsigned)q.x >= ww || q.y < 0 || (unsigned)q.y >= wh || q.hp < -131072 ||
      q.hp >= 131072 || (q.alive && q.hp < 0) || q.maxhp <= 0 || q.owner > 4 ||
      q.type >= SC_RU_COUNT || q.alive > 1 || q.selected > 1 || q.order > 15 ||
      q.tx > 2047 || q.ty > 2047 || q.target < -1 || q.target > 511 ||
      q.path_timer > 24 || q.path_retry > 12 || q.path_x > 2047 ||
      q.path_y > 2047 || q.shield > 131071 || q.raw_type > 254 ||
      q.raw_owner > 14 || q.shield_remainder > 1279)
    return 0;
  for (; k < sizeof(row->bytes) * 8u; k++)
    if ((row->bytes[k >> 3] >> (k & 7)) & 1u)
      return 0;
  *out = q;
  return 1;
}

static int sc64_derived_building_encode(Sc64DerivedBuildingSave *out,
                                        const Building *q, unsigned ww,
                                        unsigned wh) {
  Sc64DerivedBuildingSave z = {{0}};
  unsigned k = 0;
  static const Building zero = {0};
  if (!out || !q || !ww || !wh || ww > 2048 || wh > 2048 || q->type < 0 ||
      q->type >= SC_RB_COUNT)
    return 0;
  if (!memcmp(q, &zero, sizeof(*q))) {
    *out = z;
    return 1;
  }
  if (q->x < 0 || (unsigned)q->x >= ww || q->y < 0 || (unsigned)q->y >= wh ||
      q->hp < -131072 || q->hp >= 131072 || (q->alive && q->hp < 0) ||
      q->maxhp != sc64_derived_building_hp(q->type) || q->owner < 0 ||
      q->owner > 4 || q->type < 0 || q->type >= SC_RB_COUNT || q->alive > 1 ||
      q->queue_head < 0 || q->queue_head > 4 || q->queue_count < 0 ||
      q->queue_count > 5 || q->queue_progress < 0 ||
      q->queue_progress > 65535 || q->build_progress < -1 ||
      q->build_progress > 65534 || q->under_construction > 1 || q->level < 0 ||
      q->level > 127 || q->shield < 0 || q->shield > 131071 ||
      q->raw_type < -1 || q->raw_type > 254 || q->raw_owner < -1 ||
      q->raw_owner > 14 || q->flags < 0 || q->flags > 65535 || q->larva < 0 ||
      q->larva > 3 || q->larva_timer < 0 || q->larva_timer > 511 ||
      q->shield_remainder < 0 || q->shield_remainder > 1279)
    return 0;
  for (int i = 0; i < 5; i++)
    if (q->queue[i] < 0 || q->queue[i] >= SC_RU_COUNT)
      return 0;
#define P(w, v)                                                                \
  do {                                                                         \
    if (!sc64_derived_put(z.bytes, sizeof(z.bytes), &k, w, (u32)(v)))          \
      return 0;                                                                \
  } while (0)
  P(11, q->x);
  P(11, q->y);
  P(18, q->hp + 131072);
  P(3, q->owner);
  P(6, q->type);
  P(1, q->alive);
  for (int i = 0; i < 5; i++)
    P(6, q->queue[i]);
  P(3, q->queue_head);
  P(3, q->queue_count);
  P(16, q->queue_progress);
  P(16, q->build_progress + 1);
  P(1, q->under_construction);
  P(7, q->level);
  P(17, q->shield);
  P(8, q->raw_type + 1);
  P(4, q->raw_owner + 1);
  P(16, q->flags);
  P(2, q->larva);
  P(9, q->larva_timer);
  P(11, q->shield_remainder);
  P(1, 1u); /* Prevent a dead occupied row colliding with an unused slot. */
#undef P
  if (k != 194)
    return 0;
  *out = z;
  return 1;
}
static int sc64_derived_building_decode(Building *out,
                                        const Sc64DerivedBuildingSave *row) {
  Building q;
  u32 v;
  unsigned k = 0;
  if (!out || !row)
    return 0;
  memset(&q, 0, sizeof(q));
  if (sc64_derived_zero(row->bytes, sizeof(row->bytes))) {
    *out = q;
    return 1;
  }
#define G(w, d)                                                                \
  do {                                                                         \
    if (!sc64_derived_get(row->bytes, sizeof(row->bytes), &k, w, &v))          \
      return 0;                                                                \
    (d) = (int)v;                                                              \
  } while (0)
  G(11, q.x);
  G(11, q.y);
  G(18, v);
  q.hp = (int)v - 131072;
  G(3, q.owner);
  G(6, q.type);
  G(1, q.alive);
  for (int i = 0; i < 5; i++)
    G(6, q.queue[i]);
  G(3, q.queue_head);
  G(3, q.queue_count);
  G(16, q.queue_progress);
  G(16, v);
  q.build_progress = (int)v - 1;
  G(1, q.under_construction);
  G(7, q.level);
  G(17, q.shield);
  G(8, v);
  q.raw_type = (int)v - 1;
  G(4, v);
  q.raw_owner = (int)v - 1;
  G(16, q.flags);
  G(2, q.larva);
  G(9, q.larva_timer);
  G(11, q.shield_remainder);
  G(1, v);
  if (v != 1u)
    return 0;
#undef G
  if (q.type < 0 || q.type >= SC_RB_COUNT)
    return 0;
  q.maxhp = sc64_derived_building_hp(q.type);
  if (k != 194 || q.x > 2047 || q.y > 2047 || q.hp < -131072 ||
      q.hp >= 131072 || (q.alive && q.hp < 0) || q.maxhp <= 0 || q.owner > 4 ||
      q.type >= SC_RB_COUNT || q.alive > 1 || q.queue_head > 4 ||
      q.queue_count > 5 || q.under_construction > 1 || q.level > 127 ||
      q.shield > 131071 || q.raw_type > 254 || q.raw_owner > 14 ||
      q.larva > 3 || q.shield_remainder > 1279 || q.build_progress < -1)
    return 0;
  for (; k < sizeof(row->bytes) * 8u; k++)
    if ((row->bytes[k >> 3] >> (k & 7)) & 1u)
      return 0;
  for (int i = 0; i < 5; i++)
    if (q.queue[i] >= SC_RU_COUNT)
      return 0;
  *out = q;
  return 1;
}
#endif

#ifndef FIELD_MENU_REPEAT_H
#define FIELD_MENU_REPEAT_H

/* Transient input state: a fresh tap moves immediately, then a held direction
 * repeats after 18 updates and every 5 updates thereafter. Never serialized. */
static u16 field_menu_repeat_direction;
static u8 field_menu_repeat_age;

static void field_menu_repeat_reset(void)
{
    field_menu_repeat_direction = 0;
    field_menu_repeat_age = 0;
}

static u16 field_menu_repeat_keys(u16 held, u16 pressed)
{
    u16 direction = held & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT);
    if (!direction || (direction & (direction - 1))) {
        field_menu_repeat_reset();
        return pressed;
    }
    if (direction != field_menu_repeat_direction || (pressed & direction)) {
        field_menu_repeat_direction = direction;
        field_menu_repeat_age = 0;
    } else if (++field_menu_repeat_age >= 18) {
        field_menu_repeat_age = 13;
        pressed |= direction;
    }
    return pressed;
}
#endif

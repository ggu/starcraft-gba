#ifndef SC64_MAP_MATH_H
#define SC64_MAP_MATH_H

/* Fast coordinate conversion for the supported 64/96/128 tile map widths.
 * These helpers return SC64_MAP_MATH_INVALID for an unsupported width or an
 * out-of-bounds coordinate. No variable integer division is emitted. */
#define SC64_MAP_MATH_INVALID 0xffffffffu

static inline unsigned sc64_map_math_width_supported(unsigned width)
{
    return width == 64u || width == 96u || width == 128u;
}

static inline unsigned sc64_map_cell_row(unsigned cell, unsigned width)
{
    if (!sc64_map_math_width_supported(width) || cell >= width * 128u)
        return SC64_MAP_MATH_INVALID;
    if (width == 64u) return cell >> 6;
    if (width == 128u) return cell >> 7;
    /* ceil(2^22 / 96); exact for every cell in the supported 96-wide maps. */
    return (cell * 43691u) >> 22;
}

static inline unsigned sc64_map_cell_col(unsigned cell, unsigned width)
{
    unsigned row = sc64_map_cell_row(cell, width);
    if (row == SC64_MAP_MATH_INVALID) return SC64_MAP_MATH_INVALID;
    return cell - row * width;
}

/* Return the linear index into the 24x24 minimap for a tile coordinate. */
static inline unsigned sc64_map_tile_minimap_index(unsigned col, unsigned row,
                                                    unsigned width, unsigned height)
{
    unsigned mx, my;
    if (!sc64_map_math_width_supported(width) ||
        !sc64_map_math_width_supported(height) || col >= width || row >= height)
        return SC64_MAP_MATH_INVALID;
    if (width == 64u) mx = (col * 3u) >> 3;
    else if (width == 96u) mx = col >> 2;
    else mx = (col * 3u) >> 4;
    if (height == 64u) my = (row * 3u) >> 3;
    else if (height == 96u) my = row >> 2;
    else my = (row * 3u) >> 4;
    return mx + my * 24u;
}

/* Return the linear index for a world-pixel coordinate. Each tile is 16px. */
static inline unsigned sc64_map_pixel_minimap_index(unsigned x, unsigned y,
                                                     unsigned width, unsigned height)
{
    unsigned mx, my;
    if (!sc64_map_math_width_supported(width) ||
        !sc64_map_math_width_supported(height) ||
        x >= width * 16u || y >= height * 16u)
        return SC64_MAP_MATH_INVALID;
    if (width == 64u) mx = (x * 3u) >> 7;
    else if (width == 96u) mx = x >> 6;
    else mx = (x * 3u) >> 8;
    if (height == 64u) my = (y * 3u) >> 7;
    else if (height == 96u) my = y >> 6;
    else my = (y * 3u) >> 8;
    return mx + my * 24u;
}

#endif

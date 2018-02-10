#pragma once

struct Grid {
    Grid();

    void clear();
    void color_movement_range(void);
    // mark grid for all current player selectable units
    void color_units(void);
    void color_targets(void);
    // ---
    object o; // graphical object in blitter -> fixme inherit ?
    uint32_t data[32]; // 32 bits per line - use a bit array ?
    // ---
    static void grid_line (struct object *o);
};
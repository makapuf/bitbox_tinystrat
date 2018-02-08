#pragma once

struct Grid {
    Grid();

    void clear();
    void color_movement_range(void);
    // mark grid for all current player selectable units
    void color_units(void);
    void color_targets(void);
    // ---
    uint32_t data[32]; // 32 bits per line - use a bit array ?
    object *o; // graphical object in blitter
    // ---
    static void grid_line (struct object *o);
};
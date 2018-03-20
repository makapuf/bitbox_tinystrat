#pragma once

struct Grid : public object
{
    Grid();

    void clear();
    void hide();
    void show();

    void color_movement_range(void);
    // mark grid for all current player selectable units
    void color_units(void);
    void color_targets(void);
    // ---
    uint32_t m_data[32]; // 32 bits per line - use a bit array ?
    // ---
    static void grid_line (struct object *o);
};
npc_types = {
    {
        type        = "BLUESLIME",
        move_type   = "ROAMING",
        battle_type = "AGRO",
        level       = 3,
        count       = 50000,
        spawn_area  = { x1=0, y1=0, x2=1000, y2=1000 }  -- 스폰 구역
    },
    {
        type        = "CHICKEN",
        move_type   = "ROAMING",
        battle_type = "PEACE",
        level       = 1,
        count       = 50000,
        spawn_area  = { x1=1000, y1=0, x2=2000, y2=1000 }
    },
    {
        type        = "COW",
        move_type   = "FIXED",
        battle_type = "PEACE",
        level       = 2,
        count       = 50000,
        spawn_area  = { x1=0, y1=1000, x2=1000, y2=2000 }
    },
    {
        type        = "REDSLIME",
        move_type   = "FIXED",
        battle_type = "AGRO",
        level       = 5,
        count       = 50000,
        spawn_area  = { x1=1000, y1=1000, x2=2000, y2=2000 }
    },
}
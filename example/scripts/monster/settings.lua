return {
    ["Maggot"] = {
        strollrange = TILESIZE,
        aggressive = false,
        trackrange = 5 * TILESIZE,
        attack_distance = TILESIZE,
    },
    ["Scorpion"] = {
        strollrange = 2 * TILESIZE,
        aggressive = false,
        trackrange = 5 * TILESIZE,
        attack_distance = TILESIZE,
    },
    ["Red Scorpion"] = {
        strollrange = TILESIZE,
        aggressive = true,
        trackrange = 5 * TILESIZE,
        attack_distance = TILESIZE,
        ability_id = 2,
        damage = {
            base = 2,
            delta = 5,
            chance_to_hit = 20,
        },
    },
    ["Green Slime"] = {
        strollrange = TILESIZE,
        aggressive = true,
        trackrange = 5 * TILESIZE,
        attack_distance = TILESIZE,
    },
}

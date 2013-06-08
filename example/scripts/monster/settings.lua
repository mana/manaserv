return {
    ["Maggot"] = {
        strollrange = TILESIZE,
        aggressive = false,
        trackrange = 5 * TILESIZE,
        attack_distance = TILESIZE,
        experience = 10,
        ability_id = 2,
        damage = {
            base = 0,
            delta = 1,
            chance_to_hit = 2,
        },
    },
    ["Scorpion"] = {
        strollrange = 2 * TILESIZE,
        aggressive = false,
        trackrange = 5 * TILESIZE,
        attack_distance = TILESIZE,
        experience = 10,
    },
    ["Red Scorpion"] = {
        strollrange = TILESIZE,
        aggressive = true,
        trackrange = 5 * TILESIZE,
        attack_distance = TILESIZE,
        ability_id = 2,
        experience = 10,
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
        experience = 10,
    },
}

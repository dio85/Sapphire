-- LOOT TABLE
INSERT INTO loot_tables (id, name, type)
VALUES
(
    1,
    'bnpc_forestFunguar',
    'bNPC'
);

-- LOOT POOL
INSERT INTO loot_pools
(
    id,
    loot_table_id,
    name,
    enabled,
    duplicates,
    pick_min,
    pick_max
)
VALUES
(
    1,
    1,
    'Pool #1 (Shriekshroom)',
    TRUE,
    FALSE,
    1,
    1
);

-- LOOT ITEMS
INSERT INTO loot_items
(
    pool_id,
    item_id,
    weight,
    is_hq,
    quantity_min,
    quantity_max
)
VALUES
    (1, 4794, 296, FALSE, 1, 1),
    (1, 0,    704, FALSE, 0, 0);
-- LOOT ITEMS
INSERT INTO loot_pool_items
(
    pool_id,
    item_id,
    weight,
    is_hq,
    qty_min,
    qty_max
)
VALUES
    (1, 4794, 296, FALSE, 1, 1),
    (1, 0,    704, FALSE, 0, 0);
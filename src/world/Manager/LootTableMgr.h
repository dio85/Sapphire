#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <deque>
#include <string>

namespace Sapphire::World::Loot
{
  struct LootTableRange {
    uint32_t min;
    uint32_t max;
  };

  struct LootTableItem {
    uint32_t id{};
    uint32_t weight{};
    bool isHq{};

    LootTableRange quantity;
  };

  struct LootTablePool {
    std::string name;
    bool enabled{};
    bool duplicates{};
    LootTableRange pick;
    std::vector< LootTableItem > items;
  };

  struct LootTable {
    std::string lootTable;
    std::deque< LootTablePool > pools;
  };

  using LootTablePtr = std::shared_ptr< LootTable >;

  struct LootTableResultItem {
    uint32_t id{};
    uint32_t quantity{};
    bool isHq{};
  };

  struct LootTableResult {
    std::string name;
    std::vector< LootTableResultItem > items;

    bool isEmpty() const { return items.empty(); }
    size_t count() const { return items.size(); }
  };

  struct TempPoolRef {
    LootTablePool* pool = nullptr;
  };
}// namespace Sapphire::World::Loot

namespace Sapphire::World::Manager
{
  class LootTableMgr
  {
  public:
    bool cacheLootTables();
    bool loadLootTable();
    bool LoadLootPools();
    bool LoadLootPoolItems();

    Loot::LootTablePtr getLootTableByName( const std::string& name );
    Loot::LootTableResult rollLootForBNpc( uint32_t bnpcNameId );
    Loot::LootTableResult rollLoot( const std::string& name );

  private:
    const Loot::LootTableItem& pickWeightedItem(
            const std::vector< Loot::LootTableItem >& items );

  private:
    std::unordered_map< uint32_t, Loot::LootTablePtr > m_tempTableIds;
    std::unordered_map< uint32_t, Loot::TempPoolRef > m_tempPools;
    std::unordered_map< std::string, Loot::LootTablePtr > m_lootTableMap;
  };
}// namespace Sapphire::World::Manager
#include <iterator>
#include <fstream>
#include <filesystem>
#include <iostream>

#include <Common.h>
#include <Service.h>
#include <Logging/Logger.h>

#include <Random/RNGMgr.h>
#include "LootTableMgr.h"
#include <Database/DatabaseDef.h>
#include <unordered_set>

using namespace Sapphire;
using namespace Sapphire::World::Loot;
using namespace Sapphire::World::Manager;
namespace fs = std::filesystem;

Sapphire::Db::DbWorkerPool< Sapphire::Db::ZoneDbConnection > g_charaDb;

bool LootTableMgr::cacheLootTables()
{
  m_tempTableIds.clear();
  m_tempPools.clear();

  Logger::info( "LootTableMgr: cacheLootTables start" );

  if( !loadLootTable() )
    return false;

  if( !LoadLootPools() )
    return false;

  if( !LoadLootPoolItems() )
    return false;

  return true;
}

bool LootTableMgr::loadLootTable()
{
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();

  auto stmt = db.getPreparedStatement(
          Db::ZoneDbStatements::LOOT_SEL_TABLES );

   auto res = db.query( stmt );

  while( res->next() )
  {
    uint32_t id = res->getUInt( "id" );

    auto lootTable = std::make_shared< LootTable >();
    lootTable->lootTable = res->getString( "name" );

    m_lootTableMap[ lootTable->lootTable ] = lootTable;
    m_tempTableIds[ id ] = lootTable;

    Logger::info(
            "Loaded loot table {} with id {}",
            lootTable->lootTable,
            id );
  }

  return true;
}

bool LootTableMgr::LoadLootPools()
{
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();

  auto stmt = db.getPreparedStatement(
          Db::ZoneDbStatements::LOOT_SEL_POOLS );

  auto res = db.query( stmt );

  while( res->next() )
  {
    uint32_t id = res->getUInt( "id" );
    uint32_t lootTableId = res->getUInt( "loot_table_id" );

    auto tableIt = m_tempTableIds.find( lootTableId );
    if( tableIt == m_tempTableIds.end() )
      continue;

    LootTablePool pool;
    pool.name = res->getString( "name" );
    pool.enabled = res->getBoolean( "enabled" );
    pool.duplicates = res->getBoolean( "duplicates" );
    pool.pick.min = res->getUInt( "pick_min" );
    pool.pick.max = res->getUInt( "pick_max" );

    auto& tablePools = tableIt->second->pools;

    tablePools.push_back( pool );

    // pointer valid (deque miatt stabil)
    LootTablePool* poolPtr = &tablePools.back();

    TempPoolRef ref;
    ref.pool = poolPtr;

    m_tempPools[ id ] = ref;

    Logger::info(
            "Loaded loot pool {} for table {}",
            pool.name,
            lootTableId );
  }

  return true;
}

bool LootTableMgr::LoadLootPoolItems()
{
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();

  auto stmt = db.getPreparedStatement(
          Db::ZoneDbStatements::LOOT_SEL_ITEMS );

  auto res = db.query( stmt );

  while( res->next() )
  {
    uint32_t poolId = res->getUInt( "pool_id" );

    auto poolIt = m_tempPools.find( poolId );
    if( poolIt == m_tempPools.end() || poolIt->second.pool == nullptr )
      continue;

    auto* pool = poolIt->second.pool;

    LootTableItem item;
    item.id = res->getUInt( "item_id" );
    item.weight = res->getUInt( "weight" );
    item.isHq = res->getBoolean( "is_hq" );
    item.quantity.min = res->getUInt( "qty_min" );
    item.quantity.max = res->getUInt( "qty_max" );

    pool->items.push_back( item );

    Logger::info(
            "Loaded loot item {} weight {} for pool {}",
            item.id,
            item.weight,
            poolId );
  }

  return true;
}

LootTablePtr LootTableMgr::getLootTableByName( const std::string& name )
{
  auto it = m_lootTableMap.find( name );
  if( it == m_lootTableMap.end() )
    return nullptr;
  else
    return it->second;
}

LootTableResult LootTableMgr::rollLootForBNpc( uint32_t bnpcNameId )
{
  auto it = m_tempTableIds.find( bnpcNameId );

  if( it == m_tempTableIds.end() || !it->second )
  {
    Logger::error( "LootTable not found for BNpcNameId: {}", bnpcNameId );
    return {};
  }

  return rollLoot( it->second->lootTable );
}

LootTableResult LootTableMgr::rollLoot( const std::string& name )
{
  auto& RNGMgr = Common::Service< Common::Random::RNGMgr >::ref();

  LootTableResult result;

  auto pLootTable = getLootTableByName( name );
  if( !pLootTable )
  {
    Logger::error( "LootTable missing: {}", name );
    return {};
  }

  result.name = pLootTable->lootTable;

  for( const auto& pool : pLootTable->pools )
  {
    if( !pool.enabled )
      continue;

    if( pool.pick.min > pool.pick.max )
    {
      Logger::error( "Invalid pick range in pool {}", pool.name );
      continue;
    }

    if( pool.items.empty() )
      continue;

    auto pickGen = RNGMgr.getRandGenerator< uint32_t >( pool.pick.min, pool.pick.max );
    uint32_t picks = pickGen.next();

    std::unordered_set< uint32_t > usedItems;

    uint32_t attempts = 0;
    const uint32_t maxAttempts = picks * 3;// anti infinite skip bias

    while( result.count() < picks && attempts < maxAttempts )
    {
      attempts++;

      const auto& item = pickWeightedItem( pool.items );

      if( item.weight == 0 )
        continue;

      if( item.quantity.min > item.quantity.max )
      {
        Logger::error( "Invalid qty range item {}", item.id );
        continue;
      }

      if( !pool.duplicates )
      {
        if( usedItems.find( item.id ) != usedItems.end() )
          continue;

        usedItems.insert( item.id );
      }

      auto qtyGen = RNGMgr.getRandGenerator< uint32_t >(
              item.quantity.min,
              item.quantity.max );

      uint32_t qty = qtyGen.next();
      if( qty == 0 )
        qty = 1;

      result.items.push_back( { item.id, qty, item.isHq } );
    }
  }

  return result;
}

const LootTableItem& LootTableMgr::pickWeightedItem(
        const std::vector< LootTableItem >& items )
{
  auto& RNGMgr = Common::Service< Common::Random::RNGMgr >::ref();

  if( items.empty() )
    throw std::runtime_error( "Empty loot pool" );

  uint32_t totalWeight = 0;

  for( const auto& it : items )
    totalWeight += it.weight;

  if( totalWeight == 0 )
    throw std::runtime_error( "All loot weights are zero" );

  auto gen = RNGMgr.getRandGenerator< uint32_t >( 1, totalWeight );
  uint32_t roll = gen.next();

  uint32_t cumulative = 0;

  for( const auto& it : items )
  {
    cumulative += it.weight;
    if( roll <= cumulative )
      return it;
  }

  return items.back();// fallback safety
}

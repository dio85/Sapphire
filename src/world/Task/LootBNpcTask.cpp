#include "LootBNpcTask.h"

#include <Actor/Player.h>
#include <Logging/Logger.h>
#include <Manager/PlayerMgr.h>
#include <Manager/LootTableMgr.h>
#include <Manager/MgrUtil.h>
#include "WorldServer.h"
#include <Service.h>

using namespace Sapphire::World;
using namespace Sapphire::World::Manager;

LootBNpcTask::LootBNpcTask(Entity::Player& player, uint32_t bnpcNameId, uint64_t delayTime ) : Task( delayTime )
{
  m_playerId = player.getId();
  m_bnpcId = bnpcNameId;
}

void LootBNpcTask::onQueue()
{
  Logger::debug( { __FUNCTION__ } );
}

void LootBNpcTask::execute()
{
  auto& playerMgr = Common::Service< World::Manager::PlayerMgr >::ref();
  auto& lootTableMgr = Common::Service< World::Manager::LootTableMgr >::ref();

  auto pPlayer = playerMgr.getPlayer( m_playerId );
  if( !pPlayer )
    return;

  auto lootResult = lootTableMgr.rollLootForBNpc( m_bnpcId );

  for( const auto& item : lootResult.items )
  {
    pPlayer->addItem(
            item.id,
            item.quantity,
            item.isHq,
            false,
            true );
  }
}

std::string LootBNpcTask::toString()
{
  return fmt::format( "LootBNpcTask: PlayerId#{}, LootTable {}, ElapsedTimeMs: {}", m_playerId, m_bnpcId, getDelayTimeMs() );
}



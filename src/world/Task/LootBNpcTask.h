#pragma once

#include <cstdint>
#include <string>
#include <ForwardsZone.h>
#include "Task.h"

namespace Sapphire::World
{
  class LootBNpcTask : public Task
  {
  public:
    LootBNpcTask( Entity::Player& player, const uint32_t bnpcNameId, uint64_t delayTime );

    void onQueue() override;
    void execute() override;
    std::string toString() override;

  private:
    uint32_t m_playerId;
    uint32_t m_bnpcId;
  };

  template< typename... Args >
  std::shared_ptr< LootBNpcTask > makeLootBNpcTask( Args... args )
  {
    return std::make_shared< LootBNpcTask >( args... );
  }
}

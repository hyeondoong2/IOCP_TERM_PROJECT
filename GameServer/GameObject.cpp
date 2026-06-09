#include "pch.h"
#include "GameObject.h"
#include "ObjectManager.h"
#include "SectorManager.h"
#include "SessionManager.h"
#include "NPC.h"
#include "Player.h"
#include "Session.h"

void GameObject::SendMovePacketToViewers()
{
    S2C_MoveObject movePkt;
    movePkt.size = sizeof(S2C_MoveObject);
    movePkt.type = S2C_MOVE_OBJECT;
    movePkt.object_id = _id;
    movePkt.x = _x;
    movePkt.y = _y;
    movePkt.move_time = _lastMoveTime;
    //std::cout << _lastMoveTime << std::endl;

  
    for (auto& nearbyId : GSectorManager->GetNearbyObjectIds(shared_from_this()))
    {
        if (IsPlayer(nearbyId))
        {
            auto nearbyPlayer = GSessionManager->Find(nearbyId);
            if (nearbyPlayer)
            {
                nearbyPlayer->DoSend(reinterpret_cast<const char*>(&movePkt));
            }
        }
    }
}

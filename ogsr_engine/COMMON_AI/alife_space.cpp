#include "stdafx.h"
#include "alife_space.h"

namespace ALife
{

xr_token hit_types_token[] = {{"burn", eHitTypeBurn},
                              {"shock", eHitTypeShock},
                              {"strike", eHitTypeStrike},
                              {"wound", eHitTypeWound},
                              {"radiation", eHitTypeRadiation},
                              {"telepatic", eHitTypeTelepatic},
                              {"chemical_burn", eHitTypeChemicalBurn},
                              {"explosion", eHitTypeExplosion},
                              {"fire_wound", eHitTypeFireWound},
                              {"wound_2", eHitTypeWound_2},
                              {"physic_strike", eHitTypePhysicStrike},
                              {0, 0}};

};
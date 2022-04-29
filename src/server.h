/*
 *  ALCHYMIST
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  server.h - Provides tools for managing an internal game server.
 *  
 *  */

#include "atra.h"

/*  Features:
        Needs to be able to send packets with data (UDP):
            - Player state change
            - World state change
            - NPC state change
        Needs to be able to receive packets with data (UDP):
            - Player actions
            - World changes made
        Needs to be able to reliably send (TCP):
            - World data on initial download
            - Mod files
            - Anything which cannot drop packets
*/


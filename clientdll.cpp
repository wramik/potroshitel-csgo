#include "includes.h"

void Hooks::LevelInitPreEntity( const char* map ) {
	float rate{ 1.f / g_csgo.m_globals->m_interval };

	// set rates when joining a server.
	g_csgo.cl_updaterate->SetValue( rate );
	g_csgo.cl_cmdrate->SetValue( rate );

	g_aimbot.reset( );
	g_visuals.m_hit_start = g_visuals.m_hit_end = g_visuals.m_hit_duration = 0.f;

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelInitPreEntity_t >( CHLClient::LEVELINITPREENTITY )( this, map );
}

void Hooks::LevelInitPostEntity( ) {
	g_cl.OnMapload( );

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelInitPostEntity_t >( CHLClient::LEVELINITPOSTENTITY )( this );
}

void Hooks::LevelShutdown( ) {
	g_aimbot.reset( );

	g_cl.m_local       = nullptr;
	g_cl.m_weapon      = nullptr;
	g_cl.m_processing  = false;
	g_cl.m_weapon_info = nullptr;
	g_cl.m_round_end   = false;

	g_cl.m_sequences.clear( );

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelShutdown_t >( CHLClient::LEVELSHUTDOWN )( this );
}

void Hooks::FrameStageNotify( Stage_t stage ) {
	// save stage.
	if( stage != FRAME_START )
		g_cl.m_stage = stage;

	// damn son.
	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );

	static auto extended_fakelag_packets = [ ]( ) -> void {
		static bool unlocked_fakelag = false;

		if ( unlocked_fakelag )
			return;

		if ( !unlocked_fakelag ) {
			auto cl_move_clamp = pattern::find( g_csgo.m_engine_dll, XOR( "B8 ? ? ? ? 3B F0 0F 4F F0 89 5D FC" ) ) + 1;
			unsigned long protect = 0;

			VirtualProtect( ( void* )cl_move_clamp, 4, PAGE_EXECUTE_READWRITE, &protect );
			*( std::uint32_t* )cl_move_clamp = 62;
			VirtualProtect( ( void* )cl_move_clamp, 4, protect, &protect );
			unlocked_fakelag = true;
		}
	};

	if( stage == FRAME_RENDER_START ) {	
		// apply local player animated angles.
		g_cl.SetAngles( );

		// apply local player animation fix.
		g_cl.UpdateAnimations( );

        // draw our custom beams.
        g_visuals.DrawBeams( );

		// gavno.
		extended_fakelag_packets( );
	}

	// call og.
	g_hooks.m_client.GetOldMethod< FrameStageNotify_t >( CHLClient::FRAMESTAGENOTIFY )( this, stage );

	if( stage == FRAME_RENDER_START ) {
		// ...
	}

	else if( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START ) {
		// restore non-compressed netvars.
		// g_netdata.apply( );

		g_skins.think( );
	}

	else if( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_END ) {
		g_visuals.NoSmoke( );
	}

	else if( stage == FRAME_NET_UPDATE_END ) {
        // restore non-compressed netvars and detect prediction errors.
		g_inputpred.detect_prediction_error( );

		// update all players.
		for( int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i ) {
			Player* player = g_csgo.m_entlist->GetClientEntity< Player* >( i );
			if( !player || player->m_bIsLocalPlayer( ) )
				continue;

			AimPlayer* data = &g_aimbot.m_players[ i - 1 ];
			data->OnNetUpdate( player );
		}
	}
}
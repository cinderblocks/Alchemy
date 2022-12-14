/** 
 * @file llviewerdisplay.cpp
 * @brief LLViewerDisplay class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llviewerdisplay.h"

#include "llgl.h"
#include "llrender.h"
#include "alglmath.h"
#include "llglheaders.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llviewercontrol.h"
#include "llcoord.h"
#include "llcriticaldamp.h"
#include "lldir.h"
#include "lldynamictexture.h"
#include "lldrawpoolalpha.h"
#include "llfeaturemanager.h"
//#include "llfirstuse.h"
#include "llfloaterprogressview.h"
#include "llfloaterreg.h"
#include "llhudmanager.h"
#include "llimagepng.h"
#include "llmemory.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llstartup.h"
#include "lltoolfocus.h"
#include "lltoolmgr.h"
#include "lltooldraganddrop.h"
#include "lltoolpie.h"
#include "lltracker.h"
#include "lltrans.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llvograss.h"
#include "llworld.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llappviewer.h"
#include "llstartup.h"
#include "llviewershadermgr.h"
#include "llfasttimer.h"
#include "llfloatertools.h"
#include "llviewertexturelist.h"
#include "llfocusmgr.h"
#include "llcubemap.h"
#include "llviewerregion.h"
#include "lldrawpoolwater.h"
#include "lldrawpoolbump.h"
#include "llscenemonitor.h"
// [RLVa:KB] - Checked: 2011-05-22 (RLVa-1.3.1a)
#include "llvisualeffect.h"
#include "rlvactions.h"
#include "rlvlocks.h"
// [/RLVa:KB]

#include "llenvironment.h"
#include "alcinematicmode.h"

extern LLPointer<LLViewerTexture> gStartTexture;
extern bool gShiftFrame;

LLPointer<LLViewerTexture> gDisconnectedImagep = NULL;

// used to toggle renderer back on after teleport
BOOL		 gTeleportDisplay = FALSE;
LLFrameTimer gTeleportDisplayTimer;
LLFrameTimer gTeleportArrivalTimer;
const F32		RESTORE_GL_TIME = 5.f;	// Wait this long while reloading textures before we raise the curtain

BOOL gForceRenderLandFence = FALSE;
BOOL gDisplaySwapBuffers = FALSE;
BOOL gDepthDirty = FALSE;
BOOL gResizeScreenTexture = FALSE;
BOOL gResizeShadowTexture = FALSE;
BOOL gWindowResized = FALSE;
BOOL gSnapshot = FALSE;
BOOL gShaderProfileFrame = FALSE;

// This is how long the sim will try to teleport you before giving up.
const F32 TELEPORT_EXPIRY = 15.0f;
// Additional time (in seconds) to wait per attachment
const F32 TELEPORT_EXPIRY_PER_ATTACHMENT = 3.f;

U32 gRecentFrameCount = 0; // number of 'recent' frames
LLFrameTimer gRecentFPSTime;
LLFrameTimer gRecentMemoryTime;
LLFrameTimer gAssetStorageLogTime;

// Rendering stuff
void pre_show_depth_buffer();
void post_show_depth_buffer();
void render_ui(F32 zoom_factor = 1.f, int subfield = 0);
void swap();
void render_hud_attachments();
void render_ui_3d();
void render_ui_2d();
void render_disconnected_background();

void display_startup()
{
	if (   !gViewerWindow
		|| !gViewerWindow->getActive()
		|| !gViewerWindow->getWindow()->getVisible() 
		|| gViewerWindow->getWindow()->getMinimized() )
	{
		return; 
	}

	gPipeline.updateGL();

	// Written as branch to appease GCC which doesn't like different
	// pointer types across ternary ops
	//
	if (!LLViewerFetchedTexture::sWhiteImagep.isNull())
	{
	LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();
	}

	LLGLSDefault gls_default;

	// Required for HTML update in login screen
	static S32 frame_count = 0;

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	if (frame_count++ > 1) // make sure we have rendered a frame first
	{
		LLViewerDynamicTexture::updateAllInstances();
	}
    else
    {
        LL_DEBUGS("Window") << "First display_startup frame" << LL_ENDL;
    }

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	LLGLSUIDefault gls_ui;
	gPipeline.disableLights();

	if (gViewerWindow)
	gViewerWindow->setup2DRender();
	gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);

	gGL.color4f(1,1,1,1);
	if (gViewerWindow)
	gViewerWindow->draw();
	gGL.flush();

	LLVertexBuffer::unbind();

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	if (gViewerWindow && gViewerWindow->getWindow())
	gViewerWindow->getWindow()->swapBuffers();

	glClear(GL_DEPTH_BUFFER_BIT);
}

static LLTrace::BlockTimerStatHandle FTM_UPDATE_CAMERA("Update Camera");

void display_update_camera()
{
	LL_RECORD_BLOCK_TIME(FTM_UPDATE_CAMERA);
	// TODO: cut draw distance down if customizing avatar?
	// TODO: cut draw distance on per-parcel basis?

	// Cut draw distance in half when customizing avatar,
	// but on the viewer only.
	F32 final_far = gAgentCamera.mDrawDistance;
	if (CAMERA_MODE_CUSTOMIZE_AVATAR == gAgentCamera.getCameraMode())
	{
		final_far *= 0.5f;
	}
	LLViewerCamera::getInstanceFast()->setFar(final_far);
	gViewerWindow->setup3DRender();
	
	// Update land visibility too
	LLWorld::getInstanceFast()->setLandFarClip(final_far);
}

// Write some stats to LL_INFOS()
void display_stats()
{
	static const LLCachedControl<F32> fps_log_freq(gSavedSettings, "FPSLogFrequency");
	if (fps_log_freq > 0.f && gRecentFPSTime.getElapsedTimeF32() >= fps_log_freq)
	{
		F32 fps = gRecentFrameCount / fps_log_freq;
		LL_INFOS() << llformat("FPS: %.02f", fps) << LL_ENDL;
		gRecentFrameCount = 0;
		gRecentFPSTime.reset();
	}
	static const LLCachedControl<F32> mem_log_freq(gSavedSettings, "MemoryLogFrequency");
	if (mem_log_freq > 0.f && gRecentMemoryTime.getElapsedTimeF32() >= mem_log_freq)
	{
		gMemoryAllocated = U64Bytes(LLMemory::getCurrentRSS());
		U32Megabytes memory = gMemoryAllocated;
		LL_INFOS() << "MEMORY: " << memory << LL_ENDL;
		LLMemory::logMemoryInfo(TRUE) ;
		gRecentMemoryTime.reset();
	}
    static const LLCachedControl<F32> asset_storage_log_freq(gSavedSettings, "AssetStorageLogFrequency");
    if (asset_storage_log_freq > 0.f && gAssetStorageLogTime.getElapsedTimeF32() >= asset_storage_log_freq)
    {
        gAssetStorageLogTime.reset();
        gAssetStorage->logAssetStorageInfo();
    }
}

static LLTrace::BlockTimerStatHandle FTM_PICK("Picking");
static LLTrace::BlockTimerStatHandle FTM_RENDER("Render");
static LLTrace::BlockTimerStatHandle FTM_RENDER_HUD("Render HUD");
static LLTrace::BlockTimerStatHandle FTM_UPDATE_SKY("Update Sky");
static LLTrace::BlockTimerStatHandle FTM_UPDATE_DYNAMIC_TEXTURES("Update Dynamic Textures");
static LLTrace::BlockTimerStatHandle FTM_IMAGE_UPDATE("Update Images");
static LLTrace::BlockTimerStatHandle FTM_IMAGE_UPDATE_CLASS("Class");
static LLTrace::BlockTimerStatHandle FTM_IMAGE_UPDATE_BUMP("Image Update Bump");
static LLTrace::BlockTimerStatHandle FTM_IMAGE_UPDATE_LIST("List");
static LLTrace::BlockTimerStatHandle FTM_IMAGE_UPDATE_DELETE("Delete");
static LLTrace::BlockTimerStatHandle FTM_RESIZE_WINDOW("Resize Window");
static LLTrace::BlockTimerStatHandle FTM_HUD_UPDATE("HUD Update");
static LLTrace::BlockTimerStatHandle FTM_DISPLAY_UPDATE_GEOM("Update Geom");
static LLTrace::BlockTimerStatHandle FTM_TEXTURE_UNBIND("Texture Unbind");
static LLTrace::BlockTimerStatHandle FTM_TELEPORT_DISPLAY("Teleport Display");
static LLTrace::BlockTimerStatHandle FTM_EEP_UPDATE("Env Update");

static std::string STR_DISPLAY_PICK("Display:Pick");
static std::string STR_DISPLAY_CHECK_STATES("Display:CheckStates");
static std::string STR_DISPLAY_STARTUP("Display:Startup");
static std::string STR_DISPLAY_TEXTURE_STATS("Display:TextureStats");
static std::string STR_DISPLAY_TELEPORT("Display:Teleport");
static std::string STR_DISPLAY_LOGOUT("Display:Logout");
static std::string STR_DISPLAY_RESTOREGL("Display:RestoreGL");
static std::string STR_DISPLAY_CAMERA("Display:Camera");
static std::string STR_DISPLAY_DISCONNECTED("Display:Disconnected");
static std::string STR_DISPLAY_RENDER_SETUP("Display:RenderSetup");
static std::string STR_DISPLAY_DYN_TEX("Display:DynamicTextures");
static std::string STR_DISPLAY_UPDATE("Display:Update");
static std::string STR_DISPLAY_CULL("Display:Cull");
static std::string STR_DISPLAY_SWAP("Display:Swap");
static std::string STR_DISPLAY_IMAGERY("Display:Imagery");
static std::string STR_DISPLAY_UPDATE_IMAGES("Display:UpdateImages");
static std::string STR_DISPLAY_STATE_SORT("Display:StateSort");
static std::string STR_DISPLAY_SKY("Display:Sky");
static std::string STR_DISPLAY_RENDER_START("Display:RenderStart");
static std::string STR_DISPLAY_RENDER_GEOM("Display:RenderGeom");
static std::string STR_DISPLAY_RENDER_FLUSH("Display:RenderFlush");
static std::string STR_DISPLAY_RENDERUI("Display:RenderUI");
static std::string STR_DISPLAY_FRAME_STATS("Display:FrameStats");
static std::string STR_DISPLAY_DONE("Display:Done");

// Paint the display!
void display(BOOL rebuild, F32 zoom_factor, int subfield, BOOL for_snapshot)
{
	LL_ALWAYS_RECORD_BLOCK_TIME(FTM_RENDER);
	LLVBOPool::deleteReleasedBuffers();

	if (gWindowResized)
	{ //skip render on frames where window has been resized
		LL_DEBUGS("Window") << "Resizing window" << LL_ENDL;
		LL_RECORD_BLOCK_TIME(FTM_RESIZE_WINDOW);
		gGL.flush();
		glClear(GL_COLOR_BUFFER_BIT);
		gViewerWindow->getWindow()->swapBuffers();
		LLPipeline::refreshCachedSettings();
		gPipeline.resizeScreenTexture();
		gResizeScreenTexture = FALSE;
		gWindowResized = FALSE;
		return;
	}

    if (gResizeShadowTexture)
	{ //skip render on frames where window has been resized
		gPipeline.resizeShadowTexture();
		gResizeShadowTexture = FALSE;
	}

	if (LLPipeline::sRenderDeferred)
	{ //hack to make sky show up in deferred snapshots
		for_snapshot = FALSE;
	}

	if (LLPipeline::sRenderFrameTest)
	{
		send_agent_pause();
	}

	gSnapshot = for_snapshot;

	LLGLSDefault gls_default;
	LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE, GL_LEQUAL);
	
	LLVertexBuffer::unbind();

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	
	stop_glerror();

	gPipeline.disableLights();
	
	//reset vertex buffers if needed
	gPipeline.doResetVertexBuffers();

	stop_glerror();

	// Don't draw if the window is hidden or minimized.
	// In fact, must explicitly check the minimized state before drawing.
	// Attempting to draw into a minimized window causes a GL error. JC
	if (   !gViewerWindow->getActive()
		|| !gViewerWindow->getWindow()->getVisible() 
		|| gViewerWindow->getWindow()->getMinimized() )
	{
		// Clean up memory the pools may have allocated
		if (rebuild)
		{
			stop_glerror();
			gPipeline.rebuildPools();
			stop_glerror();
		}

		LLHUDObject::renderAllForTimer();

		stop_glerror();
		gViewerWindow->returnEmptyPicks();
		stop_glerror();
		return; 
	}
	
	gViewerWindow->checkSettings();

	{
		LL_RECORD_BLOCK_TIME(FTM_PICK);
		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_PICK);
		gViewerWindow->performPick();
	}
	
	LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_CHECK_STATES);
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	
	//////////////////////////////////////////////////////////
	//
	// Logic for forcing window updates if we're in drone mode.
	//

	// *TODO: Investigate running display() during gHeadlessClient.  See if this early exit is needed DK 2011-02-18
	if (gHeadlessClient) 
	{
#if LL_WINDOWS
		static F32 last_update_time = 0.f;
		if ((gFrameTimeSeconds - last_update_time) > 1.f)
		{
			InvalidateRect((HWND)gViewerWindow->getPlatformWindow(), NULL, FALSE);
			last_update_time = gFrameTimeSeconds;
		}
#elif LL_DARWIN
		// MBW -- Do something clever here.
#endif
		// Not actually rendering, don't bother.
		return;
	}


	//
	// Bail out if we're in the startup state and don't want to try to
	// render the world.
	//
	if (LLStartUp::getStartupState() < STATE_PRECACHE)
	{
		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_STARTUP);
		display_startup();
		return;
	}


	if (gShaderProfileFrame)
	{
		LLGLSLShader::initProfile();
	}

	//LLGLState::verify(FALSE);

	/////////////////////////////////////////////////
	//
	// Update GL Texture statistics (used for discard logic?)
	//

	LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_TEXTURE_STATS);
	stop_glerror();

	LLImageGL::updateStats(gFrameTimeSeconds);
	
	static const LLCachedControl<S32> av_name_tag_mode(gSavedSettings, "AvatarNameTagMode");
	static const LLCachedControl<bool> name_tag_show_grp_title(gSavedSettings, "NameTagShowGroupTitles");

	LLVOAvatar::sRenderName = (ALCinematicMode::isEnabled() ? 0 : (S32)av_name_tag_mode);;
	LLVOAvatar::sRenderGroupTitles = (name_tag_show_grp_title && av_name_tag_mode);
	
	gPipeline.mBackfaceCull = TRUE;
	gFrameCount++;
	gRecentFrameCount++;
	if (gFocusMgr.getAppHasFocus())
	{
		gForegroundFrameCount++;
	}

	//////////////////////////////////////////////////////////
	//
	// Display start screen if we're teleporting, and skip render
	//

	if (gTeleportDisplay)
	{
		LL_RECORD_BLOCK_TIME(FTM_TELEPORT_DISPLAY);
		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_TELEPORT);
		static LLCachedControl<F32> teleport_arrival_delay(gSavedSettings, "TeleportArrivalDelay");
		static LLCachedControl<F32> teleport_local_delay(gSavedSettings, "TeleportLocalDelay");

		S32 attach_count = 0;
		if (isAgentAvatarValid())
		{
			attach_count = gAgentAvatarp->getAttachmentCount();
		}
		F32 teleport_save_time = TELEPORT_EXPIRY + TELEPORT_EXPIRY_PER_ATTACHMENT * attach_count;
		F32 teleport_elapsed = gTeleportDisplayTimer.getElapsedTimeF32();
		F32 teleport_percent = teleport_elapsed * (100.f / teleport_save_time);
		if( (gAgent.getTeleportState() != LLAgent::TELEPORT_START) && (teleport_percent > 100.f) )
		{
			// Give up.  Don't keep the UI locked forever.
			LL_WARNS("Teleport") << "Giving up on teleport. elapsed time " << teleport_elapsed << " exceeds max time " << teleport_save_time << LL_ENDL;
			gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
			gAgent.setTeleportMessage(LLStringUtil::null);
		}

		LLFloaterProgressView* pProgFloater = LLFloaterReg::getTypedInstance<LLFloaterProgressView>("progress_view");
		
		const std::string& message = gAgent.getTeleportMessage();
		switch( gAgent.getTeleportState() )
		{
		case LLAgent::TELEPORT_PENDING:
			gTeleportDisplayTimer.reset();
			pProgFloater->setVisible(TRUE);
			pProgFloater->setProgressPercent(llmin(teleport_percent, 0.f));
			gAgent.setTeleportMessage(LLAgent::sTeleportProgressMessages["pending"]);
			pProgFloater->setProgressText(LLAgent::sTeleportProgressMessages["pending"]);
			break;

		case LLAgent::TELEPORT_START:
			// Transition to REQUESTED.  Viewer has sent some kind
			// of TeleportRequest to the source simulator
			gTeleportDisplayTimer.reset();
			pProgFloater->setVisible(TRUE);
			pProgFloater->setProgressPercent(llmin(teleport_percent, 0.f));
			LL_INFOS("Teleport") << "A teleport request has been sent, setting state to TELEPORT_REQUESTED" << LL_ENDL;
			gAgent.setTeleportState( LLAgent::TELEPORT_REQUESTED );
			gAgent.setTeleportMessage(
				LLAgent::sTeleportProgressMessages["requesting"]);
			pProgFloater->setProgressText(LLAgent::sTeleportProgressMessages["requesting"]);
			break;

		case LLAgent::TELEPORT_REQUESTED:
			// Waiting for source simulator to respond
			pProgFloater->setProgressPercent(llmin(teleport_percent, 37.5f));
			pProgFloater->setProgressText(message);
			break;

		case LLAgent::TELEPORT_MOVING:
			// Viewer has received destination location from source simulator
			pProgFloater->setProgressPercent(llmin(teleport_percent, 75.f));
			pProgFloater->setProgressText(message);
			break;

		case LLAgent::TELEPORT_START_ARRIVAL:
			// Transition to ARRIVING.  Viewer has received avatar update, etc., from destination simulator
			gTeleportArrivalTimer.reset();
pProgFloater->setProgressCancelButtonVisible(FALSE, LLTrans::getString("Cancel"));
			pProgFloater->setProgressPercent(75.f);
			LL_INFOS("Teleport") << "Changing state to TELEPORT_ARRIVING" << LL_ENDL;
			gAgent.setTeleportState( LLAgent::TELEPORT_ARRIVING );
			gAgent.setTeleportMessage(
				LLAgent::sTeleportProgressMessages["arriving"]);
			gAgent.sheduleTeleportIM();
			gTextureList.mForceResetTextureStats = TRUE;
			gAgentCamera.resetView(TRUE, TRUE);
			
			break;

		case LLAgent::TELEPORT_ARRIVING:
			// Make the user wait while content "pre-caches"
			{
				F32 arrival_fraction = (gTeleportArrivalTimer.getElapsedTimeF32() / teleport_arrival_delay());
				if( arrival_fraction > 1.f )
				{
					arrival_fraction = 1.f;
					//LLFirstUse::useTeleport();
					LL_INFOS("Teleport") << "arrival_fraction is " << arrival_fraction << " changing state to TELEPORT_NONE" << LL_ENDL;
					gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
				}
				pProgFloater->setProgressCancelButtonVisible(FALSE, LLTrans::getString("Cancel"));
				pProgFloater->setProgressPercent(arrival_fraction * 25.f + 75.f);
				pProgFloater->setProgressText(message);
			}
			break;

		case LLAgent::TELEPORT_LOCAL:
			// Short delay when teleporting in the same sim (progress screen active but not shown - did not
			// fall-through from TELEPORT_START)
			{
				if( gTeleportDisplayTimer.getElapsedTimeF32() > teleport_local_delay() )
				{
					//LLFirstUse::useTeleport();
					LL_INFOS("Teleport") << "State is local and gTeleportDisplayTimer " << gTeleportDisplayTimer.getElapsedTimeF32()
										 << " exceeds teleport_local_delete " << teleport_local_delay
										 << "; setting state to TELEPORT_NONE"
										 << LL_ENDL;
					gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
				}
			}
			break;

		case LLAgent::TELEPORT_NONE:
			// No teleport in progress
			pProgFloater->setVisible(FALSE);
			gTeleportDisplay = FALSE;
// [SL:KB] - Patch: Appearance-TeleportAttachKill | Checked: Catznip-4.0
			LLViewerParcelMgr::getInstance()->onTeleportDone();
// [/SL:KB]
			break;
		}
	}
    else if(LLAppViewer::instance()->logoutRequestSent())
	{
		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_LOGOUT);
		F32 percent_done = gLogoutTimer.getElapsedTimeF32() * 100.f / gLogoutMaxTime;
		if (percent_done > 100.f)
		{
			percent_done = 100.f;
		}

		if( LLApp::isExiting() )
		{
			percent_done = 100.f;
		}
		
		gViewerWindow->setProgressPercent( percent_done );
		gViewerWindow->setProgressMessage(LLStringUtil::null);
	}
	else
	if (gRestoreGL)
	{
		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_RESTOREGL);
		F32 percent_done = gRestoreGLTimer.getElapsedTimeF32() * 100.f / RESTORE_GL_TIME;
		if( percent_done > 100.f )
		{
			gViewerWindow->setShowProgress(FALSE);
			gRestoreGL = FALSE;
		}
		else
		{

			if( LLApp::isExiting() )
			{
				percent_done = 100.f;
			}
			
			gViewerWindow->setProgressPercent( percent_done );
		}
		gViewerWindow->setProgressMessage(LLStringUtil::null);
	}

	//////////////////////////
	//
	// Prepare for the next frame
	//

	/////////////////////////////
	//
	// Update the camera
	//
	//

	LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_CAMERA);
	auto& vwrCamera = LLViewerCamera::instanceFast();
	vwrCamera.setZoomParameters(zoom_factor, subfield);
	vwrCamera.setNear(MIN_NEAR_PLANE);

	//////////////////////////
	//
	// clear the next buffer
	// (must follow dynamic texture writing since that uses the frame buffer)
	//

	if (gDisconnected)
	{
		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_DISCONNECTED);
		render_ui();
		swap();
	}
	
	//////////////////////////
	//
	// Set rendering options
	//
	//
	LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_RENDER_SETUP);
	stop_glerror();

	///////////////////////////////////////
	//
	// Slam lighting parameters back to our defaults.
	// Note that these are not the same as GL defaults...

	stop_glerror();
	gGL.setAmbientLightColor(LLColor4::white);
	stop_glerror();
			
	/////////////////////////////////////
	//
	// Render
	//
	// Actually push all of our triangles to the screen.
	//

	// do render-to-texture stuff here
	if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_DYNAMIC_TEXTURES))
	{
		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_DYN_TEX);
		LL_RECORD_BLOCK_TIME(FTM_UPDATE_DYNAMIC_TEXTURES);
		if (LLViewerDynamicTexture::updateAllInstances())
		{
			gGL.setColorMask(true, true);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
	}

	gViewerWindow->setup3DViewport();

	gPipeline.resetFrameStats();	// Reset per-frame statistics.
	
	if (!gDisconnected)
	{
		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_UPDATE);
		if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
		{ //don't draw hud objects in this frame
			gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
		}

		if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES))
		{ //don't draw hud particles in this frame
			gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
		}

		stop_glerror();
		display_update_camera();
		stop_glerror();
				
		{
			LL_RECORD_BLOCK_TIME(FTM_EEP_UPDATE);
            // update all the sky/atmospheric/water settings
            LLEnvironment::getInstanceFast()->update(&vwrCamera);
		}

		// *TODO: merge these two methods
		{
			LL_RECORD_BLOCK_TIME(FTM_HUD_UPDATE);
			LLHUDManager::getInstance()->updateEffects();
			LLHUDObject::updateAll();
			stop_glerror();
		}

		{
			LL_RECORD_BLOCK_TIME(FTM_DISPLAY_UPDATE_GEOM);
			const F32 max_geom_update_time = 0.005f*10.f*gFrameIntervalSeconds.value(); // 50 ms/second update time
			gPipeline.createObjects(max_geom_update_time);
			gPipeline.processPartitionQ();
			gPipeline.updateGeom(max_geom_update_time);
			stop_glerror();
		}

		gPipeline.updateGL();
		
		stop_glerror();

		S32 water_clip = 0;
		if ((LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_ENVIRONMENT) > 1) &&
			 (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_WATER) || 
			  gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_VOIDWATER)))
		{
			if (vwrCamera.cameraUnderWater())
			{
				water_clip = -1;
			}
			else
			{
				water_clip = 1;
			}
		}
		
		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_CULL);
		
		//Increment drawable frame counter
		LLDrawable::incrementVisible();

		LLSpatialGroup::sNoDelete = TRUE;
		LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();

		S32 occlusion = LLPipeline::sUseOcclusion;
		if (gDepthDirty)
		{ //depth buffer is invalid, don't overwrite occlusion state
			LLPipeline::sUseOcclusion = llmin(occlusion, 1);
		}
		gDepthDirty = FALSE;

		LLGLState::checkStates();
		LLGLState::checkTextureChannels();
		LLGLState::checkClientArrays();

		static LLCullResult result;
		LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
		LLPipeline::sUnderWaterRender = vwrCamera.cameraUnderWater();
		gPipeline.updateCull(vwrCamera, result, water_clip);
		stop_glerror();

		LLGLState::checkStates();
		LLGLState::checkTextureChannels();
		LLGLState::checkClientArrays();

		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_SWAP);
		
		{ 
			if (gResizeScreenTexture)
			{
				gResizeScreenTexture = FALSE;
				gPipeline.resizeScreenTexture();
			}

			gGL.setColorMask(true, true);
			glClearColor(0,0,0,0);

			LLGLState::checkStates();
			LLGLState::checkTextureChannels();
			LLGLState::checkClientArrays();

			if (!for_snapshot)
			{
				if (gFrameCount > 1)
				{ //for some reason, ATI 4800 series will error out if you 
				  //try to generate a shadow before the first frame is through
					gPipeline.generateSunShadow(vwrCamera);
				}

				LLVertexBuffer::unbind();

				LLGLState::checkStates();
				LLGLState::checkTextureChannels();
				LLGLState::checkClientArrays();

				LLMatrix4a proj = get_current_projection();
				LLMatrix4a mod = get_current_modelview();
				glViewport(0,0,512,512);
				LLVOAvatar::updateFreezeCounter() ;

				LLVOAvatar::updateImpostors();

				set_current_projection(proj);
				set_current_modelview(mod);
				gGL.matrixMode(LLRender::MM_PROJECTION);
				gGL.loadMatrix(proj);
				gGL.matrixMode(LLRender::MM_MODELVIEW);
				gGL.loadMatrix(mod);
				gViewerWindow->setup3DViewport();

				LLGLState::checkStates();
				LLGLState::checkTextureChannels();
				LLGLState::checkClientArrays();

			}
			glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}

		LLGLState::checkStates();
		LLGLState::checkClientArrays();

		//if (!for_snapshot)
		{
			LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_IMAGERY);
			gPipeline.generateWaterReflection(vwrCamera);
			gPipeline.generateHighlight(vwrCamera);
			gPipeline.renderPhysicsDisplay();
		}

		LLGLState::checkStates();
		LLGLState::checkClientArrays();

		//////////////////////////////////////
		//
		// Update images, using the image stats generated during object update/culling
		//
		// Can put objects onto the retextured list.
		//
		// Doing this here gives hardware occlusion queries extra time to complete
		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_UPDATE_IMAGES);
		
		{
			LL_RECORD_BLOCK_TIME(FTM_IMAGE_UPDATE);
			
			{
				LL_RECORD_BLOCK_TIME(FTM_IMAGE_UPDATE_CLASS);
				LLTrace::CountStatHandle<>* velocity_stat = LLViewerCamera::getVelocityStat();
				LLTrace::CountStatHandle<>* angular_velocity_stat = LLViewerCamera::getAngularVelocityStat();
				LLViewerTexture::updateClass(LLTrace::get_frame_recording().getPeriodMeanPerSec(*velocity_stat),
											LLTrace::get_frame_recording().getPeriodMeanPerSec(*angular_velocity_stat));
			}

			
			{
				LL_RECORD_BLOCK_TIME(FTM_IMAGE_UPDATE_BUMP);
				gBumpImageList.updateImages();  // must be called before gTextureList version so that it's textures are thrown out first.
			}

			{
				LL_RECORD_BLOCK_TIME(FTM_IMAGE_UPDATE_LIST);
				F32 max_image_decode_time = 0.050f*gFrameIntervalSeconds.value(); // 50 ms/second decode time
				max_image_decode_time = llclamp(max_image_decode_time, 0.002f, 0.005f ); // min 2ms/frame, max 5ms/frame)
				gTextureList.updateImages(max_image_decode_time);
			}

			/*{
				LL_RECORD_BLOCK_TIME(FTM_IMAGE_UPDATE_DELETE);
				//remove dead textures from GL
				LLImageGL::deleteDeadTextures();
				stop_glerror();
			}*/
			}

		LLGLState::checkStates();
		LLGLState::checkClientArrays();

		///////////////////////////////////
		//
		// StateSort
		//
		// Responsible for taking visible objects, and adding them to the appropriate draw orders.
		// In the case of alpha objects, z-sorts them first.
		// Also creates special lists for outlines and selected face rendering.
		//
		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_STATE_SORT);
		{
			LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
			gPipeline.stateSort(vwrCamera, result);
			stop_glerror();
				
			if (rebuild)
			{
				//////////////////////////////////////
				//
				// rebuildPools
				//
				//
				gPipeline.rebuildPools();
				stop_glerror();
			}
		}

		LLSceneMonitor::getInstanceFast()->fetchQueryResult();
		
		LLGLState::checkStates();
		LLGLState::checkClientArrays();

		LLPipeline::sUseOcclusion = occlusion;

		{
			LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_SKY);
			LL_RECORD_BLOCK_TIME(FTM_UPDATE_SKY);	
			gSky.updateSky();
		}

		if(gUseWireframe)
		{
			glClearColor(0.5f, 0.5f, 0.5f, 0.f);
			glClear(GL_COLOR_BUFFER_BIT);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		LLVBOPool::deleteReleasedBuffers();

		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_RENDER_START);
		
		//// render frontmost floater opaque for occlusion culling purposes
		//LLFloater* frontmost_floaterp = gFloaterView->getFrontmost();
		//// assumes frontmost floater with focus is opaque
		//if (frontmost_floaterp && gFocusMgr.childHasKeyboardFocus(frontmost_floaterp))
		//{
		//	gGL.matrixMode(LLRender::MM_MODELVIEW);
		//	gGL.pushMatrix();
		//	{
		//		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		//		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
		//		gGL.loadIdentity();

		//		LLRect floater_rect = frontmost_floaterp->calcScreenRect();
		//		// deflate by one pixel so rounding errors don't occlude outside of floater extents
		//		floater_rect.stretch(-1);
		//		LLRectf floater_3d_rect((F32)floater_rect.mLeft / (F32)gViewerWindow->getWindowWidthScaled(), 
		//								(F32)floater_rect.mTop / (F32)gViewerWindow->getWindowHeightScaled(),
		//								(F32)floater_rect.mRight / (F32)gViewerWindow->getWindowWidthScaled(),
		//								(F32)floater_rect.mBottom / (F32)gViewerWindow->getWindowHeightScaled());
		//		floater_3d_rect.translate(-0.5f, -0.5f);
		//		gGL.translatef(0.f, 0.f, -vwrCamera.getNear());
		//		gGL.scalef(vwrCamera.getNear() * vwrCamera.getAspect() / sinf(vwrCamera.getView()), vwrCamera.getNear() / sinf(vwrCamera.getView()), 1.f);
		//		gGL.color4fv(LLColor4::white.mV);
		//		gGL.begin(LLVertexBuffer::QUADS);
		//		{
		//			gGL.vertex3f(floater_3d_rect.mLeft, floater_3d_rect.mBottom, 0.f);
		//			gGL.vertex3f(floater_3d_rect.mLeft, floater_3d_rect.mTop, 0.f);
		//			gGL.vertex3f(floater_3d_rect.mRight, floater_3d_rect.mTop, 0.f);
		//			gGL.vertex3f(floater_3d_rect.mRight, floater_3d_rect.mBottom, 0.f);
		//		}
		//		gGL.end();
		//		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		//	}
		//	gGL.popMatrix();
		//}

		LLPipeline::sUnderWaterRender = vwrCamera.cameraUnderWater() ? TRUE : FALSE;

		LLGLState::checkStates();
		LLGLState::checkClientArrays();

		stop_glerror();

        gGL.setColorMask(true, true);

        if (LLPipeline::sRenderDeferred)
        {
            gPipeline.mDeferredScreen.bindTarget();
            glClearColor(1, 0, 1, 1);
            gPipeline.mDeferredScreen.clear();
        }
        else
        {
            gPipeline.mScreen.bindTarget();
            if (LLPipeline::sUnderWaterRender && !gPipeline.canUseWindLightShaders())
            {
                const LLColor3 col = LLEnvironment::getInstanceFast()->getCurrentWater()->getWaterFogColor();
                glClearColor(col.mV[0], col.mV[1], col.mV[2], 0.f);
            }
            gPipeline.mScreen.clear();
        }

        gGL.setColorMask(true, false);

		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_RENDER_GEOM);
		
		if (!(LLAppViewer::instance()->logoutRequestSent() && LLAppViewer::instance()->hasSavedFinalSnapshot())
				&& !gRestoreGL)
		{
			LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;

			static LLCachedControl<bool> renderDepthPrePass(gSavedSettings, "RenderDepthPrePass");
			if (renderDepthPrePass && LLGLSLShader::sNoFixedFunction)
			{
				gGL.setColorMask(false, false);

				static const U32 types[] = { 
					LLRenderPass::PASS_SIMPLE, 
					LLRenderPass::PASS_FULLBRIGHT, 
					LLRenderPass::PASS_SHINY 
				};

				U32 num_types = LL_ARRAY_SIZE(types);
				gOcclusionProgram.bind();
				for (U32 i = 0; i < num_types; i++)
				{
					gPipeline.renderObjects(types[i], LLVertexBuffer::MAP_VERTEX, FALSE);
				}

				gOcclusionProgram.unbind();
			}


			gGL.setColorMask(true, false);
			if (LLPipeline::sRenderDeferred)
			{
				gPipeline.renderGeomDeferred(vwrCamera);
			}
			else
			{
				gPipeline.renderGeom(vwrCamera, TRUE);
			}
			
			gGL.setColorMask(true, true);

			//store this frame's modelview matrix for use
			//when rendering next frame's occlusion queries
			set_last_modelview(get_current_modelview());
			set_last_projection(get_current_projection());

			stop_glerror();
		}

		{
			LL_RECORD_BLOCK_TIME(FTM_TEXTURE_UNBIND);
			for (U32 i = 0; i < gGLManager.mNumTextureImageUnits; i++)
			{ //dummy cleanup of any currently bound textures
				if (gGL.getTexUnit(i)->getCurrType() != LLTexUnit::TT_NONE)
				{
					gGL.getTexUnit(i)->unbind(gGL.getTexUnit(i)->getCurrType());
					gGL.getTexUnit(i)->disable();
				}
			}
		}

		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_RENDER_FLUSH);

        LLRenderTarget &rt = (gPipeline.sRenderDeferred ? gPipeline.mDeferredScreen : gPipeline.mScreen);
        rt.flush();

        if (rt.sUseFBO)
        {
            LLRenderTarget::copyContentsToFramebuffer(rt, 0, 0, rt.getWidth(), rt.getHeight(), 0, 0, rt.getWidth(),
                                                      rt.getHeight(), GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
                                                      GL_NEAREST);
        }

        if (LLPipeline::sRenderDeferred)
        {
			gPipeline.renderDeferredLighting(&gPipeline.mScreen);
		}

		LLPipeline::sUnderWaterRender = FALSE;

		{
			//capture the frame buffer.
			LLSceneMonitor::getInstanceFast()->capture();
		}

		LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_RENDERUI);
		if (!for_snapshot)
		{
			render_ui();
			swap();
		}

		
		LLSpatialGroup::sNoDelete = FALSE;
		gPipeline.clearReferences();

		gPipeline.rebuildGroups();
	}

	LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_FRAME_STATS);
	
	stop_glerror();

	if (LLPipeline::sRenderFrameTest)
	{
		send_agent_resume();
		LLPipeline::sRenderFrameTest = FALSE;
	}

	display_stats();
				
	LLAppViewer::instance()->pingMainloopTimeout(STR_DISPLAY_DONE);

	gShiftFrame = false;

	if (gShaderProfileFrame)
	{
		gShaderProfileFrame = FALSE;
		LLGLSLShader::finishProfile();
	}
}

void render_hud_attachments()
{
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
		
	LLMatrix4a current_proj = get_current_projection();
	LLMatrix4a current_mod = get_current_modelview();

	// clamp target zoom level to reasonable values
//	gAgentCamera.mHUDTargetZoom = llclamp(gAgentCamera.mHUDTargetZoom, 0.1f, 1.f);
// [RLVa:KB] - Checked: 2010-08-22 (RLVa-1.2.1a) | Modified: RLVa-1.0.0c
	gAgentCamera.mHUDTargetZoom = llclamp(gAgentCamera.mHUDTargetZoom, (!gRlvAttachmentLocks.hasLockedHUD()) ? 0.1f : 0.85f, 1.f);
// [/RLVa:KB]

	// smoothly interpolate current zoom level
	gAgentCamera.mHUDCurZoom = ll_lerp(gAgentCamera.mHUDCurZoom, gAgentCamera.getAgentHUDTargetZoom(), LLSmoothInterpolation::getInterpolant(0.03f));

	if (!ALCinematicMode::isEnabled() && LLPipeline::sShowHUDAttachments && !gDisconnected && setup_hud_matrices())
	{
		LLPipeline::sRenderingHUDs = TRUE;
		LLCamera hud_cam = LLViewerCamera::instanceFast();
		hud_cam.setOrigin(-1.f,0,0);
		hud_cam.setAxes(LLVector3(1,0,0), LLVector3(0,1,0), LLVector3(0,0,1));
		LLViewerCamera::updateFrustumPlanes(hud_cam, TRUE);

		static LLCachedControl<bool> renderHUDParticles(gSavedSettings, "RenderHUDParticles");
		bool render_particles = (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_PARTICLES) && renderHUDParticles);
		
		//only render hud objects
		gPipeline.pushRenderTypeMask();
		
		// turn off everything
		gPipeline.andRenderTypeMask(LLPipeline::END_RENDER_TYPES);
		// turn on HUD
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
		// turn on HUD particles
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);

		// if particles are off, turn off hud-particles as well
		if (!render_particles)
		{
			// turn back off HUD particles
			gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
		}

		bool has_ui = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);
		if (has_ui)
		{
			gPipeline.toggleRenderDebugFeature(LLPipeline::RENDER_DEBUG_FEATURE_UI);
		}

		S32 use_occlusion = LLPipeline::sUseOcclusion;
		LLPipeline::sUseOcclusion = 0;
				
		//cull, sort, and render hud objects
		static LLCullResult result;
		LLSpatialGroup::sNoDelete = TRUE;

		LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
		gPipeline.updateCull(hud_cam, result);

		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_BUMP);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_SIMPLE);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_VOLUME);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_ALPHA);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_ALPHA_MASK);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_FULLBRIGHT_ALPHA_MASK);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_FULLBRIGHT);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_ALPHA);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_ALPHA_MASK);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_BUMP);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_MATERIAL);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_ALPHA_MASK);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_SHINY);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_SHINY);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_INVISIBLE);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_INVISI_SHINY);
		
		gPipeline.stateSort(hud_cam, result);

		gPipeline.renderGeom(hud_cam);

		LLSpatialGroup::sNoDelete = FALSE;
		//gPipeline.clearReferences();

		render_hud_elements();

		//restore type mask
		gPipeline.popRenderTypeMask();

		if (has_ui)
		{
			gPipeline.toggleRenderDebugFeature(LLPipeline::RENDER_DEBUG_FEATURE_UI);
		}
		LLPipeline::sUseOcclusion = use_occlusion;
		LLPipeline::sRenderingHUDs = FALSE;
	}
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.popMatrix();
	
	set_current_projection(current_proj);
	set_current_modelview(current_mod);
}

LLRect get_whole_screen_region()
{
	LLRect whole_screen = gViewerWindow->getWorldViewRectScaled();
	
	// apply camera zoom transform (for high res screenshots)
	auto& vwrCamera = LLViewerCamera::instanceFast();
	F32 zoom_factor = vwrCamera.getZoomFactor();
	S16 sub_region = vwrCamera.getZoomSubRegion();
	if (zoom_factor > 1.f)
	{
		S32 num_horizontal_tiles = llceil(zoom_factor);
		S32 tile_width = ll_round((F32)gViewerWindow->getWorldViewWidthScaled() / zoom_factor);
		S32 tile_height = ll_round((F32)gViewerWindow->getWorldViewHeightScaled() / zoom_factor);
		int tile_y = sub_region / num_horizontal_tiles;
		int tile_x = sub_region - (tile_y * num_horizontal_tiles);
			
		whole_screen.setLeftTopAndSize(tile_x * tile_width, gViewerWindow->getWorldViewHeightScaled() - (tile_y * tile_height), tile_width, tile_height);
	}
	return whole_screen;
}

bool get_hud_matrices(const LLRect& screen_region, LLMatrix4a &proj, LLMatrix4a &model)
{
	if (isAgentAvatarValid() && gAgentAvatarp->hasHUDAttachment())
	{
		auto& vwrCamera = LLViewerCamera::instanceFast();
		F32 zoom_level = gAgentCamera.mHUDCurZoom;
		LLBBox hud_bbox = gAgentAvatarp->getHUDBBox();
		
		F32 hud_depth = llmax(1.f, hud_bbox.getExtentLocal().mV[VX] * 1.1f);
		proj = ALGLMath::genOrtho(-0.5f * vwrCamera.getAspect(), 0.5f * vwrCamera.getAspect(), -0.5f, 0.5f, 0.f, hud_depth);
		proj.getRow<2>().copyComponent<2>(LLVector4a(-0.01f));

		F32 aspect_ratio = LLViewerCamera::getInstance()->getAspect();
		
		F32 scale_x = (F32)gViewerWindow->getWorldViewWidthScaled() / (F32)screen_region.getWidth();
		F32 scale_y = (F32)gViewerWindow->getWorldViewHeightScaled() / (F32)screen_region.getHeight();

		proj.applyTranslation_affine(
			clamp_rescale((F32)(screen_region.getCenterX() - screen_region.mLeft), 0.f, (F32)gViewerWindow->getWorldViewWidthScaled(), 0.5f * scale_x * aspect_ratio, -0.5f * scale_x * aspect_ratio),
			clamp_rescale((F32)(screen_region.getCenterY() - screen_region.mBottom), 0.f, (F32)gViewerWindow->getWorldViewHeightScaled(), 0.5f * scale_y, -0.5f * scale_y),
			0.f);
		proj.applyScale_affine(scale_x, scale_y, 1.f);

		model = OGL_TO_CFR_ROTATION_4A;
		model.applyTranslation_affine(LLVector3(-hud_bbox.getCenterLocal().mV[VX] + (hud_depth * 0.5f), 0.f, 0.f));
		model.applyScale_affine(zoom_level);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

bool get_hud_matrices(LLMatrix4a &proj, LLMatrix4a &model)
{
	LLRect whole_screen = get_whole_screen_region();
	return get_hud_matrices(whole_screen, proj, model);
}

bool setup_hud_matrices()
{
	LLRect whole_screen = get_whole_screen_region();
	return setup_hud_matrices(whole_screen);
}

bool setup_hud_matrices(const LLRect& screen_region)
{
	LLMatrix4a proj, model;
	bool result = get_hud_matrices(screen_region, proj, model);
	if (!result) return result;
	
	// set up transform to keep HUD objects in front of camera
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.loadMatrix(proj);
	set_current_projection(proj);
	
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.loadMatrix(model);
	set_current_modelview(model);
	return TRUE;
}

void render_ui(F32 zoom_factor, int subfield)
{
    LL_RECORD_BLOCK_TIME(FTM_RENDER_UI);

	LLGLState::checkStates();
	
	LLMatrix4a saved_view = get_current_modelview();

	if (!gSnapshot)
	{
		gGL.pushMatrix();
		gGL.loadMatrix(gGLLastModelView);
		set_current_modelview(get_last_modelview());
	}
	
	if(LLSceneMonitor::getInstanceFast()->needsUpdate())
	{
        LL_RECORD_BLOCK_TIME(FTM_RENDER_UI_SCENE_MON);
		gGL.pushMatrix();
		gViewerWindow->setup2DRender();
		LLSceneMonitor::getInstanceFast()->compare();
		gViewerWindow->setup3DRender();
		gGL.popMatrix();
	}

    // Finalize scene
    gPipeline.renderFinalize();

    LL_RECORD_BLOCK_TIME(FTM_RENDER_HUD);
    render_hud_elements();
// [RLVa:KB] - Checked: RLVa-2.2 (@setoverlay)
    if (RlvActions::hasBehaviour(RLV_BHVR_SETOVERLAY))
    {
        LLVfxManager::instanceFast().runEffect(EVisualEffect::RlvOverlay);
    }
// [/RLVa:KB]
	render_hud_attachments();

	LLGLSDefault gls_default;
	LLGLSUIDefault gls_ui;
	{
		gPipeline.disableLights();
	}

	{
		gGL.color4f(1,1,1,1);
		if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
		{
			if (!gDisconnected)
			{
                LL_RECORD_BLOCK_TIME(FTM_RENDER_UI_3D);
				render_ui_3d();
				LLGLState::checkStates();
			}
			else
			{
				render_disconnected_background();
			}

            LL_RECORD_BLOCK_TIME(FTM_RENDER_UI_2D);
			render_ui_2d();
			LLGLState::checkStates();
		}
		gGL.flush();

		{
            LL_RECORD_BLOCK_TIME(FTM_RENDER_UI_DEBUG_TEXT);
			gViewerWindow->setup2DRender();
			gViewerWindow->updateDebugText();
			gViewerWindow->drawDebugText();
		}

		LLVertexBuffer::unbind();
	}

	if (!gSnapshot)
	{
		set_current_modelview(saved_view);
		gGL.popMatrix();
	}
}

static LLTrace::BlockTimerStatHandle FTM_SWAP("Swap");

void swap()
{
	LL_RECORD_BLOCK_TIME(FTM_SWAP);

	if (gDisplaySwapBuffers)
	{
		gViewerWindow->getWindow()->swapBuffers();
	}
	gDisplaySwapBuffers = TRUE;
}

void renderCoordinateAxes()
{
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.begin(LLRender::LINES);
		gGL.color3f(1.0f, 0.0f, 0.0f);   // i direction = X-Axis = red
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(2.0f, 0.0f, 0.0f);
		gGL.vertex3f(3.0f, 0.0f, 0.0f);
		gGL.vertex3f(5.0f, 0.0f, 0.0f);
		gGL.vertex3f(6.0f, 0.0f, 0.0f);
		gGL.vertex3f(8.0f, 0.0f, 0.0f);
		// Make an X
		gGL.vertex3f(11.0f, 1.0f, 1.0f);
		gGL.vertex3f(11.0f, -1.0f, -1.0f);
		gGL.vertex3f(11.0f, 1.0f, -1.0f);
		gGL.vertex3f(11.0f, -1.0f, 1.0f);

		gGL.color3f(0.0f, 1.0f, 0.0f);   // j direction = Y-Axis = green
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 2.0f, 0.0f);
		gGL.vertex3f(0.0f, 3.0f, 0.0f);
		gGL.vertex3f(0.0f, 5.0f, 0.0f);
		gGL.vertex3f(0.0f, 6.0f, 0.0f);
		gGL.vertex3f(0.0f, 8.0f, 0.0f);
		// Make a Y
		gGL.vertex3f(1.0f, 11.0f, 1.0f);
		gGL.vertex3f(0.0f, 11.0f, 0.0f);
		gGL.vertex3f(-1.0f, 11.0f, 1.0f);
		gGL.vertex3f(0.0f, 11.0f, 0.0f);
		gGL.vertex3f(0.0f, 11.0f, 0.0f);
		gGL.vertex3f(0.0f, 11.0f, -1.0f);

		gGL.color3f(0.0f, 0.0f, 1.0f);   // Z-Axis = blue
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 0.0f, 2.0f);
		gGL.vertex3f(0.0f, 0.0f, 3.0f);
		gGL.vertex3f(0.0f, 0.0f, 5.0f);
		gGL.vertex3f(0.0f, 0.0f, 6.0f);
		gGL.vertex3f(0.0f, 0.0f, 8.0f);
		// Make a Z
		gGL.vertex3f(-1.0f, 1.0f, 11.0f);
		gGL.vertex3f(1.0f, 1.0f, 11.0f);
		gGL.vertex3f(1.0f, 1.0f, 11.0f);
		gGL.vertex3f(-1.0f, -1.0f, 11.0f);
		gGL.vertex3f(-1.0f, -1.0f, 11.0f);
		gGL.vertex3f(1.0f, -1.0f, 11.0f);
	gGL.end();
}


void draw_axes() 
{
	LLGLSUIDefault gls_ui;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	// A vertical white line at origin
	LLVector3 v = gAgent.getPositionAgent();
	gGL.begin(LLRender::LINES);
		gGL.color3f(1.0f, 1.0f, 1.0f); 
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 0.0f, 40.0f);
	gGL.end();
	// Some coordinate axes
	gGL.pushMatrix();
		gGL.translatef( v.mV[VX], v.mV[VY], v.mV[VZ] );
		renderCoordinateAxes();
	gGL.popMatrix();
}

void render_ui_3d()
{
	LLGLSPipeline gls_pipeline;

	//////////////////////////////////////
	//
	// Render 3D UI elements
	// NOTE: zbuffer is cleared before we get here by LLDrawPoolHUD,
	//		 so 3d elements requiring Z buffer are moved to LLDrawPoolHUD
	//

	/////////////////////////////////////////////////////////////
	//
	// Render 2.5D elements (2D elements in the world)
	// Stuff without z writes
	//

	// Debugging stuff goes before the UI.

	stop_glerror();
	
	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.bind();
	}

	// Coordinate axes
	static LLCachedControl<bool> showAxes(gSavedSettings, "ShowAxes");
	if (showAxes)
	{
		draw_axes();
	}

	gViewerWindow->renderSelections(FALSE, FALSE, TRUE); // Non HUD call in render_hud_elements
	stop_glerror();
}

void render_ui_2d()
{
	LLGLSUIDefault gls_ui;

	/////////////////////////////////////////////////////////////
	//
	// Render 2D UI elements that overlay the world (no z compare)

	//  Disable wireframe mode below here, as this is HUD/menus
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//  Menu overlays, HUD, etc
	gViewerWindow->setup2DRender();

	auto& vwrCamera = LLViewerCamera::instanceFast();
	F32 zoom_factor = vwrCamera.getZoomFactor();
	S16 sub_region = vwrCamera.getZoomSubRegion();

	if (zoom_factor > 1.f)
	{
		//decompose subregion number to x and y values
		int pos_y = sub_region / llceil(zoom_factor);
		int pos_x = sub_region - (pos_y*llceil(zoom_factor));
		// offset for this tile
		LLFontGL::sCurOrigin.mX -= ll_round((F32)gViewerWindow->getWindowWidthScaled() * (F32)pos_x / zoom_factor);
		LLFontGL::sCurOrigin.mY -= ll_round((F32)gViewerWindow->getWindowHeightScaled() * (F32)pos_y / zoom_factor);
	}

	stop_glerror();
	//gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);

	// render outline for HUD
	if (isAgentAvatarValid() && gAgentCamera.mHUDCurZoom < 0.98f)
	{
		gGL.pushMatrix();
		S32 half_width = (gViewerWindow->getWorldViewWidthScaled() / 2);
		S32 half_height = (gViewerWindow->getWorldViewHeightScaled() / 2);
		gGL.scalef(LLUI::getScaleFactor().mV[0], LLUI::getScaleFactor().mV[1], 1.f);
		gGL.translatef((F32)half_width, (F32)half_height, 0.f);
		F32 zoom = gAgentCamera.mHUDCurZoom;
		gGL.scalef(zoom,zoom,1.f);
		gGL.color4fv(LLColor4::white.mV);
		gl_rect_2d(-half_width, half_height, half_width, -half_height, FALSE);
		gGL.popMatrix();
		stop_glerror();
	}
	

	if (LLPipeline::RenderUIBuffer)
	{
		if (LLView::sIsRectDirty)
		{
            LLView::sIsRectDirty = false;
			LLRect t_rect;

			gPipeline.mUIScreen.bindTarget();
			gGL.setColorMask(true, true);
			{
				static const S32 pad = 8;

                LLView::sDirtyRect.mLeft -= pad;
                LLView::sDirtyRect.mRight += pad;
                LLView::sDirtyRect.mBottom -= pad;
                LLView::sDirtyRect.mTop += pad;

				LLGLEnable scissor(GL_SCISSOR_TEST);
				static LLRect last_rect = LLView::sDirtyRect;

				//union with last rect to avoid mouse poop
				last_rect.unionWith(LLView::sDirtyRect);
								
				t_rect = LLView::sDirtyRect;
                LLView::sDirtyRect = last_rect;
				last_rect = t_rect;

				last_rect.mLeft = LLRect::tCoordType(last_rect.mLeft / LLUI::getScaleFactor().mV[0]);
				last_rect.mRight = LLRect::tCoordType(last_rect.mRight / LLUI::getScaleFactor().mV[0]);
				last_rect.mTop = LLRect::tCoordType(last_rect.mTop / LLUI::getScaleFactor().mV[1]);
				last_rect.mBottom = LLRect::tCoordType(last_rect.mBottom / LLUI::getScaleFactor().mV[1]);

				//LLRect clip_rect(last_rect);
				
				glClear(GL_COLOR_BUFFER_BIT);

				gViewerWindow->draw();
			}

			gPipeline.mUIScreen.flush();
			gGL.setColorMask(true, false);

            LLView::sDirtyRect = t_rect;
		}

		LLGLDisable cull(GL_CULL_FACE);
		LLGLDisable blend(GL_BLEND);
		S32 width = gViewerWindow->getWindowWidthScaled();
		S32 height = gViewerWindow->getWindowHeightScaled();
		gGL.getTexUnit(0)->bind(&gPipeline.mUIScreen);
		gGL.begin(LLRender::TRIANGLE_STRIP);
		gGL.color4f(1,1,1,1);
		gGL.texCoord2f(0, 0);			gGL.vertex2i(0, 0);
		gGL.texCoord2f(width, 0);		gGL.vertex2i(width, 0);
		gGL.texCoord2f(0, height);		gGL.vertex2i(0, height);
		gGL.texCoord2f(width, height);	gGL.vertex2i(width, height);
		gGL.end();
	}
	else
	{
		gViewerWindow->draw();
	}



	// reset current origin for font rendering, in case of tiling render
	LLFontGL::sCurOrigin.set(0, 0);
}

void render_disconnected_background()
{
	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.bind();
	}

	gGL.color4f(1,1,1,1);
	if (!gDisconnectedImagep && gDisconnected)
	{
		LL_INFOS() << "Loading last bitmap..." << LL_ENDL;

		std::string temp_str;
		temp_str = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + LLStartUp::getScreenLastFilename();

		LLPointer<LLImagePNG> image_png = new LLImagePNG;
		if( !image_png->load(temp_str) )
		{
			//LL_INFOS() << "Bitmap load failed" << LL_ENDL;
			return;
		}
		
		LLPointer<LLImageRaw> raw = new LLImageRaw;
		if (!image_png->decode(raw, 0.0f))
		{
			LL_INFOS() << "Bitmap decode failed" << LL_ENDL;
			gDisconnectedImagep = NULL;
			return;
		}

		U8 *rawp = raw->getData();
		S32 npixels = (S32)image_png->getWidth()*(S32)image_png->getHeight();
		for (S32 i = 0; i < npixels; i++)
		{
			S32 sum = 0;
			sum = *rawp + *(rawp+1) + *(rawp+2);
			sum /= 3;
			*rawp = ((S32)sum*6 + *rawp)/7;
			rawp++;
			*rawp = ((S32)sum*6 + *rawp)/7;
			rawp++;
			*rawp = ((S32)sum*6 + *rawp)/7;
			rawp++;
		}

		
		raw->expandToPowerOfTwo();
		gDisconnectedImagep = LLViewerTextureManager::getLocalTexture(raw.get(), FALSE );
		gStartTexture = gDisconnectedImagep;
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	}

	// Make sure the progress view always fills the entire window.
	S32 width = gViewerWindow->getWindowWidthScaled();
	S32 height = gViewerWindow->getWindowHeightScaled();

	if (gDisconnectedImagep)
	{
		LLGLSUIDefault gls_ui;
		gViewerWindow->setup2DRender();
		gGL.pushMatrix();
		{
			// scale ui to reflect UIScaleFactor
			// this can't be done in setup2DRender because it requires a
			// pushMatrix/popMatrix pair
			const LLVector2& display_scale = gViewerWindow->getDisplayScale();
			gGL.scalef(display_scale.mV[VX], display_scale.mV[VY], 1.f);

			gGL.getTexUnit(0)->bind(gDisconnectedImagep);
			gGL.color4f(1.f, 1.f, 1.f, 1.f);
			gl_rect_2d_simple_tex(width, height);
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		}
		gGL.popMatrix();
	}
	gGL.flush();

	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.unbind();
	}

}

void display_cleanup()
{
	gDisconnectedImagep = NULL;
}


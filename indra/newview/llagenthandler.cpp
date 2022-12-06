/**
 * @file llagenthandler.cpp
 * @brief Command handler involving agent requests
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "llavataractions.h"
#include "llcommandhandler.h"
#include "llfloaterreg.h"
#include "llmutelist.h"
#include "llnotificationsutil.h"
#include "llpanelblockedlist.h"

class LLAgentHandler : public LLCommandHandler
{
  public:
    // requires trusted browser to trigger
    LLAgentHandler() : LLCommandHandler("agent", UNTRUSTED_THROTTLE) {}

    bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
    {
        if (params.size() < 2)
            return false;
        LLUUID avatar_id;
        if (!avatar_id.set(params[0].asStringRef(), FALSE))
        {
            return false;
        }

        const std::string verb = params[1].asString();
        if (verb == "about")
        {
            LLAvatarActions::showProfile(avatar_id);
            return true;
        }

        if (verb == "inspect")
        {
            LLFloaterReg::showInstance("inspect_avatar", LLSD().with("avatar_id", avatar_id));
            return true;
        }

        if (verb == "im")
        {
            LLAvatarActions::startIM(avatar_id);
            return true;
        }

        if (verb == "pay")
        {
            if (!LLUI::getInstanceFast()->mSettingGroups["config"]->getBOOL("EnableAvatarPay"))
            {
                LLNotificationsUtil::add("NoAvatarPay", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
                return true;
            }

            LLAvatarActions::pay(avatar_id);
            return true;
        }

        if (verb == "offerteleport")
        {
            LLAvatarActions::offerTeleport(avatar_id);
            return true;
        }

        if (verb == "requestfriend")
        {
            LLAvatarActions::requestFriendshipDialog(avatar_id);
            return true;
        }

        if (verb == "removefriend")
        {
            LLAvatarActions::removeFriendDialog(avatar_id);
            return true;
        }

        if (verb == "mute")
        {
            if (!LLAvatarActions::isBlocked(avatar_id))
            {
                LLAvatarActions::toggleBlock(avatar_id);
            }
            return true;
        }

        if (verb == "unmute")
        {
            if (LLAvatarActions::isBlocked(avatar_id))
            {
                LLAvatarActions::toggleBlock(avatar_id);
            }
            return true;
        }

        if (verb == "block")
        {
            if (params.size() > 2)
            {
                const std::string object_name = LLURI::unescape(params[2].asString());
                LLMute            mute(avatar_id, object_name, LLMute::OBJECT);
                LLMuteList::getInstance()->add(mute);
                LLPanelBlockedList::showPanelAndSelect(mute.mID);
            }
            return true;
        }

        if (verb == "unblock")
        {
            if (params.size() > 2)
            {
                const std::string object_name = params[2].asString();
                LLMute            mute(avatar_id, object_name, LLMute::OBJECT);
                LLMuteList::getInstance()->remove(mute);
            }
            return true;
        }
        return false;
    }
};
LLAgentHandler gAgentHandler;

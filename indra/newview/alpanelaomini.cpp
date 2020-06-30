/*
 * @file alpanelaomini.cpp
 * @brief Animation overrides mini control panel
 *
 * Copyright (c) 2016, Cinder Roxley <cinder@sdf.org>
 * Copyright (C) 2020, Rye Mutt <rye@alchemyviewer.org>
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "llviewerprecompiledheaders.h"
#include "alpanelaomini.h"

#include "alaoengine.h"
#include "alfloaterao.h"
#include "llfloaterreg.h"
#include "llcombobox.h"

static LLPanelInjector<ALPanelAOMini> t_ao_mini("ao_mini");

ALPanelAOMini::ALPanelAOMini() 
	: LLPanel()
	, mSetList(nullptr)
{
	mCommitCallbackRegistrar.add("AO.SitOverride", boost::bind(&ALPanelAOMini::onClickSit, this, _2));
	mCommitCallbackRegistrar.add("AO.NextAnim", boost::bind(&ALPanelAOMini::onClickNext, this));
	mCommitCallbackRegistrar.add("AO.PrevAnim", boost::bind(&ALPanelAOMini::onClickPrevious, this));
	mCommitCallbackRegistrar.add("AO.OpenFloater", boost::bind(&ALPanelAOMini::openAOFloater, this));
	
	//mEnableCallbackRegistrar.add("AO.EnableSet", boost::bind());
	//mEnableCallbackRegistrar.add("AO.EnableState", boost::bind());
}

ALPanelAOMini::~ALPanelAOMini()
{
	if (mReloadCallback.connected())
		mReloadCallback.disconnect();
	if (mSetChangedCallback.connected())
		mSetChangedCallback.disconnect();
}

BOOL ALPanelAOMini::postBuild()
{
	mSetList = getChild<LLComboBox>("set_list");
	mSetList->setCommitCallback(boost::bind(&ALPanelAOMini::onSelectSet, this, _2));
	mReloadCallback = ALAOEngine::instance().setReloadCallback(boost::bind(&ALPanelAOMini::updateSetList, this));
	mSetChangedCallback = ALAOEngine::instance().setSetChangedCallback(boost::bind(&ALPanelAOMini::onSetChanged, this, _1));
	
	return TRUE;
}

/////////////////////////////////////
// Callback receivers

void ALPanelAOMini::updateSetList()
{
	std::vector<ALAOSet*> list = ALAOEngine::getInstance()->getSetList();
	if (list.empty())
	{
		return;
	}
	mSetList->removeall();
	for (ALAOSet* set : list)
	{
		mSetList->add(set->getName(), &set, ADD_BOTTOM, true);
	}
	const std::string& current_set = ALAOEngine::instance().getCurrentSetName();
	mSetList->selectByValue(LLSD(current_set));
}

void ALPanelAOMini::onSetChanged(const std::string& set_name)
{
	mSetList->selectByValue(LLSD(set_name));
}

////////////////////////////////////
// Control actions

void ALPanelAOMini::onSelectSet(const LLSD& userdata)
{
	ALAOSet* selected_set = ALAOEngine::instance().getSetByName(userdata.asString());
	if (selected_set)
	{
		ALAOEngine::instance().selectSet(selected_set);
	}
}

void ALPanelAOMini::onClickSit(const LLSD& userdata)
{
	const std::string& current_set = ALAOEngine::instance().getCurrentSetName();
	ALAOSet* selected_set = ALAOEngine::instance().getSetByName(current_set);
	if (selected_set)
	{
		ALAOEngine::instance().setOverrideSits(selected_set, userdata.asBoolean());
	}
}

void ALPanelAOMini::onClickNext()
{
	ALAOEngine::instance().cycle(ALAOEngine::CycleNext);
}

void ALPanelAOMini::onClickPrevious()
{
	ALAOEngine::instance().cycle(ALAOEngine::CyclePrevious);
}

void ALPanelAOMini::openAOFloater()
{
	LLFloaterReg::showInstance("ao");
}
